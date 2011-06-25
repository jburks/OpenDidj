/* 
 * drivers/spi/spi_lf1000.c
 *
 * LF1000 SPI/SSP Driver
 *
 * Copyright 2009 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/lf1000/spi_ioctl.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

#include <mach/common.h>
#include <mach/gpio.h>
#include <mach/platform.h>

#include "spi_lf1000_hal.h"
#include "spi_lf1000.h"

#define MIN(a,b) \
	(( (a) < (b) ) ? (a) : (b) )

#define MAX(a,b) \
	(( (a) > (b) ) ? (a) : (b) )

#define LF1000_SPI_MAX_FREQ		25000000
#define LF1000_SPI_FIFO_DEPTH		32

extern void lf1000_dpc_register_spi(struct spi_device *slave);

static int lf1000_spi_setup_transfer(struct spi_device *spi, struct spi_transfer *t);

struct lf1000_pin_cfg {
	const enum gpio_port		port;
	const enum gpio_pin		pin;
	const enum gpio_function	fn;
};

struct lf1000_spi_pins {
	const struct lf1000_pin_cfg	clk;
	const struct lf1000_pin_cfg	frm;
	const struct lf1000_pin_cfg	rx;
	const struct lf1000_pin_cfg	tx;
};

static const struct lf1000_spi_pins pins[] = {
	[0] = {
		.clk	=	{ GPIO_PORT_B, GPIO_PIN13, GPIO_ALT1 },
		.frm	=	{ GPIO_PORT_B, GPIO_PIN12, GPIO_ALT1 },
		.rx	=	{ GPIO_PORT_B, GPIO_PIN14, GPIO_ALT1 },
		.tx	=	{ GPIO_PORT_B, GPIO_PIN15, GPIO_ALT1 },
	},
	[1] = {
		.clk	=	{ GPIO_PORT_C, GPIO_PIN4, GPIO_ALT1 },
		.frm	=	{ GPIO_PORT_C, GPIO_PIN3, GPIO_ALT1 },
		.rx	=	{ GPIO_PORT_C, GPIO_PIN5, GPIO_ALT1 },
		.tx	=	{ GPIO_PORT_C, GPIO_PIN6, GPIO_ALT1 },
	},
	[2] = {
		.clk	=	{ GPIO_PORT_B, GPIO_PIN0, GPIO_ALT2 },
		.frm	=	{ GPIO_PORT_B, GPIO_PIN5, GPIO_ALT2 },
		.rx	=	{ GPIO_PORT_B, GPIO_PIN1, GPIO_ALT2 },
		.tx	=	{ GPIO_PORT_B, GPIO_PIN2, GPIO_ALT2 },
	},
};

struct lf1000_spi {
	void __iomem		*base;
	unsigned long		phys;
	int			irq;		// assigned IRQ number

	spinlock_t		lock;
	struct work_struct	work;		// bottom half
	struct list_head	msg_queue;
	
	struct spi_master	*master;

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*proc_port;
#endif
};

static struct workqueue_struct *lf1000_spi_wq;

/*******************
 * sysfs Interface *
 *******************/
#ifdef CONFIG_SPI_DEBUG
static ssize_t show_registers(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t len = 0;
	struct spi_master	*master;
	struct lf1000_spi	*lf1000spi;

	master = dev_get_drvdata(dev);
	lf1000spi = spi_master_get_devdata(master);

	len += sprintf(buf+len,"SPI_CONT0  = 0x%04X\n",
		ioread16(lf1000spi->base+SSPSPICONT0));
	len += sprintf(buf+len,"SPI_CONT1  = 0x%04X\n",
		ioread16(lf1000spi->base+SSPSPICONT1));
	len += sprintf(buf+len,"SPI_DATA   = 0x%04X\n",
		ioread16(lf1000spi->base+SSPSPIDATA));
	len += sprintf(buf+len,"SPI_STATE  = 0x%04X\n",
		ioread16(lf1000spi->base+SSPSPISTATE));
	len += sprintf(buf+len,"SPI_CLKENB = 0x%08X\n",
		ioread32(lf1000spi->base+SSPSPICLKENB));
	len += sprintf(buf+len,"SPI_CLKGEN = 0x%04X\n",
		ioread16(lf1000spi->base+SSPSPICLKGEN));

	return len;
}

