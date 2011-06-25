/* 
 * drivers/lf1000/dpc/main.c
 *
 * LF1000 Display Controller (DPC) Driver 
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * Note: this driver is deprecated.  Do not add any functionality to it.
 *  - DPC support has been moved to arch/arm/mach-lf1000/screen.c
 *  - TODO: move backlight stuff to a backlight driver,
 *          ie: drivers/video/backlight, and clean it up
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/lf1000/dpc_ioctl.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/platform.h>
#include <mach/common.h>
#include <mach/pwm.h>
#include <mach/gpio.h>

#include "dpc_hal.h"
#include "dpc.h"
#include "dpc_config.h"

/* backlight logical to virtual mapping */
int backlight_l2v[] =
 {BACKLIGHT_LEVEL_0, BACKLIGHT_LEVEL_1, BACKLIGHT_LEVEL_2, BACKLIGHT_LEVEL_3};

#define BACKLIGHT_L2V_INDEX_MAX  ( sizeof(backlight_l2v) / sizeof(int)  - 1 )

/* device private data */
struct dpc_device dpc = {
	.mem = NULL,
	.cdev = NULL,
	.dev = 0,
	.major = DPC_MAJOR,
	.backlight = 0,
	.backlight_threshold = 0,  // min PWM value that turns LEDs on
	.backlight_logical =  1,   // initial setting, to match bootstrap */
	.backlight_next = 1,	/* next button direction -1=down, 1=up */
	.pdev = NULL,
};

// forward declarations used in sysfs
int getBacklightVirt(void);
int setBacklightVirt(int virtValue);
int getBacklight(void);
int setBacklight(unsigned long arg);
int setBacklightNext(void);

/*******************
 * sysfs Interface *
 *******************/
