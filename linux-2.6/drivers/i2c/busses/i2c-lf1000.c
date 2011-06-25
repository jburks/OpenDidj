/*
 * drivers/i2c/bus/i2c-lf1000.c
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * I2C bus driver for the LF1000 CPUs.  Based heavily on 
 * i2c-at91.c and i2c-mpc.c
 *
 * TODO: more pin configurations (for the rest of the channels)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/i2c.h>
#include <mach/gpio.h>

#define I2C_CHANNEL		CONFIG_I2C_LF1000_CHANNEL
#define LF1000_I2C_TIMEOUT	10 /* (in jiffies) */
#define LF1000_I2C_RATE_HZ	100000

/* FIXME: add channel 0 settings, choose based on I2C_CHANNEL */
#define I2C_SCL0_PORT	GPIO_PORT_A
#define I2C_SCL0_PIN	GPIO_PIN26
#define I2C_SCL0_FN	GPIO_ALT1
#define I2C_SDA0_PORT	GPIO_PORT_A
#define I2C_SDA0_PIN	GPIO_PIN27
#define I2C_SDA0_FN	GPIO_ALT1

#define I2C_SCL1_PORT	GPIO_PORT_A
#define I2C_SCL1_PIN	GPIO_PIN28
#define I2C_SCL1_FN	GPIO_ALT1
#define I2C_SDA1_PORT	GPIO_PORT_A
#define I2C_SDA1_PIN	GPIO_PIN29
#define I2C_SDA1_FN	GPIO_ALT1

enum lf1000_i2c_state {
	I2C_SEND_ADDR,
	I2C_SEND_DATA,
	I2C_SEND_DONE,
};

struct lf1000_i2c {
	int			div;

	/* device access */
	wait_queue_head_t	wait;
	int			ready;

	/* bus access */
	wait_queue_head_t	bus_access;
	int			busy;
	struct i2c_adapter	adap;
	
	void __iomem		*reg_base;

	unsigned long		iobase;
	unsigned long		iosize;

	int			irq;
};

static irqreturn_t lf1000_i2c_irq(int irq, void *dev_id)
{
	struct lf1000_i2c *i2c = dev_id;
	u32 tmp = ioread32(i2c->reg_base+IRQ_PEND);

	if(!(tmp & (1<<PEND))) /* sanity check */
		return IRQ_NONE;

	/* clear pending interrupt */
	tmp |= (1<<PEND);
	iowrite32(tmp, i2c->reg_base+IRQ_PEND);

	i2c->ready = 1;
	wake_up_interruptible(&i2c->wait); /* wake up anyone that is waiting */
	return IRQ_HANDLED;
}

static int lf1000_i2c_wait(struct lf1000_i2c *i2c)
{
	int ret = wait_event_interruptible_timeout(i2c->wait, (i2c->ready),
						   LF1000_I2C_TIMEOUT);

	if(unlikely(ret < 0))
		printk(KERN_INFO "i2c: interrupted\n");
	else if(unlikely(!(i2c->ready)))
		return -ETIMEDOUT;

	i2c->ready = 0;
	return 0;
}

static int i2c_bus_available(struct lf1000_i2c *i2c)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&i2c->bus_access, flags);
	ret = !(i2c->busy);
	spin_unlock_irqrestore(&i2c->bus_access, flags);

	return ret;
}

static void start_stop_condition(struct lf1000_i2c *i2c)
{
	u32 tmp = ioread32(i2c->reg_base+IRQ_PEND);
	tmp |= (1<<OP_HOLD); /* generate a condition */
	iowrite32(tmp, i2c->reg_base+IRQ_PEND);
}

static void lf1000_i2c_clock(struct lf1000_i2c *i2c, char en)
{
	u32 tmp = ioread32(i2c->reg_base+I2C_CLKENB);

	en ? BIT_SET(tmp, 3) :BIT_CLR(tmp, 3);

	iowrite32(tmp, i2c->reg_base+I2C_CLKENB);
}

/* initialize the I2C hardware, see page 17-6 in the MP2530 data book */
static void lf1000_i2c_hwinit(struct lf1000_i2c *i2c)
{
	/* clear control registers */
	iowrite32(0, i2c->reg_base+ICCR);
	iowrite32(0, i2c->reg_base+BURST_CTRL);

	/* Pclk/256/div, enable interrupts */
	iowrite32((1<<CLK_SRC)|((i2c->div-1)<<CLK_SCALER)|(1<<IRQ_ENB), 
			i2c->reg_base+ICCR);

	iowrite32((1<<CNT_MAX), i2c->reg_base+QCNT_MAX);

	iowrite32(0x1010, i2c->reg_base+ICSR);

	start_stop_condition(i2c); /* STOP */
}