static DEVICE_ATTR(registers, S_IRUSR|S_IRGRP, show_registers, NULL);

static struct attribute *spi_attributes[] = {
	&dev_attr_registers.attr,
	NULL
};

static struct attribute_group spi_attr_group = {
	.attrs = spi_attributes
};
#endif /* CONFIG_SPI_DEBUG */

/**********************
 * Interrupt Handling *
 *********************/
static void lf1000_spi_interrupt(struct lf1000_spi *lf1000spi, u8 en) {
	u16 reg = ioread16(lf1000spi->base+SSPSPISTATE);

	if(en) {
		BIT_SET(reg, IRQEENB);			// RX, TX completed
		BIT_SET(reg, IRQWENB);			// TX Buffer Empty
		BIT_SET(reg, IRQRENB);			// RX Buffer Full
		BIT_SET(reg, IRQE);			// clear status bits
		BIT_SET(reg, IRQW);
		BIT_SET(reg, IRQR);
	}
	else {
		BIT_CLR(reg, IRQEENB);			// RX, TX completed
		BIT_CLR(reg, IRQWENB);			// TX Buffer Empty
		BIT_CLR(reg, IRQRENB);			// RX Buffer Full
	}
	iowrite16(reg, lf1000spi->base+SSPSPISTATE);
}

static irqreturn_t lf1000_spi_irq(int irq, void *dev_id)
{
	struct spi_master	*master = dev_id;
	struct lf1000_spi	*lf1000spi = spi_master_get_devdata(master);

	u16 reg;

	reg = ioread16(lf1000spi->base+SSPSPISTATE);
	
	if (IS_CLR(reg,IRQE)) {	/* interrupt not from SPI */
		return(IRQ_NONE);
	}

	lf1000_spi_interrupt(lf1000spi, 0);
//	wake_up_interruptible(&lf1000spi->intqueue);	/* wake any tasks */
	return(IRQ_HANDLED);
}

static unsigned int
lf1000_spi_txrx_pio(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct lf1000_spi	*lf1000spi;
	unsigned int		count, extra, tx_c, rx_c, chan;
	int			c;
	void __iomem		*txrx_reg, *status;

	chan = spi->master->bus_num;
	lf1000spi = spi_master_get_devdata(spi->master);
	c = count = xfer->len;

	txrx_reg = lf1000spi->base + SSPSPIDATA;
	status = lf1000spi->base + SSPSPISTATE;

	/*
	 * Since SPI is full-duplex, we want to use as much of the LF1000
	 * SPI TX FIFO as possible to prevent data starvation.  Some slaves
	 * (e.g., SST25VF010A) expect a continuous command stream, and will
	 * not recognize commands that are broken by rising CE#
	 *
	 * So, the strategy is to fill the TX FIFO as quickly as possible.
	 * Meanwhile, the hardware is populating the RX FIFO, which the
	 * driver can dump later.  This is in contrast to other SPI PIO
	 * drivers which implement a write-read-write-read-... scheme, which
	 * is undesirable as described above.
	 */

	/* wait for FIFOs */
	while(IS_CLR(ioread16(status), WFFEMPTY) ) ;
	while(IS_CLR(ioread16(status), RFFEMPTY) ) ioread16(txrx_reg);

	if(spi->bits_per_word <= 8)
	{
		u8		*rx;
		const u8	*tx;

		rx = xfer->rx_buf;
		tx = xfer->tx_buf;

		do {
			/* if we have more than a FIFO's worth to deal with */
			extra = MAX(0, (c - LF1000_SPI_FIFO_DEPTH));
			tx_c = rx_c = MIN(c, LF1000_SPI_FIFO_DEPTH);
			c = extra;

			tx_c--;

			/* Fill Tx */
			if(tx != NULL)
			{
				do {
					iowrite8(*tx++, txrx_reg);
				} while(tx_c--);

			}
			else
			{
				do {
					iowrite8(0, txrx_reg);
				} while (tx_c--);
			}

			/* Dump Rx */	
			do
			{
				/* Wait for FIFOs to catch up */
				while(IS_SET(ioread16(status), RFFEMPTY) ) ;
				if(rx != NULL)
				{
					*rx++ = ioread8(txrx_reg);
				}
				else
				{
					ioread8(txrx_reg);
				}
				rx_c -= 1;
			} while (rx_c);

		} while(c);
	}
	else if(xfer->bits_per_word <= 16)
	{
		u16		*rx;
		const u16	*tx;

		rx = xfer->rx_buf;
		tx = xfer->tx_buf;

		do {
			/* if we have more than a FIFO's worth to deal with */
			extra = MAX(0, (c - LF1000_SPI_FIFO_DEPTH));
			tx_c = rx_c = MIN(c, LF1000_SPI_FIFO_DEPTH);
			c = extra;

			/* Fill Tx */
			do {
				if(tx != NULL)
				{
					iowrite16(*tx++, txrx_reg);
				}
				else
				{
					iowrite16(0, txrx_reg);
				}
				tx_c -= 2;	/* bytes, not words */
			} while (tx_c);

			/* Dump Rx */	
			do
			{
				/* Wait for FIFOs to catch up */
				while(IS_SET(ioread16(status), RFFEMPTY) ) ;
				if(rx != NULL)
				{
					*rx++ = ioread16(txrx_reg);
				}
				else
				{
					ioread16(txrx_reg);
				}
				rx_c -= 2;
			} while (rx_c);

		} while (c);
	}

	return count - c;
}

