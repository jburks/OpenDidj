/*
 * drivers/lf1000/dpc/ili9322.c
 *
 * SPI slave driver for ILI9322 LCD chip.
 *
 * Copyright 2009 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include "ili9322.h"

/*
 * Register I/O primitive.
 */
static int spi_reg(struct spi_device *spi, u16 op)
{
	int ret;
	struct spi_message m;
	struct spi_transfer t;

	union {
		u16 txb;
		u8 txbuf[2];
	} tx;
	union {
		u16 rxb;
		u8 rxbuf[2];
	} rx;

	tx.txb = op;

	memset(&t, 0, sizeof(t));
	t.tx_buf = &tx;
	t.rx_buf = &rx;
	t.len = 2;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret  = spi_sync(spi, &m);

	if( ret < 0 )
	{
		return ret;
	}

	return rx.rxb;
}

/*
 * Attribute helpers - public because of DPC ioctls
 */
void dpc_SetContrast(struct spi_device *spi, u8 contrast)
{
	spi_reg(spi, LCD_SET((CMD_CONTRAST|(contrast & 0xF))));
}

int dpc_GetContrast(struct spi_device *spi)
{
	return spi_reg(spi, LCD_GET(CMD_CONTRAST));
}

void dpc_SetBrightness(struct spi_device *spi, u8 brightness)
{
	spi_reg(spi, LCD_SET((CMD_BRIGHTNESS|(brightness & 0xFF))));
}

int dpc_GetBrightness(struct spi_device *spi)
{
	return spi_reg(spi, LCD_GET(CMD_BRIGHTNESS));
}

/*
 * sysfs interface
 */
static ssize_t show_lcdid(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int ret = -ENODEV;
	struct spi_device *spi = (struct spi_device*)dev->platform_data;

	if(spi != NULL)	
		ret = spi_reg(spi, LCD_GET(CMD_CHIPID));

	if(ret < 0)
		return sprintf(buf, "%s", "error: chip ID not read.");
	return sprintf(buf, "0x%0X\n", ret&0xff);
}
static DEVICE_ATTR(lcd_id, S_IRUSR|S_IRGRP, show_lcdid, NULL);

static ssize_t show_brightness(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	struct spi_device *spi = (struct spi_device*)dev->platform_data;
	int value;
	if(spi == NULL)
		return -ENODEV;
	value = dpc_GetBrightness(spi);
	value = (value < 0) ? 0 : (value & 0xFF) - 128;
	return(sprintf(buf, "%d\n", value));
}

static ssize_t set_brightness(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int value;
	struct spi_device *spi = (struct spi_device*)dev->platform_data;
	if(spi == NULL)
		return -ENODEV;
	if(sscanf(buf, "%i", &value) != 1)
		return -EINVAL;
	if (value < -128 || value > 127)
		return -EINVAL;
	// convert virtual [-128,127] to physical [0,255]
	value = value + 128;
	dpc_SetBrightness(spi, value);
	return(count);
}

static DEVICE_ATTR( brightness, S_IRUGO | S_IWUGO,
	       	show_brightness, set_brightness);

static ssize_t show_contrast(struct device *dev, 
			      struct device_attribute *attr,
			      char *buf)
{
	struct spi_device *spi = (struct spi_device*)dev->platform_data;
	int value;

	if(spi == NULL)
		return -ENODEV;

	value = dpc_GetContrast(spi);
	// map physical [0,15] to virtual [-128,127]
	value = (value < 0) ? 0 : ((value & 0xF) << 4) - 128;
	return(sprintf(buf, "%d\n", value));
}

static ssize_t set_contrast(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int value;
	struct spi_device *spi = (struct spi_device*)dev->platform_data;

	if(spi == NULL)
		return -ENODEV;
	if(sscanf(buf, "%i", &value) != 1)
		return -EINVAL;
	if (value < -128 || value > 127)
		return -EINVAL;
	// map virtual [-128,127] to physcial [0,15]
	value = (value + 128) >> 4;
	dpc_SetContrast(spi, value);
	return(count);
}

static DEVICE_ATTR( contrast, S_IRUGO | S_IWUGO,
	       	show_contrast, set_contrast);