/* write to a slave device, see page 17-6 in the MP2530 data book */
static int xfer_write(struct lf1000_i2c *i2c, unsigned char *buf, int length)
{
	u32 tmp;
	int ret;
	enum lf1000_i2c_state state = I2C_SEND_ADDR;

	/* configure for master transmitter mode */
	tmp = (ioread32(i2c->reg_base+ICSR) & 0x10F0);
	tmp |= ((1<<IRQ_ENB)|(1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TX_RX)|
			(1<<ST_BUSY)|(1<<TXRX_ENB));
	iowrite32(tmp, i2c->reg_base+ICSR);

	start_stop_condition(i2c); /*START*/	

	while(1) {
		switch(state) {
			case I2C_SEND_ADDR:
			ret = lf1000_i2c_wait(i2c);
			if(ret != 0)
				goto done_write;
			tmp = ioread32(i2c->reg_base+ICSR); /* check for an ACK */
			if(tmp & (1<<ACK_STATUS)) {
				printk(KERN_INFO "i2c: no ACK in xfer_write\n");
				ret = -EFAULT;
				goto done_write;
			}
			state = I2C_SEND_DATA;
			break;

			case I2C_SEND_DATA:
			iowrite32(*buf++, i2c->reg_base+IDSR); /* write data */
			start_stop_condition(i2c); /* START */
			ret = lf1000_i2c_wait(i2c); /* wait for IRQ */
			if(ret != 0)
				goto done_write;
			if(--length <= 0)
				state = I2C_SEND_DONE;
			break;

			case I2C_SEND_DONE:
			ret = 0;
			goto done_write;
		}
	}

done_write:
	/* set up to generate STOP condition */
	tmp = (ioread32(i2c->reg_base+ICSR) & 0x1F0F);
	tmp |= ((1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TX_RX)|(1<<TXRX_ENB));
	iowrite32(tmp, i2c->reg_base+ICSR);

	start_stop_condition(i2c); /* STOP */

	iowrite32(0, i2c->reg_base+ICSR); /* turn off I2C controller */
	return 0;
}

/* read from a slave device, see page 17-7 in the MP2530 data book */
static int xfer_read(struct lf1000_i2c *i2c, unsigned char *buf, int length)
{
	u32 tmp;
	int ret;
	enum lf1000_i2c_state state = I2C_SEND_ADDR;

	/* configure for master receiver mode */
	tmp = (ioread32(i2c->reg_base+ICSR) & 0x1F0F);
	tmp |= ((1<<IRQ_ENB)|(1<<ST_ENB)|(1<<MASTER_SLV)|
			(1<<ST_BUSY)|(1<<TXRX_ENB));
	iowrite32(tmp, i2c->reg_base+ICSR);

	start_stop_condition(i2c); /*START*/	

	while(1) {
		switch(state) {
			case I2C_SEND_ADDR:
			ret = lf1000_i2c_wait(i2c);
			if(ret != 0)
				goto done_read;
			tmp = ioread32(i2c->reg_base+ICSR); /* check for an ACK */
			if(tmp & (1<<ACK_STATUS)) {
				printk(KERN_INFO "i2c: no ACK in xfer_read\n");
				ret = -EFAULT;
				goto done_read;
			}
			state = I2C_SEND_DATA;
			break;

			case I2C_SEND_DATA:
			*buf++ = ioread32(i2c->reg_base+IDSR); /* get data */
			start_stop_condition(i2c); /* START (request more data) */
			ret = lf1000_i2c_wait(i2c); /* wait for IRQ */
			if(ret != 0)
				goto done_read;

			if(--length <= 0)
				state = I2C_SEND_DONE;
			break;

			case I2C_SEND_DONE:
			ret = 0;
			goto done_read;
		}
	}

done_read:
	/* set up to generate STOP condition */
	tmp = ioread32(i2c->reg_base+ICSR);
	tmp &= ~(0x1F0F);
	tmp |= ((1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TXRX_ENB));
	iowrite32(tmp, i2c->reg_base+ICSR);

	start_stop_condition(i2c); /* STOP */

	iowrite32(0, i2c->reg_base+ICSR); /* turn off I2C controller */
	return ret;
}