static void lf1000_spi_work(struct work_struct *work)
{
	struct lf1000_spi	*lf1000spi;

	lf1000spi = container_of(work, struct lf1000_spi, work);
	spin_lock_irq(&lf1000spi->lock);

	while(!list_empty(&lf1000spi->msg_queue))
	{
		struct spi_message	*m;
		struct spi_device	*spi;
		struct spi_transfer	*t = NULL;
		int			par_override = 0;
		int			status = 0;
		
		m = container_of(lf1000spi->msg_queue.next, struct spi_message,
		                 queue);

		list_del_init(&m->queue);
		spin_unlock_irq(&lf1000spi->lock);

		spi = m->spi;

		list_for_each_entry(t, &m->transfers, transfer_list)
		{
			if(t->tx_buf == NULL && t->rx_buf == NULL && t->len)
			{
				status = -EINVAL;
				break;
			}
			if(par_override || t->speed_hz || t->bits_per_word)
			{
				status = lf1000_spi_setup_transfer(spi, t);
				if(status < 0)
					break;
				if(!t->speed_hz && !t->bits_per_word)
					par_override = 0;
			}

			if(t->len)
			{
				unsigned	count;
				count = lf1000_spi_txrx_pio(spi, t);

				if(count != t->len)
				{
					status = -EIO;
					break;
				}
			}

			if(t->delay_usecs)
				udelay(t->delay_usecs);

		}

		if(par_override)
		{
			par_override = 0;
			status = lf1000_spi_setup_transfer(spi, NULL);
		}

		m->status = status;
		m->complete(m->context);

		spin_lock_irq(&lf1000spi->lock);
	}

	spin_unlock_irq(&lf1000spi->lock);
}

