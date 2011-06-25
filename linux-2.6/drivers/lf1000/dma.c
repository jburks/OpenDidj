/*
 *  Brian Cavagnolo <brian@cozybit.com>
 *
 *  DMA functions specific to LF1000
 *
 *  Based on linux/arch/arm/mach-pxa/dma.c
 *  Author:	Nicolas Pitre
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#define DMA_IO_BASE 0xC0000000
#define DMA_IO_LENGTH (LF1000_DMA_CHANNELS*0x80)

struct dma_channel {
	char *name;
	void (*irq_handler)(int, void *, unsigned int);
	void *data;
	spinlock_t lock;
	int fifo_len; /* Max number of pending transfers */
};

struct dma_dev {
	struct dma_channel dma_channels[LF1000_DMA_CHANNELS];
	int irq;
	void __iomem *iomem;
	struct platform_device *pdev;
};

static struct dma_dev dev;

/* low-level register handling */
#define DMA_WRITE32(chan, reg, val) iowrite32((val), (dev.iomem+(chan)*0x80+(reg)))
#define DMA_READ32(chan, reg) ioread32(dev.iomem+(chan)*0x80+(reg))
#define DMA_WRITE16(chan, reg, val) iowrite16((val), (dev.iomem+(chan)*0x80+(reg)))
#define DMA_READ16(chan, reg) ioread16(dev.iomem+(chan)*0x80+(reg))

#define DMASRCADDR	(0x0)
#define DMADSTADDR	(0x4)
#define DMALENGTH	(0x8)
#define DMAREQID	(0xA)
#define DMAMODE		(0xC)

#define DMACMDWAIT	(0x20)
#define DMACMDSTOP	(0x24)
#define DMACMDBUSY	(0x28)
#define DMACMDSPACE	(0x2C)

/* The following are internal flags for the dma handler.  Not for use in
 * drivers.
 */
#define DMA_RUN			(1<<19)
#define DMA_INTENB		(1<<18)
#define DMA_INTPEND		(1<<17)
#define DMA_BUSY		(1<<16)
#define DMA_DST_NOREQ		(1<<13)
#define DMA_SRC_NOREQ		(1<<5)

int lf1000_dma_request(char *name, lf1000_dma_prio prio,
		       void (*irq_handler)(int, void *, unsigned int),
		       void *data)
{
	unsigned long flags;
	int i, found = 0;
	struct dma_channel *c;

	/* basic sanity checks */
	if (!name || !irq_handler)
		return -EINVAL;

	do {
		/* try grabbing a DMA channel with the requested priority */
		lf1000_for_each_dma_prio (i, prio) {
			if (!dev.dma_channels[i].name) {
				found = 1;
				break;
			}
		}
		/* if requested prio group is full, try a hier priority */
	} while (!found && prio--);

	if (found) {
		c = &dev.dma_channels[i];
		spin_lock_irqsave(&c->lock, flags);
		c->name = name;
		c->irq_handler = irq_handler;
		c->data = data;
		spin_unlock_irqrestore(&c->lock, flags);
		pr_debug("%s: Registered dma chan %d to %s\n", __FUNCTION__, i,
			       	name);
	} else {
		printk (KERN_WARNING "No more available DMA channels for %s\n",
				name);
		i = -ENODEV;
	}
	return i;
}
EXPORT_SYMBOL(lf1000_dma_request);

void lf1000_dma_free(int dma_ch)
{
	unsigned long flags, tmp;
	struct dma_channel *c;
	c = &dev.dma_channels[dma_ch];

	if (!c->name) {
		printk (KERN_CRIT
			"%s: trying to free channel %d which is already freed\n",
			__FUNCTION__, dma_ch);
		return;
	}

	pr_debug("%s: Releasing dma chan %d\n", __FUNCTION__, dma_ch);

	/* disable interrupt on channel */
	tmp = DMA_READ32(dma_ch, DMAMODE);
	tmp &= ~(DMA_INTENB);
	DMA_WRITE32(dma_ch, DMAMODE, tmp);

	spin_lock_irqsave(&c->lock, flags);
	c->name = NULL;
	spin_unlock_irqrestore(&c->lock, flags);
}
EXPORT_SYMBOL(lf1000_dma_free);

