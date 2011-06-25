/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * (C) Copyright 2002 Hewlett-Packard Company
 *
 * USB Bus Glue for LF1000
 *
 * Written by Christopher Hoover <ch@hpl.hp.com>
 * Based on fragments of previous driver by Russell King et al.
 *
 * Modified for S3C2410 from ohci-sa1111.c, ohci-omap.c and ohci-lh7a40.c
 *	by Ben Dooks, <ben@simtec.co.uk>
 *	Copyright (C) 2004 Simtec Electronics
 *
 * Modified for LH7A404 from ohci-sa1111.c
 *  by Durgesh Pattamatta <pattamattad@sharpsec.com>
 *
 * Modified for LF1000 from ohci-s3c2410.c and ohci-lh7a404.c
 *
 * This file is licenced under the GPL.
*/

#include <linux/platform_device.h>
#include <mach/gpio.h>

static void lf1000_start_hc(struct platform_device *dev, struct usb_hcd *hcd)
{
	int pll_divisor;

	/* start VUSB (turn on DC power to devices) */
	gpio_set_fn(GPIO_PORT_A, GPIO_PIN9, GPIO_GPIOFN);
	gpio_set_val(GPIO_PORT_A, GPIO_PIN9, 0);
	gpio_set_out_en(GPIO_PORT_A, GPIO_PIN9, 1);

	/* calculate USB Host 48 Mhz divisor */
	pll_divisor = get_pll_freq(PLL1) / 48000000;

	/* start USB Host clock */
	writew(((pll_divisor-1)<<4) | (PLL1<<1), hcd->regs + 0xC4);
	writew(0x000F, hcd->regs + 0xC0);
	writew(0x0018, hcd->regs + 0x80);
}

static void lf1000_stop_hc(struct platform_device *dev, struct usb_hcd *hcd)
{
	/* stop VUSB (turn off DC power to devices) */
	gpio_set_fn(GPIO_PORT_A, GPIO_PIN9, GPIO_GPIOFN);
	gpio_set_val(GPIO_PORT_A, GPIO_PIN9, 1);
	gpio_set_out_en(GPIO_PORT_A, GPIO_PIN9, 0);

	/* stop USB Host clock */
	writew(0, hcd->regs + 0xC0);
	writew(0, hcd->regs + 0x80);
}

/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/*
 * usb_hcd_lf1000_remove - shutdown processing for HCD
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_lf1000_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
*/

static void
usb_hcd_lf1000_remove (struct usb_hcd *hcd, struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	lf1000_stop_hc(dev, hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
}

/**
 * usb_hcd_lf1000_probe - initialize lf1000-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 */
static int usb_hcd_lf1000_probe (const struct hc_driver *driver,
		struct platform_device *dev)
{
	struct usb_hcd *hcd = NULL;
	int retval;

	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		return -ENOMEM;
	}

	hcd = usb_create_hcd(driver, &dev->dev, "lf1000-usb");
	if (hcd == NULL)
		return -ENOMEM;

	hcd->rsrc_start = dev->resource[0].start;
	hcd->rsrc_len   = dev->resource[0].end - dev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		retval = -EBUSY;
		goto err_put;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&dev->dev, "ioremap failed\n");
		retval = -ENOMEM;
		goto err_mem;
	}
	lf1000_start_hc(dev, hcd);

	ohci_hcd_init(hcd_to_ohci(hcd));

	retval = usb_add_hcd(hcd, dev->resource[1].start, IRQF_DISABLED);

	if (retval == 0)
		return retval;

	lf1000_stop_hc(dev, hcd);

	iounmap(hcd->regs);

err_mem:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);

err_put:
	usb_put_hcd(hcd);
	return retval;
}

/*-------------------------------------------------------------------------*/

static int ohci_lf1000_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run (ohci)) < 0) {
		err("can't start %s", hcd->self.bus_name);
		ohci_stop (hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_lf1000_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"LF1000 OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_lf1000_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

/* device driver */

static int ohci_hcd_lf1000_drv_probe(struct platform_device *pdev)
{
	return usb_hcd_lf1000_probe(&ohci_lf1000_hc_driver, pdev);
}

static int ohci_hcd_lf1000_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_lf1000_remove(hcd, pdev);
	return 0;
}

static struct platform_driver ohci_hcd_lf1000_driver = {
	.probe		= ohci_hcd_lf1000_drv_probe,
	.remove		= ohci_hcd_lf1000_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	/*.suspend	= ohci_hcd_lf1000_drv_suspend, */
	/*.resume	= ohci_hcd_lf1000_drv_resume, */
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "lf1000-ohci",
	},
};

MODULE_ALIAS("platform:lf1000-ohci");