static int lf1000_spi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct lf1000_spi 	*lf1000spi;
	u8			word_len = spi->bits_per_word;
	u16			fmt;

	lf1000spi = spi_master_get_devdata(spi->master);

	if(t != NULL && t->bits_per_word)
		word_len = t->bits_per_word;

	fmt = ioread16(lf1000spi->base+SSPSPICONT1);

	if(spi->mode & SPI_CPOL)
		BIT_CLR(fmt, SCLKPOL);
	else
		BIT_SET(fmt, SCLKPOL);

	if(spi->mode & SPI_CPHA)
		BIT_SET(fmt, SCLKSH);
	else
		BIT_CLR(fmt, SCLKSH);

	iowrite16(fmt, lf1000spi->base+SSPSPICONT1);

	fmt = ioread16(lf1000spi->base+SSPSPICONT0);

	fmt &= ~NUMBIT_MASK;
	fmt |= NUMBIT_MASK & ((word_len-1) << NUMBIT);
	
	iowrite16(fmt, lf1000spi->base+SSPSPICONT0);

	return 0;
}
static int lf1000_spi_setup(struct spi_device *spi)
{
	struct lf1000_spi *lf1000spi;
	int ret = 0;

	if(spi->bits_per_word == 0)
		spi->bits_per_word = 8;
	else if (spi->bits_per_word < 8 || spi->bits_per_word > 16) {
		return -EINVAL;
	}

	lf1000spi = spi_master_get_devdata(spi->master);

	ret = lf1000_spi_setup_transfer(spi, NULL);

	return ret;
}

static int lf1000_spi_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct lf1000_spi	*lf1000spi;
	unsigned long		flags;
	struct spi_transfer	*t;

	m->actual_length = 0;
	m->status = 0;

	/* reject invalid messages and transfers */
	if (list_empty(&m->transfers) || !m->complete)
		return -EINVAL;

	list_for_each_entry(t, &m->transfers, transfer_list) {
		const void	*tx_buf = t->tx_buf;
		void		*rx_buf = t->rx_buf;
		unsigned	len = t->len;

		if( t->speed_hz > LF1000_SPI_MAX_FREQ ||
		    (len && !(rx_buf || tx_buf)) ||
		    (t->bits_per_word && (t->bits_per_word < 8 ||
		                          t->bits_per_word > 16))) {
			return -EINVAL;
		}
	}

	/* parameters are valid.  queue work */
	lf1000spi = spi_master_get_devdata(spi->master);

	spin_lock_irqsave(&lf1000spi->lock, flags);
	list_add_tail(&m->queue, &lf1000spi->msg_queue);
	queue_work(lf1000_spi_wq, &lf1000spi->work);
	spin_unlock_irqrestore(&lf1000spi->lock, flags);

	return 0;
}

static void lf1000_spi_cleanup(struct spi_device *spi)
{
}


/****************************************
 *  module functions and initialization *
 ****************************************/