void lf1000_dma_initdesc(struct lf1000_dma_desc *desc)
{
	desc->id = DMA_NOSUCHID;
	desc->flags = 0;
}
EXPORT_SYMBOL(lf1000_dma_initdesc);

int lf1000_dma_launch(int channel, struct lf1000_dma_desc *desc)
{
	struct dma_channel *c;
	unsigned long dmamode, dmalen;
	int off = 0;
	int fifo_space = DMA_READ16(channel, DMACMDSPACE) & 0x7;

	if(lf1000_dma_busy(channel)) {
		if(fifo_space) {
			/* In this case, command goes to fifo instead of channel
			 * hw
			 */
			off = 0x10;
			DMA_WRITE16(channel, DMACMDWAIT, 1);
		} else {
			return -EAGAIN;
		}
	} else if(!fifo_space) {
		/* If we're here, we're not busy, and the fifo is not
		 * empty.  This is an error condition that should never happen.
		 */
		printk(KERN_WARNING
		       "%s: Strange.  HW is not busy, but fifo is not empty.\n",
		       __FUNCTION__);
		return -EAGAIN;
	}

	c = &dev.dma_channels[channel];

	dmamode = desc->flags;

	if(desc->id == DMA_NOSUCHID) {
		/* memory-to-memory */
		dmamode |= DMA_DST_NOREQ;
		dmamode |= DMA_SRC_NOREQ;
	} else {
		if(!(desc->flags & DMA_DST_IO)) {
			dmamode |= DMA_DST_NOREQ;
		}
		if(!(desc->flags & DMA_SRC_IO)) {
			dmamode |= DMA_SRC_NOREQ;
		}
		if(!(desc->flags & DMA_DST_IO) &&
		   !(desc->flags & DMA_SRC_IO)) {
			printk(KERN_CRIT "Bad DMA descriptor!\n");
			return -EINVAL;
		}
		DMA_WRITE16(channel, DMAREQID + off, desc->id);
	}

	dmalen = desc->len - 1;
	
	/* Config and enable dma channel */
	DMA_WRITE32(channel, DMASRCADDR + off, desc->src);
	DMA_WRITE32(channel, DMADSTADDR + off, desc->dst);
	DMA_WRITE16(channel, DMALENGTH + off, dmalen);
	dmamode |= (DMA_INTENB | DMA_RUN);
	pr_debug("%s: Enabling dma of %d bytes from 0x%08x to 0x%08x with DMAMODE 0x%08x\n",
		 __FUNCTION__, (unsigned int)(dmalen + 1),
		 (int)desc->src, (unsigned int)desc->dst,
		 (unsigned int)dmamode);
	DMA_WRITE32(channel, DMAMODE + off, dmamode);
	return 0;
}
EXPORT_SYMBOL(lf1000_dma_launch);

void *lf1000_dma_buf_alloc(size_t size, dma_addr_t *dma_handle)
{
	return dma_alloc_coherent(&dev.pdev->dev, size, dma_handle, GFP_KERNEL);
}
EXPORT_SYMBOL(lf1000_dma_buf_alloc);

void lf1000_dma_buf_free(size_t size, void *vaddr, dma_addr_t dma_handle)
{
	dma_free_coherent(&dev.pdev->dev, size, vaddr, dma_handle);
}
EXPORT_SYMBOL(lf1000_dma_buf_free);