static struct attribute *ili9322_attributes[] = {
	&dev_attr_lcd_id.attr,
	&dev_attr_brightness.attr,
	&dev_attr_contrast.attr,
	NULL
};

static struct attribute_group ili9322_attr_group = {
	.attrs = ili9322_attributes
};
#ifdef CONFIG_PM
static int ili9322_suspend(struct spi_device *spi, pm_message_t message)
{
	return 0;
}

static int ili9322_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define ili9322_suspend NULL
#define ili9322_resume NULL
#endif

static int __devinit ili9322_probe(struct spi_device *spi)
{
	int ret;
	struct platform_device *pdev = (struct platform_device *)spi->dev.platform_data;

	/* 
	 * Both the LF1000 DPC and the ili9322 want sysfs entries to control
	 * brightness, contrast, etc.  platform_data of each points to spi
	 */
	pdev->dev.platform_data = spi;
	spi->dev.platform_data = spi;

	spi->bits_per_word = 16;
	spi->master->setup(spi);

	ret = spi_reg(spi, LCD_GET(CMD_CHIPID));
	if(ret < 0)
		printk(KERN_ERR "dpc: failed to read LCD chip ID: %d\n", ret);
	else if((ret&0xff) != LCD_CHIP_ID) 
		printk(KERN_ERR "dpc: expected LCD chip ID 0x%08X, got %08X\n",
			LCD_CHIP_ID, ret&0xff);
	else { /* program LCD preferred register settings */
		/* Power Control : Normal display+HVDE Mode+Line Inversion */
		ret = spi_reg(spi, LCD_SET((CMD_DISPLAY | 0x05)));
		/* VCOM High Voltage : VREG1OUT x 0.87*/
		ret = spi_reg(spi, LCD_SET((CMD_HIGHVOLTAGE | 0x32)));
#if defined (CONFIG_LF1000_DPC_OVERRIDE_VIEWANGLE)
		/* change viewing angle */
		ret = spi_reg(spi, LCD_SET((CMD_VIEWANGLE |
		               CONFIG_LF1000_DPC_OVERRIDE_VIEWABLE_VALUE)));
#endif
		/* VCOM AC Voltage : VREG1OUT x 1.06*/
		ret = spi_reg(spi, LCD_SET((CMD_AMPLITUDE | 0x12)));
		/* Gamma1 : Gamma Curve*/
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA1 | 0xA7)));
		/* Gamma2 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA2 | 0x57)));
		/* Gamma3 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA3 | 0x73)));
		/* Gamma4 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA4 | 0x72)));
		/* Gamma5 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA5 | 0x73)));
		/* Gamma6 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA6 | 0x55)));
		/* Gamma7 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA7 | 0x17)));
		/* Gamma8 : */
		ret = spi_reg(spi, LCD_SET((CMD_GAMMA8 | 0x62)));
	}

	/* pdev points to the LF1000 DPC to group sysfs stuff */

	sysfs_create_group(&pdev->dev.kobj, &ili9322_attr_group);
	sysfs_create_group(&spi->dev.kobj, &ili9322_attr_group);

	return 0;
}

static int __devexit ili9322_remove(struct spi_device *spi)
{
	struct platform_device *pdev = (struct platform_device *)spi->dev.platform_data;

	sysfs_remove_group(&pdev->dev.kobj, &ili9322_attr_group);
	sysfs_remove_group(&spi->dev.kobj, &ili9322_attr_group);
	return 0;
}

static struct spi_driver ili9322_driver = {
	.driver = {
		.name	= "ili9322",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= ili9322_probe,
	.remove		= __devexit_p(ili9322_remove),
	.suspend	= ili9322_suspend,
	.resume		= ili9322_resume,
};

static int __init ili9322_init(void)
{
	return spi_register_driver(&ili9322_driver);
}
module_init(ili9322_init);

static void __exit ili9322_exit(void)
{
	spi_unregister_driver(&ili9322_driver);
}
module_exit(ili9322_exit);

MODULE_DESCRIPTION("ILI9322 LCD SPI Driver");
MODULE_LICENSE("GPL");