static int lf1000_spi_probe(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct lf1000_spi	*lf1000spi;
	int			ret = 0, chan;
	int			div;
	struct resource		*res;

	master = spi_alloc_master(&pdev->dev, sizeof *lf1000spi);
	if(master == NULL)
	{
		return -ENOMEM;
	}

	master->bus_num = chan = pdev->id;
	master->setup = lf1000_spi_setup;
	master->transfer = lf1000_spi_transfer;
	master->cleanup = lf1000_spi_cleanup;
	master->num_chipselect = 1;

	platform_set_drvdata(pdev, master);

	lf1000spi = spi_master_get_devdata(master);
	lf1000spi->master = master;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "spi: failed to get resource\n");
		ret = -ENODEV;
		goto fail_resource;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1,
				"lf1000-spi")) {
		printk(KERN_ERR "spi: failed to map region\n");
		ret = -EBUSY;
		goto fail_request;
	}

	lf1000spi->phys = res->start;
	lf1000spi->base = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(lf1000spi->base == NULL) {
		printk(KERN_ERR "spi: failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	INIT_WORK(&lf1000spi->work, lf1000_spi_work);

	spin_lock_init(&lf1000spi->lock);
	INIT_LIST_HEAD(&lf1000spi->msg_queue);

	div = lf1000_CalcDivider(get_pll_freq(SPI_CLK_SRC), SPI_SRC_HZ);
	if(div < 0) {
		printk(KERN_ERR "spi: failed to find a clock divider!\n");
		return -EFAULT;
	}

	/* set SPI for 1 MHz devices */
	iowrite16(((div-1)<<SPI_CLKDIV)|(SPI_CLK_SRC<<SPI_CLKSRCSEL), 
			lf1000spi->base+SSPSPICLKGEN);
	/* start clock generation */
	iowrite32((1<<SPI_PCLKMODE)|(1<<SPI_CLKGENENB), 
			lf1000spi->base+SSPSPICLKENB);
	/* output only for transmission, enable, 16 bits, divide internal 
	   clock by 20 to get ~1MHz */
	iowrite16((0<<ZENB)|(0<<RXONLY)|(1<<ENB)|((16-1)<<NUMBIT)|(20<<DIVCNT),
		  lf1000spi->base+SSPSPICONT0);
	/* invert SPI clock, Format B, SPI type */
	iowrite16((0<<SCLKPOL)|(1<<SCLKSH)|(1<<TYPE), lf1000spi->base+SSPSPICONT1);
	/* clear status flags */
	iowrite16(0x0000, lf1000spi->base+SSPSPISTATE);

	/* configure SPI IO pins */
	gpio_configure_pin( pins[chan].tx.port,
	                    pins[chan].tx.pin,
	                    pins[chan].tx.fn, 1, 0, 0);
	gpio_configure_pin( pins[chan].rx.port,
	                    pins[chan].rx.pin,
	                    pins[chan].rx.fn, 0, 0, 0);
	gpio_configure_pin( pins[chan].clk.port,
	                    pins[chan].clk.pin,
	                    pins[chan].clk.fn, 1, 0, 0);
	gpio_configure_pin( pins[chan].frm.port,
	                    pins[chan].frm.pin,
	                    pins[chan].frm.fn, 1, 0, 1);

	lf1000spi->irq = platform_get_irq(pdev, 0);
	if(lf1000spi->irq < 0) {
		printk(KERN_ERR "spi: failed to get an IRQ\n");
		ret = lf1000spi->irq;
		goto fail_irq;
	}
	ret = request_irq(lf1000spi->irq, lf1000_spi_irq, IRQF_DISABLED|IRQF_SAMPLE_RANDOM,
			"spi", master);
	if(ret) {
		printk(KERN_INFO "spi: requesting IRQ failed\n");
		goto fail_irq;
	}

#ifdef CONFIG_SPI_DEBUG
	sysfs_create_group(&pdev->dev.kobj, &spi_attr_group);
#endif

	spi_register_master(master);

	return 0;

fail_irq:
	iounmap(lf1000spi->base);
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
fail_request:
fail_resource:
	spi_master_put(master);
	return ret;
}

static int lf1000_spi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct lf1000_spi	*lf1000spi;
	struct resource *res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	master = platform_get_drvdata(pdev);
	lf1000spi = spi_master_get_devdata(master);

	flush_workqueue(lf1000_spi_wq);
	destroy_workqueue(lf1000_spi_wq);

#ifdef CONFIG_SPI_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &spi_attr_group);
#endif
	if(lf1000spi->irq != -1)
		free_irq(lf1000spi->irq, master);

	if(lf1000spi->base != NULL)
		iounmap(lf1000spi->base);

	release_mem_region(res->start, (res->end - res->start) + 1);
	spi_unregister_master(master);
	return 0;
}

#ifdef CONFIG_PM
static int lf1000_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int lf1000_spi_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define lf1000_spi_suspend NULL
#define lf1000_spi_resume NULL
#endif

static struct platform_driver lf1000_spi_driver = {
	.driver		= {
		.name	= "lf1000-spi",
		.owner	= THIS_MODULE,
	},
	.suspend	= lf1000_spi_suspend,
	.resume		= lf1000_spi_resume,
	.remove		= __devexit_p(lf1000_spi_remove),
};

static void __exit lf1000_spi_exit(void)
{
	platform_driver_unregister(&lf1000_spi_driver);
}

static int __init lf1000_spi_init(void)
{
	lf1000_spi_wq = create_singlethread_workqueue(
	                       lf1000_spi_driver.driver.name);
	if(lf1000_spi_wq == NULL)
		return -1;

	return platform_driver_probe(&lf1000_spi_driver, lf1000_spi_probe);
}

module_init(lf1000_spi_init);
module_exit(lf1000_spi_exit);
MODULE_AUTHOR("Scott Esters");
MODULE_VERSION("1:1.0");
MODULE_LICENSE("GPL");