#ifdef CONFIG_LF1000_DPC_DEBUG
static ssize_t show_registers(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t len = 0;

	len += sprintf(buf+len, "DPCHTOTAL   = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCHTOTAL), DPCHTOTAL);
	len += sprintf(buf+len, "DPCHSWIDTH  = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCHSWIDTH), DPCHSWIDTH);
	len += sprintf(buf+len, "DPCHASTART  = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCHASTART), DPCHASTART);
	len += sprintf(buf+len, "DPCHAEND    = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCHAEND), DPCHAEND);
	len += sprintf(buf+len, "DPCVTOTAL   = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVTOTAL), DPCVTOTAL);
	len += sprintf(buf+len, "DPCVSWIDTH  = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVSWIDTH), DPCVSWIDTH);
	len += sprintf(buf+len, "DPCVASTART  = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVASTART), DPCVASTART);
	len += sprintf(buf+len, "DPCVAEND    = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVAEND), DPCVAEND);
	len += sprintf(buf+len, "DPCCTRL0    = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCCTRL0), DPCCTRL0);
	len += sprintf(buf+len, "DPCCTRL1    = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCCTRL1), DPCCTRL1);
	len += sprintf(buf+len, "DPCEVTOTAL  = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVTOTAL), DPCEVTOTAL);
	len += sprintf(buf+len, "DPCEVSWIDTH = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVSWIDTH), DPCEVSWIDTH);
	len += sprintf(buf+len, "DPCEVASTART = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVASTART), DPCEVASTART);	
	len += sprintf(buf+len, "DPCEVAEND   = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVAEND), DPCEVAEND);	
	len += sprintf(buf+len, "DPCCTRL2    = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCCTRL2), DPCCTRL2);
	len += sprintf(buf+len, "DPCVSEOFFSET= 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVSEOFFSET), DPCVSEOFFSET);
	len += sprintf(buf+len, "DPCVSSOFFSET= 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCVSSOFFSET), DPCVSSOFFSET);
	len += sprintf(buf+len, "DPCEVSEOFFSET=0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVSEOFFSET), DPCEVSEOFFSET);
	len += sprintf(buf+len, "DPCEVSSOFFSET=0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCEVSSOFFSET), DPCEVSSOFFSET);
	len += sprintf(buf+len, "DPCDELAY0   = 0x%04X @ 0x%04X\n",
			ioread16(dpc.mem+DPCDELAY0), DPCDELAY0);
	len += sprintf(buf+len, "DPCCLKENB   = 0x%08X @ 0x%04X\n",
			ioread32(dpc.mem+DPCCLKENB), DPCCLKENB);
	len += sprintf(buf+len, "DPCCLKGEN0  = 0x%08X @ 0x%04X\n",
			ioread32(dpc.mem+DPCCLKGEN0), DPCCLKGEN0);
	len += sprintf(buf+len, "DPCCLKGEN1  = 0x%08X @ 0x%04X\n",
			ioread32(dpc.mem+DPCCLKGEN1), DPCCLKGEN1);

	return len;
}
static DEVICE_ATTR(registers, S_IRUGO, show_registers, NULL);
#endif

static ssize_t show_backlight(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	int virtValue = getBacklightVirt();
	return(sprintf(buf, "%d\n", virtValue));
}

static ssize_t set_backlight(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int virtValue;
	if(sscanf(buf, "%i", &virtValue) != 1)
		return -EINVAL;
	if (virtValue < -128 || virtValue > 127)
		return -EINVAL;
	if(0 > setBacklightVirt(virtValue))
	return -EINVAL;
	return(count);
}

static DEVICE_ATTR( backlight, S_IRUGO | S_IWUGO, 
		    show_backlight, set_backlight);

static ssize_t show_backlight_logical(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	return(sprintf(buf, "%d\n", dpc.backlight_logical));
}

static ssize_t set_backlight_logical(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int temp;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	if (temp < 0 || temp > BACKLIGHT_L2V_INDEX_MAX)
		return -EINVAL;
	dpc.backlight_logical = temp;
	if (0 > setBacklightVirt(backlight_l2v[dpc.backlight_logical]))
		return -EINVAL;
	return(count);
}

static DEVICE_ATTR( backlight_logical, S_IRUGO | S_IWUGO, 
		    show_backlight_logical, set_backlight_logical);

static ssize_t show_backlight_next(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	return(sprintf(buf, "%d\n", dpc.backlight_next));
}

static ssize_t set_backlight_next(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	if (0 > setBacklightNext())
		return -EINVAL;
	return(count);
}

static DEVICE_ATTR( backlight_next, S_IRUGO | S_IWUGO, 
		    show_backlight_next, set_backlight_next);

static ssize_t show_physical_backlight(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	int physicalValue = getBacklight();
	return(sprintf(buf, "%d\n", physicalValue));
}

static ssize_t set_physical_backlight(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int physicalValue;
	if(sscanf(buf, "%i", &physicalValue) != 1)
		return -EINVAL;
	if (physicalValue < 0 || physicalValue > 511)
		return -EINVAL;
	if(0 > setBacklight(physicalValue))
		return -EINVAL;
	return(count);
}

static DEVICE_ATTR( physical_backlight, S_IRUGO | S_IWUGO, 
		    show_physical_backlight, set_physical_backlight);

static ssize_t show_backlight_threshold(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	return(sprintf(buf, "%d\n", dpc.backlight_threshold));
}

static ssize_t set_backlight_threshold(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int value, virtValue;
	if(sscanf(buf, "%i", &value) != 1)
		return -EINVAL;
	if (value < 0 || value > 511)
		return -EINVAL;
	// update backlight using new threshold
	virtValue = getBacklightVirt();
	dpc.backlight_threshold = value;
	setBacklightVirt(virtValue);
	return(count);
}

static DEVICE_ATTR( backlight_threshold, S_IRUGO | S_IWUGO, 
		    show_backlight_threshold, set_backlight_threshold);

static struct attribute *dpc_attributes[] = {
#ifdef CONFIG_LF1000_DPC_DEBUG
	&dev_attr_registers.attr,
#endif
	&dev_attr_backlight.attr,
	&dev_attr_backlight_logical.attr,
	&dev_attr_backlight_next.attr,
	&dev_attr_physical_backlight.attr,
	&dev_attr_backlight_threshold.attr,
	NULL
};

static struct attribute_group dpc_attr_group = {
	.attrs = dpc_attributes
};

/********************************************************
 * Set Backlight    					*
 *     arg is between 0 and 511,			*
 *     where 0 is dark and 511 is the brightest		*
 ********************************************************/

int setBacklight(unsigned long arg)
{
	int retval;

	if(arg <= LCD_BACKLIGHT_PERIOD) {
		retval = pwm_set_duty_cycle(LCD_BACKLIGHT, arg);
		if(retval == 0)
			dpc.backlight = arg;
		else
			retval = -EFAULT;
	} else {
		retval = -EFAULT;
	}
	return(retval);
}

/********************
 * Get Backlight    *
 ********************/

int getBacklight(void)
{
	return(dpc.backlight);
}

/****************************************
* Set Backlight, using virtual range	*
*   value is a signed byte with zero in	*
*   the middle of the supported range	*
*   Adjust based on board version	*
****************************************/

int setBacklightVirt(int virtValue)
{
	int physValue;

	// on the Dev board the physical range is [140,511]
	if (gpio_have_gpio_dev())
		physValue = ((virtValue * 373) / 256) + 326 +
			dpc.backlight_threshold;
	// on Didj, and later, the physical range is [0,511]
	else
		physValue = (virtValue * 2) + 256 + dpc.backlight_threshold;
	// clip value to be between 0 and 511
	if (physValue < 0)
		physValue = 0;
	else if (physValue > 511)
		physValue = 511;
	return(setBacklight(physValue));
}

/****************************************
* Get Backlight, using virtual range	*
*   value is a signed byte with zero in *
*   the middle of the supported range	*
*   Adjust based on board version	*
****************************************/

int getBacklightVirt(void)
{
	int physValue = getBacklight();
	int virtValue;

	// on the Dev Board the physical range is [140,511]
	if (gpio_have_gpio_dev()) {
		virtValue = ((physValue - 326 - dpc.backlight_threshold)
				* 256) / 373;
		// fixup fixed point math truncation error
		if (virtValue < 0) virtValue--;
		else if (virtValue > 0) virtValue++;
		else {	// virtValue is zero, two special cases
			if (physValue == 325) virtValue = -1;
			if (physValue == 327) virtValue = 1;
		}
	}
	// on Didj, and later, the physical range is [0,511]
	else
		virtValue = (physValue - 256 - dpc.backlight_threshold) / 2;

	return(virtValue);
}

/*
 * setBacklightNext()
 * Advance backlight in response to the 'brightness' button press
 */

int setBacklightNext(void)
{
	int temp;

	temp = dpc.backlight_logical + dpc.backlight_next; // step up or down

	if (temp < 0) {					// too low, go up
		temp = 1;				// bottom + 1
		dpc.backlight_next = 1;			// going up
	} else if (temp > BACKLIGHT_L2V_INDEX_MAX) {	// too high, go down
		temp = BACKLIGHT_L2V_INDEX_MAX - 1;	// top - 1
		dpc.backlight_next = -1;		// going down
	}

	dpc.backlight_logical = temp;
	return(setBacklightVirt(backlight_l2v[dpc.backlight_logical]));
}
EXPORT_SYMBOL(setBacklightNext);


/********************
 * Character Device *
 ********************/

int dpc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
					  unsigned long arg)
{
	int retval = 0;
	void __user *argp = (void __user *)arg;
	union dpc_cmd c;

	switch(cmd) {
		case DPC_IOCTINTENB:
		dpc_SetIntEnb(arg);
		break;

		case DPC_IOCSHSYNC:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct hsync_cmd)))
			return -EFAULT;
		retval = dpc_SetHSync(c.hsync.avwidth, 
				      c.hsync.hsw,
				      c.hsync.hfp,
				      c.hsync.hbp,
				      c.hsync.inv_hsync);
		break;

		case DPC_IOCSVSYNC:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct vsync_cmd)))
			return -EFAULT;
		retval = dpc_SetVSync(c.vsync.avheight,
				      c.vsync.vsw,
				      c.vsync.vfp,
				      c.vsync.vbp,
				      c.vsync.inv_vsync,
				      c.vsync.eavheight,
				      c.vsync.evsw,
				      c.vsync.evfp,
				      c.vsync.evbp);
		break;

		case DPC_IOCSCLOCK0:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct clock0_cmd)))
			return -EFAULT;
		retval = dpc_SetClock0(c.clock0.source, 
				       c.clock0.div, 
				       c.clock0.delay, 
				       c.clock0.out_inv, 
				       c.clock0.out_en);
		break;

		case DPC_IOCSCLOCK1:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct clock1_cmd)))
			return -EFAULT;
		retval = dpc_SetClock1(c.clock1.source, 
				       c.clock1.div, 
				       c.clock1.delay, 
				       c.clock1.out_inv);
		break;

		case DPC_IOCSMODE:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct mode_cmd)))
			return -EFAULT;
		retval = dpc_SetMode(c.mode.format,
				     c.mode.interlace,
				     c.mode.invert_field,
				     c.mode.rgb_mode,
				     c.mode.swap_rb,
				     c.mode.ycorder,
				     c.mode.clip_yc,
				     c.mode.embedded_sync,
				     c.mode.clock);
		break;

		case DPC_IOCTSWAPRB:
		dpc_SwapRB(arg);
		break;

		case DPC_IOCTCONTRAST:
		//dpc_SetContrast(dpc.pdev->dev.platform_data, arg);
		break;

		case DPC_IOCQCONTRAST:
		//retval = dpc_GetContrast(dpc.pdev->dev.platform_data);
		//retval = (retval < 0) ? 0 : (retval & 0xF);
		retval = 0;
		break;

		case DPC_IOCTBRIGHTNESS:
		//dpc_SetBrightness(dpc.pdev->dev.platform_data, arg);
		break;

		case DPC_IOCQBRIGHTNESS:
		//retval = dpc_GetBrightness(dpc.pdev->dev.platform_data);
		//retval = (retval < 0) ? 0 : (retval & 0xFF);
		retval = 0;
		break;

		case DPC_IOCTBACKLIGHT:
		retval = setBacklight(arg);
		break;

		case DPC_IOCQBACKLIGHT:
		retval = getBacklight();
		break;

		case DPC_IOCTBACKLIGHTVIRT:
		retval = setBacklightVirt(arg);
		break;

		case DPC_IOCQBACKLIGHTVIRT:
		retval = getBacklightVirt();
		copy_to_user(argp, &retval, sizeof(retval));
		retval = 0;
		break;

		default:
		return -ENOTTY;
	}

	return retval;
}

struct file_operations dpc_fops = {
	.owner = THIS_MODULE,
	.ioctl = dpc_ioctl,
};

/********************
 * Module Functions *
 ********************/

static int lf1000_dpc_probe(struct platform_device *pdev)
{
	int ret;
	int i;
	int div;
	struct resource *res;

	printk(KERN_INFO "lf1000-dpc driver\n");

	dpc.pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "dpc: failed to get resource\n");
		return -ENXIO;
	}

	div = lf1000_CalcDivider(get_pll_freq(PLL1), DPC_DESIRED_CLOCK_HZ);
	if(div < 0) {
		printk(KERN_ERR "dpc: failed to get a clock divider!\n");
		return -EFAULT;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1, 
		"lf1000_dpc")) {
		printk(KERN_ERR "dpc: failed to map DPC region.");
		return -EBUSY;
	}

	dpc.mem = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(dpc.mem == NULL) {
		printk(KERN_ERR "dpc: failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	/* configure LCD interface pins */
	for(i = 0; i < NUM_PVD_PINS; i++)
		gpio_configure_pin(pvd_ports[i], pvd_pins[i], GPIO_ALT1, 
				1, 0, 0);
	
	/* set up and enable the DPC only if it wasn't already enabled */
	if (!(ioread16(dpc.mem+DPCCTRL0) & (1<<DPCENB))) {
		printk(KERN_INFO "dpc: DPC not enabled, do config+enable\n");
		dpc_SetClockPClkMode(PCLKMODE_ONLYWHENCPUACCESS);
		dpc_SetClock0(DISPLAY_VID_PRI_VCLK_SOURCE, 
			      div > 0 ? (div-1) : 0, 
			      DISPLAY_VID_PRI_VCLK_DELAY,
			      DISPLAY_VID_PRI_VCLK_INV,	
			      DISPLAY_VID_PRI_VCLK_OUT_ENB);
		dpc_SetClock1(DISPLAY_VID_PRI_VCLK2_SOURCE,
			      DISPLAY_VID_PRI_VCLK2_DIV,
			      0,	/* outclk delay */
			      1);	/* outclk inv */
		dpc_SetClockEnable(1);
		ret = dpc_SetMode(DISPLAY_VID_PRI_OUTPUT_FORMAT,
				  0, 	/* interlace */
				  0, 	/* invert field */
				  1,	/* RGB mode */
				  DISPLAY_VID_PRI_SWAP_RGB,
				  DISPLAY_VID_PRI_OUTORDER,
				  0,	/* clip YC */
				  0,	/* embedded sync */
				  DISPLAY_VID_PRI_PAD_VCLK);
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set display mode\n");
		dpc_SetDither(DITHER_BYPASS, DITHER_BYPASS, DITHER_BYPASS);
		ret = dpc_SetHSync(DISPLAY_VID_PRI_MAX_X_RESOLUTION,
				   DISPLAY_VID_PRI_HSYNC_SWIDTH,
				   DISPLAY_VID_PRI_HSYNC_FRONT_PORCH,
				   DISPLAY_VID_PRI_HSYNC_BACK_PORCH,
				   DISPLAY_VID_PRI_HSYNC_ACTIVEHIGH );
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set HSync\n");
		ret = dpc_SetVSync(DISPLAY_VID_PRI_MAX_Y_RESOLUTION,
				   DISPLAY_VID_PRI_VSYNC_SWIDTH,
				   DISPLAY_VID_PRI_VSYNC_FRONT_PORCH,
				   DISPLAY_VID_PRI_VSYNC_BACK_PORCH,
				   DISPLAY_VID_PRI_VSYNC_ACTIVEHIGH,
				   1, 1, 1, 1);
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set VSync\n");
		dpc_SetDelay(0, 7, 7, 7, 4, 4, 4);
		dpc_SetVSyncOffset(1, 1, 1, 1);
		dpc_SetDPCEnable();
	}

	if (gpio_have_tvout()) {
		/* 2nd DPC register set for TV out */
		dpc.mem += 0x400;
		dpc_SetClockPClkMode(PCLKMODE_ONLYWHENCPUACCESS);
		dpc_SetClock0(VID_VCLK_SOURCE_XTI,
			      0, 	/* vidclk divider */ 
			      0, 	/* vidclk delay */
			      0, 	/* vidclk invert */	
			      DISPLAY_VID_PRI_VCLK_OUT_ENB);
		dpc_SetClock1(VID_VCLK_SOURCE_VCLK2, 
			      1, 	/* vidclk2 divider */
			      0,	/* outclk delay */
			      0); 	/* outclk inv */
		dpc_SetClockEnable(1);
		ret = dpc_SetMode(VID_FORMAT_CCIR601B,
				  1,  	/* interlace */
				  0, 	/* invert field */
				  0, 	/* RGB mode */
				  0, 	/* swap RB */
				  VID_ORDER_CbYCrY, /* YC order */
				  1, 	/* clip YC */
				  0,	/* embedded sync */
				  DISPLAY_VID_PRI_PAD_VCLK);
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set display mode\n");
		dpc_SetDither(DITHER_BYPASS, DITHER_BYPASS, DITHER_BYPASS);
		ret = dpc_SetHSync(720, /* active horizontal */
		  		   33, 	/* sync width */
				   24, 	/* front porch */
			  	   81, 	/* back porch */
			  	   0); 	/* polarity */
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set HSync\n");
		ret = dpc_SetVSync(240, /* active odd field */
			  	   3, 	/* sync width */
			  	   3, 	/* front porch */
			  	   16, 	/* back porch */
			  	   0, 	/* polarity */
			  	   240, /* active even field */
			  	   3, 	/* sync width */
			  	   4, 	/* front porch */
			  	   16); /* back porch */
		if(ret < 0)
			printk(KERN_ALERT "dpc: failed to set VSync\n");
		dpc_SetDelay(0, 4, 4, 4, 4, 4, 4);
		dpc_SetVSyncOffset(0, 0, 0, 0);
	
		/* Internal video encoder for TV out */
		dpc_ResetEncoder();
		dpc_SetEncoderEnable(1);
		dpc_SetEncoderPowerDown(1);
		dpc_SetEncoderMode(0, 1);
		dpc_SetEncoderFSCAdjust(0);
		dpc_SetEncoderBandwidth(0, 0);
		dpc_SetEncoderColor(0, 0, 0, 0, 0);
		dpc_SetEncoderTiming(64, 1716, 0, 3);
		dpc_SetEncoderUpscaler(320, 720);
		dpc_SetEncoderPowerDown(0);
	
		/* 2nd DPC is master when running TV + LCD out */
		dpc_SetDPCEnable();
		dpc_SetClockEnable(1);
	
		/* Switch back to 1st DPC register set for LCD out */
		dpc.mem -= 0x400;
	}
	
	ret = register_chrdev(dpc.major, "dpc", &dpc_fops);
	if(ret < 0) {
		printk(KERN_ALERT "dpc: failed to get a device\n");
		goto fail_dev;
	}
	if(dpc.major == 0) dpc.major = ret;

	dpc.cdev = cdev_alloc();
	dpc.cdev->owner = THIS_MODULE;
	dpc.cdev->ops = &dpc_fops;
	ret = cdev_add(dpc.cdev, 0, 1);
	if(ret < 0) {
		printk(KERN_ALERT "dpc: failed to create character device\n");
		goto fail_add;
	}

#if defined CONFIG_MACH_ME_LF1000 || defined CONFIG_MACH_LF_LF1000
	/* enable backlight control 
	 * XXX: on the LF1000 Development Board v1.1, this causes the backlight
	 * 	to be on 100% */
	gpio_configure_pin(lf1000_l2p_port(LED_ENA),lf1000_l2p_pin(LED_ENA),
		GPIO_GPIOFN, 1, 0, 1);
#endif

	ret = pwm_get_clock_rate();
	if(ret < 1) {
		printk(KERN_ERR "dpc: PWM clock not set up?\n");
		dpc.backlight = 0;
	}
	else { /* set up the backlight */
		printk("dpc: PWM clock rate is %d\n", ret);
		pwm_configure_pin(LCD_BACKLIGHT);
		pwm_set_prescale(LCD_BACKLIGHT, 1);
		pwm_set_polarity(LCD_BACKLIGHT, POL_BYP);
		pwm_set_period(LCD_BACKLIGHT, LCD_BACKLIGHT_PERIOD);
		/* initial backlight setting, try to match bootstrap */
		setBacklightVirt(backlight_l2v[dpc.backlight_logical]);
	}

	/**********************
	 * LCD hardware setup *
	 **********************/

	/* on older boards, make sure LCD is not in reset */
	if(gpio_have_gpio_dev()) {
		gpio_configure_pin(lf1000_l2p_port(LCD_RESET),
		lf1000_l2p_pin(LCD_RESET), GPIO_GPIOFN, 1, 0, LCD_nRESET_LEVEL);
	}

	sysfs_create_group(&pdev->dev.kobj, &dpc_attr_group);

	return 0;

fail_add:
	unregister_chrdev(dpc.major, "dpc");
fail_dev:
	iounmap(dpc.mem);
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
	return ret;
}

static int lf1000_dpc_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	u16 tmp = ioread16(dpc.mem+DPCCTRL0);

	printk(KERN_INFO "dpc: removing...\n");

	dpc.pdev = NULL;

	BIT_CLR(tmp, DPCENB);

	unregister_chrdev(dpc.major, "dpc");
	if(dpc.cdev != NULL)
		cdev_del(dpc.cdev);

	if(dpc.mem != NULL)
		iounmap(dpc.mem);
	release_mem_region(res->start, (res->end - res->start) + 1);

	sysfs_remove_group(&pdev->dev.kobj, &dpc_attr_group);
	return 0;
}

#ifdef CONFIG_PM
static int lf1000_dpc_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int lf1000_dpc_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define lf1000_dpc_suspend	NULL
#define lf1000_dpc_resume	NULL
#endif

static struct platform_driver lf1000_dpc_driver = {
	.probe		= lf1000_dpc_probe,
	.remove		= lf1000_dpc_remove,
	.suspend	= lf1000_dpc_suspend,
	.resume		= lf1000_dpc_resume,
	.driver		= {
		.name = "lf1000-dpc",
		.owner = THIS_MODULE,
	},
};

static int __init dpc_init(void)
{
	return platform_driver_register(&lf1000_dpc_driver);
}

static void __exit dpc_cleanup(void)
{
	platform_driver_unregister(&lf1000_dpc_driver);
}

module_init(dpc_init);
module_exit(dpc_cleanup);
MODULE_AUTHOR("Andrey Yurovsky");
MODULE_VERSION("1:1.1");
MODULE_LICENSE("GPL");