int lf1000_dma_busy(int channel)
{
	if(DMA_READ32(channel, DMAMODE) & DMA_BUSY)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(lf1000_dma_busy);

void lf1000_dma_int_en(int channel)
{
	unsigned long tmp;
	tmp = DMA_READ32(channel, DMAMODE);
	tmp |= DMA_INTENB;
	tmp |= DMA_INTPEND;
	DMA_WRITE32(channel, DMAMODE, tmp);
}
EXPORT_SYMBOL(lf1000_dma_int_en);

void lf1000_dma_int_dis(int channel)
{
	unsigned long tmp;
	tmp = DMA_READ32(channel, DMAMODE);
	tmp &= ~(DMA_INTENB);
	tmp |= DMA_INTPEND;
	DMA_WRITE32(channel, DMAMODE, tmp);
}
EXPORT_SYMBOL(lf1000_dma_int_dis);

void lf1000_dma_int_clr(int channel)
{
	unsigned long tmp;
	tmp = DMA_READ32(channel, DMAMODE);
	tmp |= DMA_INTPEND;
	DMA_WRITE32(channel, DMAMODE, tmp);
}
EXPORT_SYMBOL(lf1000_dma_int_clr);

static irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
	int i, pending;
	unsigned long tmp, flags;
	struct dma_channel *c;

	for (i = 0; i < LF1000_DMA_CHANNELS; i++) {
		if (DMA_READ32(i, DMAMODE) & DMA_INTPEND) {
			c = &dev.dma_channels[i];
			if (c->name && c->irq_handler) {
				spin_lock_irqsave(&c->lock, flags);
				pending = c->fifo_len - DMA_READ16(i, DMACMDSPACE);
				if(!lf1000_dma_busy(i))
					pending--;
				c->irq_handler(i, c->data, pending);
				spin_unlock_irqrestore(&c->lock, flags);
			} else {
				/*
				 * IRQ for an unregistered DMA channel:
				 * let's clear the interrupts and disable it.
				 */
				/* clear the interrupt and disable it. */
				tmp = DMA_READ32(i, DMAMODE);
				tmp &= ~(DMA_INTENB);
				tmp |= DMA_INTPEND;
				DMA_WRITE32(i, DMAMODE, tmp);
				printk(KERN_WARNING "spurious IRQ for DMA channel %d\n", i);
			}
		}
	}
	return IRQ_HANDLED;
}

static int lf1000_dma_remove(struct platform_device* pdev)
{
	int i;
	unsigned long tmp;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if(dev.irq) {
		free_irq(dev.irq, NULL);
	}

	for(i=0; i<LF1000_DMA_CHANNELS; i++) {
		tmp = DMA_READ32(i, DMAMODE);
		tmp &= ~(DMA_INTENB);
		DMA_WRITE32(i, DMAMODE, tmp);
	}

	if(dev.iomem)
		iounmap(dev.iomem);

	release_mem_region(res->start, (res->end - res->start) + 1);

	return 0;
}

static int lf1000_dma_probe(struct platform_device *pdev)
{
	int ret, i;
	struct dma_channel *c;
	unsigned long tmp;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		dev_err(&pdev->dev, "failed to get resource\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1,
				"lf1000-dma")) {
		dev_err(&pdev->dev, "failed to map DMA region\n");
		return -EBUSY;
	}

	memset(&dev, 0, sizeof(struct dma_dev));

	dev.iomem = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(dev.iomem == NULL) {
		dev_err(&pdev->dev, "failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	for(i=0; i<LF1000_DMA_CHANNELS; i++) {
		c = &dev.dma_channels[i];
		c->lock = SPIN_LOCK_UNLOCKED;
		tmp = DMA_READ32(i, DMAMODE);
		tmp &= ~(DMA_INTENB);
		tmp |= DMA_INTPEND;
		DMA_WRITE32(i, DMAMODE, tmp);
		DMA_WRITE16(i, DMACMDSTOP, 1);
		while(DMA_READ16(i, DMACMDBUSY) & 1);
		c->fifo_len = DMA_READ16(i, DMACMDSPACE) + 1;
	}
	
	ret = request_irq(LF1000_DMA_IRQ, dma_irq_handler, 0, "DMA", NULL);
	if(ret) {
		dev_err(&pdev->dev, "can't register IRQ\n");
		goto fail_irq;
	}
	dev.irq = LF1000_DMA_IRQ;

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	dev.pdev = pdev;

	return 0;

fail_irq:
	iounmap(dev.iomem);
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);

	return ret;
}

static struct platform_driver lf1000_dma_drv = {
	.probe = lf1000_dma_probe,
	.remove = lf1000_dma_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver     = {
		.name   = "lf1000-dma",
		.owner  = THIS_MODULE,
	},
};

static void lf1000_dma_cleanup(void)
{
	platform_driver_unregister(&lf1000_dma_drv);
}

static int __init lf1000_dma_init(void)
{
	return platform_driver_register(&lf1000_dma_drv);
}

module_init(lf1000_dma_init);
module_exit(lf1000_dma_cleanup);

MODULE_AUTHOR("Brian Cavagnolo")
MODULE_LICENSE("GPL");