/* generic I2C master transfer entrypoint */
static int lf1000_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct lf1000_i2c *i2c = adap->algo_data;
	int i, ret;
	unsigned long flags;

	/* wait to get access to the bus */
	spin_lock_irqsave(&i2c->bus_access, flags);
	while(i2c->busy) {
		spin_unlock_irqrestore(&i2c->bus_access, flags);
		if(wait_event_interruptible(i2c->bus_access, 
					    i2c_bus_available(i2c))) {
			i2c->busy = 0; 
			return -ERESTARTSYS;
		}
		spin_lock_irqsave(&i2c->bus_access, flags);
	}
	i2c->busy = 1; /* got the bus */
	spin_unlock_irqrestore(&i2c->bus_access, flags);

	lf1000_i2c_clock(i2c, 1);

	for(i = 0; i < num; i++) {
		lf1000_i2c_hwinit(i2c);

		/* set slave device address */
		iowrite32(msgs[i].addr | ((msgs[i].flags & I2C_M_RD) ? 1 : 0), 
				  i2c->reg_base+IDSR);

		if(msgs[i].len && msgs[i].buf) {
			if(msgs[i].flags & I2C_M_RD) 
				ret = xfer_read(i2c, msgs[i].buf, msgs[i].len);
			else
				ret = xfer_write(i2c, msgs[i].buf, msgs[i].len);

			if(ret != 0)
				goto xfer_done;
		}
	}
	ret = i;

xfer_done:
	lf1000_i2c_clock(i2c, 0);
	/* realease the bus */
	spin_lock_irqsave(&i2c->bus_access, flags);
	i2c->busy = 0;
	spin_unlock_irqrestore(&i2c->bus_access, flags);
	return ret;
}

/*
 * Return list of supported functionality.
 */
static u32 lf1000_func(struct i2c_adapter *adapter)
{
    return I2C_FUNC_I2C;
}

static struct i2c_algorithm lf1000_algorithm = {
    .master_xfer    = lf1000_xfer,
    .functionality  = lf1000_func,
};

#define res_len(r)              ((r)->end - (r)->start + 1)
static int lf1000_i2c_probe(struct platform_device *dev)
{
	struct lf1000_i2c *i2c;
	struct resource *res;
	int ret = 0;
	unsigned int pclk_hz;
	int irq;

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "i2c: failed to get memory resource\n");
		return -ENODEV;
	}

	irq = platform_get_irq(dev, 0);

	if (irq < 0) {
		printk(KERN_ERR "i2c: failed to get irq resource\n");
		return -ENODEV;
	}
	
	if(!request_mem_region(res->start, res_len(res), res->name)) {
		printk(KERN_ERR "i2c: failed to request memory region\n");
		return -ENOMEM;
	}

	i2c = kzalloc(sizeof(struct lf1000_i2c), GFP_KERNEL);
	if (!i2c) {
		printk(KERN_ERR "i2c: failed to allocate memory\n");
		ret = -ENOMEM;
		goto fail_emalloc;
	}

	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.retries = 5;
	
	/* set up I2C IRQ wait queue */
	init_waitqueue_head(&i2c->wait);

	/* set up I2C bus access queue */
	init_waitqueue_head(&i2c->bus_access);

        /*
 	 * If "dev->id" is negative we consider it as zero.
 	 * The reason to do so is to avoid sysfs names that only make
 	 * sense when there are multiple adapters.
 	 */
	i2c->adap.nr = dev->id !=-1 ? dev->id : 0;
	snprintf(i2c->adap.name, sizeof(i2c->adap.name),"lf1000_i2c-i2c.%u",
		 i2c->adap.nr);

	i2c->reg_base = ioremap(res->start, res_len(res));
	if(!i2c->reg_base) {
		ret = -EIO;
		goto fail_eremap;
	}

	i2c->iobase = res->start;
	i2c->iosize = res_len(res);

	i2c->adap.algo = &lf1000_algorithm;
	i2c->adap.class = I2C_CLASS_HWMON;

	platform_set_drvdata(dev, i2c);

	/* set up IRQ handler */
	i2c->irq = irq;
	if(i2c->irq < 0) {
		printk(KERN_ERR "i2c: failed to get an IRQ\n");
		ret = i2c->irq;
		goto fail_irq;
	}
	ret = request_irq(i2c->irq, lf1000_i2c_irq, 
			IRQF_DISABLED, i2c->adap.name, i2c);
	if(ret) {
		printk(KERN_ERR "i2c: requesting IRQ failed\n" );
		goto fail_irq;
	}

	pclk_hz = get_pll_freq(PCLK_PLL)/2;
	i2c->div = lf1000_CalcDivider(pclk_hz/256, LF1000_I2C_RATE_HZ);
	if(i2c->div < 0) {
		printk(KERN_ALERT "i2c: failed to get divider, using 16\n");
		i2c->div = 16;
	}
	else if(i2c->div > 16) {
		printk(KERN_ALERT "i2c: divider too high, using 16\n");
		i2c->div = 16;
	}

	if (dev->id < 0)
		dev->id = 0;

	/* set up IO pins */
	if (dev->id == 0) {
		gpio_configure_pin(I2C_SCL0_PORT, I2C_SCL0_PIN, I2C_SCL0_FN,
				1, 0, 0);
		gpio_configure_pin(I2C_SDA0_PORT, I2C_SDA0_PIN, I2C_SDA0_FN,
				1, 0, 0);
	} else if (dev->id == 1) {
		gpio_configure_pin(I2C_SCL1_PORT, I2C_SCL1_PIN, I2C_SCL1_FN,
				1, 0, 0);
		gpio_configure_pin(I2C_SDA1_PORT, I2C_SDA1_PIN, I2C_SDA1_FN,
				1, 0, 0);
	}

	/* initialize I2C hardware */
	lf1000_i2c_hwinit(i2c);
	lf1000_i2c_clock(i2c,1);

	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &dev->dev;
	
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if(ret != 0) {
		printk(KERN_ERR "i2c: failed to add adapter\n");
		goto fail_register;
	}

	return 0;

fail_register:
	lf1000_i2c_clock(i2c, 0);
	platform_set_drvdata(dev, NULL);
fail_irq:
	free_irq(i2c->irq, NULL);
	i2c->irq = -1;
	iounmap(i2c->reg_base);
fail_eremap:
	kfree(i2c);
fail_emalloc:
	release_mem_region(res->start, (res->end - res->start) + 1);
	printk(KERN_DEBUG "%s.%s:%d: error loading driver",
		__FILE__, __FUNCTION__, __LINE__);
	return ret;
}

static int lf1000_i2c_remove(struct platform_device *dev)
{
	struct lf1000_i2c *i2c = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	i2c_del_adapter(&i2c->adap);
	free_irq(i2c->irq, i2c);
	
	lf1000_i2c_clock(i2c,0);

	iounmap(i2c->reg_base);
	release_mem_region(i2c->iobase, i2c->iosize);
	kfree(i2c);

	return 0;
}

#ifdef CONFIG_PM
static int lf1000_i2c_suspend(struct platform_device *dev, pm_message_t mesg)
{
	struct lf1000_i2c *i2c = platform_get_drvdata(dev);
	lf1000_i2c_clock(i2c,0);
	return 0;
}

static int lf1000_i2c_resume(struct platform_device *dev)
{
	struct lf1000_i2c *i2c = platform_get_drvdata(dev);
	lf1000_i2c_clock(i2c,1);
	return 0;
}
#else
#define lf1000_i2c_suspend	NULL
#define lf1000_i2c_resume	NULL
#endif

static struct platform_driver lf1000_i2c_driver = {
	.probe      = lf1000_i2c_probe,
	.remove     = lf1000_i2c_remove,
	.suspend    = lf1000_i2c_suspend,
	.resume     = lf1000_i2c_resume,
	.driver     = {
		.name   = "lf1000-i2c",
		.owner  = THIS_MODULE,
	},
};

static int __init lf1000_i2c_init(void)
{
	return platform_driver_register(&lf1000_i2c_driver);
}

static void __exit lf1000_i2c_exit(void)
{
	return platform_driver_unregister(&lf1000_i2c_driver);
}

module_init(lf1000_i2c_init);
module_exit(lf1000_i2c_exit);

MODULE_AUTHOR("Andrey Yurovsky");
MODULE_AUTHOR("Scott Esters");
MODULE_DESCRIPTION("I2C driver for LF1000");
MODULE_LICENSE("GPL");
