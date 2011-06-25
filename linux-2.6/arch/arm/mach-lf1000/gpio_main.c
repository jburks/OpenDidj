/* 
 * arch/arm/mach-lf1000/gpio_main.c
 *
 * LF1000 General-Purpose IO (GPIO) Driver 
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/gpio_priv.h>
#include <mach/gpio.h>
#include <mach/common.h>
#include <mach/platform.h>
#include <mach/mem_controller.h>
#include <mach/gpio_hal.h>
#include <linux/lf1000/gpio_ioctl.h>

extern u32 gpio_get_scratch(void);
extern void gpio_set_scratch(unsigned long value);

/***********************
 * device private data *
 ***********************/

struct gpio_device gpio = {
	.mem = NULL,
	.mem_cur = NULL,
	.amem = NULL,
	.irq = -1,
	.touchscreen = 0,
};

/*******************
 * sysfs interface *
 *******************/

#ifdef CONFIG_LF1000_SDRAM_TUNE
/*
 * expose SDRAM timing registers
 */

static ssize_t show_memcfg(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMCFG));
	return sprintf(buf,"MEMCFG = 0x%4.4x\n", tmp);
}

static ssize_t set_memcfg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMCFG));
	return count;
}
static DEVICE_ATTR(memcfg, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memcfg, set_memcfg);

static ssize_t show_memtime0(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMTIME0));
	return sprintf(buf,"MEMTIME0 = 0x%4.4x\n", tmp);
}

static ssize_t set_memtime0(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMTIME0));
	return count;
}
static DEVICE_ATTR(memtime0, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memtime0, set_memtime0);

static ssize_t show_memtime1(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMTIME1));
	return sprintf(buf,"MEMTIME1 = 0x%4.4x\n", tmp);
}

static ssize_t set_memtime1(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMTIME1));
	return count;
}
static DEVICE_ATTR(memtime1, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memtime1, set_memtime1);

static ssize_t show_pad_strength_gpio_a_low(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOAL));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_a_low(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOAL));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_a_low,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_a_low, set_pad_strength_gpio_a_low);


static ssize_t show_pad_strength_gpio_a_high(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOAH));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_a_high(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOAH));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_a_high,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_a_high, set_pad_strength_gpio_a_high);


static ssize_t show_pad_strength_gpio_b_low(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOBL));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_b_low(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOBL));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_b_low,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_b_low, set_pad_strength_gpio_b_low);


static ssize_t show_pad_strength_gpio_b_high(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOBH));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_b_high(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOBH));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_b_high,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_b_high, set_pad_strength_gpio_b_high);


static ssize_t show_pad_strength_gpio_c_low(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOCL));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_c_low(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOCL));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_c_low,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_c_low, set_pad_strength_gpio_c_low);


static ssize_t show_pad_strength_gpio_c_high(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOCH));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_gpio_c_high(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHGPIOCH));
	return count;
}
static DEVICE_ATTR(pad_strength_gpio_c_high,
	S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_gpio_c_high, set_pad_strength_gpio_c_high);

static ssize_t show_pad_strength_bus(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHBUS));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pad_strength_bus(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIOCURRENT_BASE + GPIOPADSTRENGTHBUS));
	return count;
}
static DEVICE_ATTR(pad_strength_bus, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pad_strength_bus, set_pad_strength_bus);

static ssize_t show_pullup_enable_gpio_a(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIO_BASE + GPIOAPUENB));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pullup_enable_gpio_a(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIO_BASE + GPIOAPUENB));
	return count;
}
static DEVICE_ATTR(pullup_enable_gpio_a, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pullup_enable_gpio_a, set_pullup_enable_gpio_a);

static ssize_t show_pullup_enable_gpio_b(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIO_BASE + GPIOBPUENB));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pullup_enable_gpio_b(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIO_BASE + GPIOBPUENB));
	return count;
}
static DEVICE_ATTR(pullup_enable_gpio_b, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pullup_enable_gpio_b, set_pullup_enable_gpio_b);

static ssize_t show_pullup_enable_gpio_c(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readl(IO_ADDRESS(LF1000_GPIO_BASE + GPIOCPUENB));
	return sprintf(buf,"0x%8.8x\n", tmp);
}

static ssize_t set_pullup_enable_gpio_c(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writel(value, IO_ADDRESS(LF1000_GPIO_BASE + GPIOCPUENB));
	return count;
}
static DEVICE_ATTR(pullup_enable_gpio_c, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_pullup_enable_gpio_c, set_pullup_enable_gpio_c);

static ssize_t show_memrefresh(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMREFRESH));
	return sprintf(buf,"MEMREFRESH = 0x%4.4x\n", tmp);
}

static ssize_t set_memrefresh(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMREFRESH));
	return count;
}
static DEVICE_ATTR(memrefresh, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memrefresh, set_memrefresh);

static ssize_t show_memcontrol(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMCONTROL));
	return sprintf(buf,"MEMCONTROL = 0x%4.4x\n", tmp);
}

static ssize_t set_memcontrol(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMCONTROL));
	return count;
}
static DEVICE_ATTR(memcontrol, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memcontrol, set_memcontrol);

static ssize_t show_memclkdelay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMCLKDELAY));
	return sprintf(buf,"MEMCLKDELAY = 0x%4.4x\n", tmp);
}

static ssize_t set_memclkdelay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMCLKDELAY));
	return count;
}
static DEVICE_ATTR(memclkdelay, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memclkdelay, set_memclkdelay);

static ssize_t show_memdqsoutdelay(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMDQSOUTDELAY));
	return sprintf(buf,"MEMDQSOUTDELAY = 0x%4.4x\n", tmp);
}

static ssize_t set_memdqsoutdelay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMDQSOUTDELAY));
	return count;
}
static DEVICE_ATTR(memdqsoutdelay, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
	show_memdqsoutdelay, set_memdqsoutdelay);

static ssize_t show_memdqsindelay(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tmp;

	tmp = readw(IO_ADDRESS(LF1000_MCU_Y_BASE + LF1000_MEMDQSINDELAY));
	return sprintf(buf,"MEMDQSINDELAY = 0x%4.4x\n", tmp);
}

static ssize_t set_memdqsindelay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	writew(value, IO_ADDRESS(LF1000_MCU_Y_BASE + MEMDQSINDELAY));
	return count;
}
static DEVICE_ATTR(memdqsindelay, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_memdqsindelay, set_memdqsindelay);
#endif
 
static ssize_t show_scratchpad(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%08X\n", (unsigned int)gpio_get_scratch());
}

static ssize_t set_scratchpad(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int x;

	if(sscanf(buf, "%x", &x) != 1)
		return -EINVAL;
	gpio_set_scratch (x);
	return count;		// read all chars
}
static DEVICE_ATTR(scratchpad, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, 
		   show_scratchpad, set_scratchpad);

static ssize_t show_board_id(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	// Don't change this format: packages/mfgdata and libMfgData.cpp
	// depend on parsing this
	return sprintf(buf, "%X\n", gpio_get_board_config());
}
static DEVICE_ATTR(board_id, S_IRUSR|S_IRGRP|S_IROTH, show_board_id, NULL);

static ssize_t show_touchscreen(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", (gpio.touchscreen) ? "YES" : "NO");
}
static DEVICE_ATTR(touchscreen, S_IRUSR|S_IRGRP|S_IROTH, show_touchscreen, NULL);

static ssize_t show_power(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
	(gpio_get_power_config() == SCRATCH_POWER_WARMBOOT) ? "WARM" : 
	 (gpio_get_power_config() == SCRATCH_POWER_COLDBOOT) ? "COLD" : 
		       "FIRST");
}
static DEVICE_ATTR(power, S_IRUSR|S_IRGRP|S_IROTH, show_power, NULL);

static ssize_t show_shutdown(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
		(gpio_get_shutdown_config() == SCRATCH_SHUTDOWN_DIRTY) ?
			"DIRTY" : "CLEAN");
}

static ssize_t set_shutdown(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!strcasecmp(buf, "DIRTY\n"))
		gpio_set_shutdown_config(SCRATCH_SHUTDOWN_DIRTY);
	else if (!strcasecmp(buf, "CLEAN\n"))
		gpio_set_shutdown_config(SCRATCH_SHUTDOWN_CLEAN);
	else
		return -EINVAL;	// invalid string
	
	return count;		// read all chars
}
static DEVICE_ATTR(shutdown, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_shutdown, set_shutdown);

static ssize_t show_request(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	switch(gpio_get_request_config()) {
	case SCRATCH_REQUEST_PLAY:       return(sprintf(buf, "PLAY\n"));
	case SCRATCH_REQUEST_RETURN:     return(sprintf(buf, "RETURN\n"));
	case SCRATCH_REQUEST_UPDATE:     return(sprintf(buf, "UPDATE\n"));
	case SCRATCH_REQUEST_BATTERY:    return(sprintf(buf, "BATTERY\n"));
	case SCRATCH_REQUEST_UNCLEAN:    return(sprintf(buf, "UNCLEAN\n"));
	case SCRATCH_REQUEST_FAILED:     return(sprintf(buf, "FAILED\n"));
	case SCRATCH_REQUEST_SHORT:      return(sprintf(buf, "SHORT\n"));
	case SCRATCH_REQUEST_TRAPDOOR:   return(sprintf(buf, "TRAPDOOR\n"));
	}
	return(sprintf(buf, "UNKNOWN\n"));  // unexpected
}

static ssize_t set_request(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!strcasecmp(buf, "PLAY\n"))
		gpio_set_request_config(SCRATCH_REQUEST_PLAY);
	else if (!strcasecmp(buf, "RETURN\n"))
		gpio_set_request_config(SCRATCH_REQUEST_RETURN);
	else if (!strcasecmp(buf, "UPDATE\n"))
		gpio_set_request_config(SCRATCH_REQUEST_UPDATE);
	else if (!strcasecmp(buf, "BATTERY\n"))
		gpio_set_request_config(SCRATCH_REQUEST_BATTERY);
	else if (!strcasecmp(buf, "UNCLEAN\n"))
		gpio_set_request_config(SCRATCH_REQUEST_UNCLEAN);
	else if (!strcasecmp(buf, "FAILED\n"))
		gpio_set_request_config(SCRATCH_REQUEST_FAILED);
	else if (!strcasecmp(buf, "SHORT\n"))
		gpio_set_request_config(SCRATCH_REQUEST_SHORT);
	else if (!strcasecmp(buf, "TRAPDOOR\n"))
		gpio_set_request_config(SCRATCH_REQUEST_TRAPDOOR);
	else
		return -EINVAL;	// invalid string
	return (count);		// read all chars
}
static DEVICE_ATTR(request, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_request, set_request);

static ssize_t show_boot_image(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	switch(gpio_get_boot_image_config()) {
	case SCRATCH_BOOT_IMAGE_RECOVERY: return(sprintf(buf, "RECOVERY\n"));
	case SCRATCH_BOOT_IMAGE_PLAY:     return(sprintf(buf, "PLAY\n"));
	case SCRATCH_BOOT_IMAGE_2:        return(sprintf(buf, "IMAGE_2\n"));
	case SCRATCH_BOOT_IMAGE_3:        return(sprintf(buf, "IMAGE_3\n"));
	}
	return(sprintf(buf, "UNKNOWN\n"));  // unexpected
}

static ssize_t set_boot_image(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!strcasecmp(buf, "RECOVERY\n"))
		gpio_set_boot_image_config(SCRATCH_BOOT_IMAGE_RECOVERY);
	else if (!strcasecmp(buf, "PLAY\n"))
		gpio_set_boot_image_config(SCRATCH_BOOT_IMAGE_PLAY);
	else if (!strcasecmp(buf, "IMAGE_2\n"))
		gpio_set_boot_image_config(SCRATCH_BOOT_IMAGE_2);
	else if (!strcasecmp(buf, "IMAGE_3\n"))
		gpio_set_boot_image_config(SCRATCH_BOOT_IMAGE_3);
	else
		return -EINVAL;	// invalid string
	return (count);		// read all chars
}
static DEVICE_ATTR(boot_image, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_boot_image, set_boot_image);

static ssize_t show_boot_source(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	switch(gpio_get_boot_source_config()) {
	case SCRATCH_BOOT_SOURCE_NOR:  return(sprintf(buf, "NOR\n"));
	case SCRATCH_BOOT_SOURCE_NAND: return(sprintf(buf, "NAND\n"));
	case SCRATCH_BOOT_SOURCE_UART: return(sprintf(buf, "UART\n"));
	case SCRATCH_BOOT_SOURCE_USB:  return(sprintf(buf, "USB\n"));
	case SCRATCH_BOOT_SOURCE_UNKNOWN:
		return(sprintf(buf, "UNKNOWN\n"));
	}
	return(sprintf(buf, "UNKNOWN\n"));  // unexpected
}

static DEVICE_ATTR(boot_source, S_IRUSR|S_IRGRP|S_IROTH, show_boot_source, NULL);

static ssize_t show_panic(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf (buf, "%d\n", gpio_get_panic_config());
}

static ssize_t set_panic(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;

	if(sscanf(buf, "%d", &value) != 1)
		return -EINVAL;
	gpio_set_panic_config(value);
	return count;		// read all chars
}
static DEVICE_ATTR(panic, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_panic, set_panic);

static ssize_t show_user_0(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	u32 val = gpio_get_user_0_config();
	return(sprintf(buf, "%X\n", val));
}

static ssize_t set_user_0(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int x;

	if(sscanf(buf, "%X", &x) != 1)
		return -EINVAL;
	gpio_set_user_0_config (x);
	return (count);		// read all chars
}
static DEVICE_ATTR(user_0, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_user_0, set_user_0);

#ifdef CONFIG_LF1000_GPIO_DEBUG
static int get_port(u8 port, char *buf)
{
	int len = 0;
	int reg = port*0x40;
	char x = 'A'+port;
	void *base = gpio.mem;

	len += sprintf(buf+len,"GPIO%cOUT      = 0x%08X\n", x,
				   ioread32(base+GPIOAOUT+reg));
	len += sprintf(buf+len,"GPIO%cOUTENB   = 0x%08X\n", x,
				   ioread32(base+GPIOAOUTENB+reg));
	len += sprintf(buf+len,"GPIO%cDETMODE0 = 0x%08X\n", x,
				   ioread32(base+GPIOADETMODE0+reg));
	len += sprintf(buf+len,"GPIO%cDETMODE1 = 0x%08X\n", x,
				   ioread32(base+GPIOADETMODE1+reg));
	len += sprintf(buf+len,"GPIO%cINTENB   = 0x%08X\n", x,
				   ioread32(base+GPIOAINTENB+reg));
	len += sprintf(buf+len,"GPIO%cDET      = 0x%08X\n", x,
				   ioread32(base+GPIOADET+reg));
	len += sprintf(buf+len,"GPIO%cPAD      = 0x%08X\n", x,
				   ioread32(base+GPIOAPAD+reg));
	len += sprintf(buf+len,"GPIO%cPUENB    = 0x%08X\n", x,
				   ioread32(base+GPIOAPUENB+reg));
	len += sprintf(buf+len,"GPIO%cALTFN0   = 0x%08X\n", x,
				   ioread32(base+GPIOAALTFN0+reg));
	len += sprintf(buf+len,"GPIO%cALTFN1   = 0x%08X\n", x,
				   ioread32(base+GPIOAALTFN1+reg));

	return len;
}

static ssize_t show_portA(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return get_port(0, buf);
}
static DEVICE_ATTR(port_A, S_IRUSR|S_IRGRP|S_IROTH, show_portA, NULL);

static ssize_t show_portB(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return get_port(1, buf);
}
static DEVICE_ATTR(port_B, S_IRUSR|S_IRGRP|S_IROTH, show_portB, NULL);

static ssize_t show_portC(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return get_port(2, buf);
}
static DEVICE_ATTR(port_C, S_IRUSR|S_IRGRP|S_IROTH, show_portC, NULL);

static ssize_t show_alive(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int len = 0;

	len += sprintf(buf+len, "ALIVEPWRGATEREG     = 0x%08X\n",
			ioread32(gpio.amem+ALIVEPWRGATEREG));
	len += sprintf(buf+len, "ALIVEGPIORSTREG     = 0x%08X\n",
			ioread32(gpio.amem+ALIVEGPIORSTREG));
	len += sprintf(buf+len, "ALIVEGPIOSETREG     = 0x%08X\n",
			ioread32(gpio.amem+ALIVEGPIOSETREG));
	len += sprintf(buf+len, "ALIVEGPIOREADREG    = 0x%08X\n",
			ioread32(gpio.amem+ALIVEGPIOREADREG));
	len += sprintf(buf+len, "ALIVESCRATCHRSTREG  = 0x%08X\n",
			ioread32(gpio.amem+ALIVESCRATCHRSTREG));
	len += sprintf(buf+len, "ALIVESCRATCHSETREG  = 0x%08X\n",
			ioread32(gpio.amem+ALIVESCRATCHSETREG));
	len += sprintf(buf+len, "ALIVESCRATCHREADREG = 0x%08X\n",
			ioread32(gpio.amem+ALIVESCRATCHREADREG));
	return len;
}
static DEVICE_ATTR(port_alive, S_IRUSR|S_IRGRP|S_IROTH, show_alive, NULL);
#endif /* CONFIG_LF1000_GPIO_DEBUG */

static struct attribute *gpio_attributes[] = {
#ifdef CONFIG_LF1000_SDRAM_TUNE
	&dev_attr_memcfg.attr,
	&dev_attr_pad_strength_gpio_a_low.attr,
	&dev_attr_pad_strength_gpio_a_high.attr,
	&dev_attr_pad_strength_gpio_b_low.attr,
	&dev_attr_pad_strength_gpio_b_high.attr,
	&dev_attr_pad_strength_gpio_c_low.attr,
	&dev_attr_pad_strength_gpio_c_high.attr,
	&dev_attr_pad_strength_bus.attr,
	&dev_attr_memtime0.attr,
	&dev_attr_memtime1.attr,
	&dev_attr_pullup_enable_gpio_a.attr,
	&dev_attr_pullup_enable_gpio_b.attr,
	&dev_attr_pullup_enable_gpio_c.attr,
	&dev_attr_memrefresh.attr,
	&dev_attr_memcontrol.attr,
	&dev_attr_memclkdelay.attr,
	&dev_attr_memdqsoutdelay.attr,
	&dev_attr_memdqsindelay.attr,
#endif
	&dev_attr_board_id.attr,
	&dev_attr_touchscreen.attr,
	&dev_attr_shutdown.attr,
	&dev_attr_request.attr,
	&dev_attr_boot_image.attr,
	&dev_attr_boot_source.attr,
	&dev_attr_power.attr,
	&dev_attr_panic.attr,
	&dev_attr_user_0.attr,
#ifdef CONFIG_LF1000_GPIO_DEBUG
	&dev_attr_port_A.attr,
	&dev_attr_port_B.attr,
	&dev_attr_port_C.attr,
#endif /* CONFIG_LF1000_GPIO_DEBUG */
	NULL
};

static struct attribute_group gpio_attr_group = {
	.attrs = gpio_attributes
};

#ifdef CONFIG_LF1000_STRESS_TEST

static enum gpio_port accy_port, cart_port, power_port, usb_port;
static enum gpio_pin accy_pin, cart_pin, power_pin, usb_pin;

static ssize_t show_port(struct device *dev, struct device_attribute *attr,
			char *buf, const enum gpio_port port)
{
	char port_name = 'A' + port - GPIO_PORT_A;
	return(sprintf(buf, "PORT %c\n", port_name));
}

static ssize_t set_port(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count, enum gpio_port *port)
{
	unsigned char value;

	if(sscanf(buf, "%c", &value) != 1)
		return -EINVAL;

	value = toupper(value);
	value = GPIO_PORT_A + value - 'A';

	if(value > (unsigned char)GPIO_PORT_ALV)
		return -EINVAL;

	*port = value;

	return count;
}

static ssize_t show_pin(struct device *dev, struct device_attribute *attr,
			char *buf, const enum gpio_pin pin)
{
	return(sprintf(buf, "%d\n", (int)pin));
}

static ssize_t set_pin(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count, enum gpio_pin *pin)
{
	int ret, value;

	ret = get_option(&buf, &value);

	if(ret != 1)
		return -EINVAL;

	if((value < (int)GPIO_PIN0) || (value > (int)GPIO_PIN31))
		return -EINVAL;

	*pin = value;

	return count;
}

static ssize_t show_accy_port(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_port(dev, attr, buf, accy_port);
}

static ssize_t set_accy_port(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_port(dev, attr, buf, count, &accy_port);
}

static DEVICE_ATTR(accy_port, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_accy_port, set_accy_port);

static ssize_t show_accy_pin(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_pin(dev, attr, buf, accy_pin);
}

static ssize_t set_accy_pin(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_pin(dev, attr, buf, count, &accy_pin);
}

static DEVICE_ATTR(accy_pin, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_accy_pin, set_accy_pin);

static ssize_t show_cart_port(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_port(dev, attr, buf, cart_port);
}

static ssize_t set_cart_port(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_port(dev, attr, buf, count, &cart_port);
}

static DEVICE_ATTR(cart_port, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_cart_port, set_cart_port);

static ssize_t show_cart_pin(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_pin(dev, attr, buf, cart_pin);
}

static ssize_t set_cart_pin(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_pin(dev, attr, buf, count, &cart_pin);
}

static DEVICE_ATTR(cart_pin, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_cart_pin, set_cart_pin);

static ssize_t show_power_port(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_port(dev, attr, buf, power_port);
}

static ssize_t set_power_port(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_port(dev, attr, buf, count, &power_port);
}

static DEVICE_ATTR(power_port, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_power_port, set_power_port);

static ssize_t show_power_pin(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_pin(dev, attr, buf, power_pin);
}

static ssize_t set_power_pin(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_pin(dev, attr, buf, count, &power_pin);
}

static DEVICE_ATTR(power_pin, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_power_pin, set_power_pin);

static ssize_t show_usb_port(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_port(dev, attr, buf, usb_port);
}

static ssize_t set_usb_port(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_port(dev, attr, buf, count, &usb_port);
}

static DEVICE_ATTR(usb_port, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_usb_port, set_usb_port);

static ssize_t show_usb_pin(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return show_pin(dev, attr, buf, usb_pin);
}

static ssize_t set_usb_pin(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return set_pin(dev, attr, buf, count, &usb_pin);
}

static DEVICE_ATTR(usb_pin, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
			show_usb_pin, set_usb_pin);

static struct attribute *stress_attributes[] = {
	&dev_attr_accy_port.attr,
	&dev_attr_accy_pin.attr,
	&dev_attr_cart_port.attr,
	&dev_attr_cart_pin.attr,
	&dev_attr_power_port.attr,
	&dev_attr_power_pin.attr,
	&dev_attr_usb_port.attr,
	&dev_attr_usb_pin.attr,
	NULL
};

static struct attribute_group stress_attr_group = {
	.name = "stress",
	.attrs = stress_attributes
};

/*
 * In order to make power cut as fast as possible, pre-configure trigger pin
 * so all that must be done at cut-time is toggle the output value.
 */
void stress_config_power(void)
{
	gpio_set_pu(power_port, power_pin, 0);
	gpio_set_val(power_port, power_pin, 0);
	gpio_set_out_en(power_port, power_pin, 1);
	gpio_set_fn(power_port, power_pin, GPIO_GPIOFN);
}

EXPORT_SYMBOL(stress_config_power);

void stress_cut_power(void)
{
	//gpio_set_val(GPIO_PORT_ALV, VDDPWRONSET, 0);
	gpio_set_val(power_port, power_pin, 1);
}

EXPORT_SYMBOL(stress_cut_power);

void stress_cut_cart(int cut)
{
	gpio_configure_pin(cart_port, cart_pin, 
		GPIO_GPIOFN, 1, 0, cut);
}

EXPORT_SYMBOL(stress_cut_cart);

#endif	/* CONFIG_LF1000_STRESS_TEST */

static struct attribute *alive_attributes[] = {
#ifdef CONFIG_LF1000_GPIO_DEBUG
	&dev_attr_port_alive.attr,
#endif /* CONFIG_LF1000_GPIO_DEBUG */
	&dev_attr_scratchpad.attr,
	NULL,
};

static struct attribute_group alive_attr_group = {
	.attrs = alive_attributes
};

/*******************************
 * character device operations *
 *******************************/

int gpio_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	int retval = 0;
	void __user *argp = (void __user *)arg;
	union gpio_cmd c;

	switch(cmd) {
		case GPIO_IOCSOUTVAL:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, 
				sizeof(struct outvalue_cmd)))
			return -EFAULT;
		retval = gpio_set_val(c.outvalue.port, c.outvalue.pin, 
					c.outvalue.value);
		if(retval)
			retval = -EFAULT;
		break;

		case GPIO_IOCSOUTENB:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct outenb_cmd)))
			return -EFAULT;
		retval = gpio_set_out_en(c.outenb.port, c.outenb.pin, 
				c.outenb.value);
		if(retval)
			retval = -EFAULT;
		break;

		case GPIO_IOCXINVAL:
		if(copy_from_user((void *)&c, argp, sizeof(struct invalue_cmd)))
			return -EFAULT;
		c.invalue.value = gpio_get_val(c.invalue.port, c.invalue.pin);
		if(copy_to_user(argp, (void *)&c, sizeof(struct invalue_cmd)))
			return -EFAULT;
		retval = 0;
		break;

		case GPIO_IOCSFUNC:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = gpio_set_fn(c.func.port, c.func.pin, c.func.func);
		if(retval)
			retval = -EFAULT;
		retval = 0;
		break;

		case GPIO_IOCXFUNC:
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = gpio_get_fn(c.func.port, c.func.pin);
		if(retval < 0)
			return -EFAULT;
		c.func.func = retval;
		if(copy_to_user(argp, (void *)&c, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = 0;
		break;

		case GPIO_IOCSDRIVE:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		gpio_set_cur(c.func.port, c.func.pin, c.func.func);
		retval = 0;
		break;

		case GPIO_IOCXDRIVE:
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = gpio_get_cur(c.func.port, c.func.pin);
		if(retval < 0)
			return -EFAULT;
		c.func.func = retval;
		if(copy_to_user(argp, (void *)&c, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = 0;
		break;

		case GPIO_IOCSPULLUP:
		if(!(_IOC_DIR(cmd) & _IOC_WRITE))
			return -EFAULT;
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		gpio_set_pu(c.func.port, c.func.pin, c.func.func);
		retval = 0;
		break;

		case GPIO_IOCXPULLUP:
		if(copy_from_user((void *)&c, argp, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = gpio_get_pu(c.func.port, c.func.pin);
		if(retval < 0)
			return -EFAULT;
		c.func.func = retval;
		if(copy_to_user(argp, (void *)&c, sizeof(struct func_cmd)))
			return -EFAULT;
		retval = 0;
		break;

		default: /* unknown command */
		return -ENOTTY;
	}
	return retval;
}

struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.ioctl = gpio_ioctl,
};

/*************************
 * interrupt hanlding    *
 *************************/
static irqreturn_t gpio_irq(int irq, void *dev_id)
{
	enum gpio_pin pin;
	enum gpio_port port;
	unsigned int pins;
	irqreturn_t ret = IRQ_NONE;
	unsigned long flags;
	struct gpio_handler *gh;

	/* Scan through all of the pins.  When you find the source, invoke the
	 * handler.  Return after handling one interrupt.  If others are
	 * pending, we'll be invoked again. 
	 */
	for( port = GPIO_PORT_A; port <= GPIO_PORT_C; port++) {
		pins = gpio_get_pend32(port);
		for( pin = GPIO_PIN0; pin <= GPIO_PIN31; pin++) {
			if(!gpio_get_int(port, pin))
				continue;
			if(pins & (0x1<<pin)) {
				spin_lock_irqsave(&gpio_handlers_lock, flags);
				gh = &gpio_handlers[port][pin];
				if(!gh->handler.handler) {
					/* Avoid spurious interrupts */
					gpio_clear_pend(port, pin);
					ret = IRQ_HANDLED;
				} else if(gh->mode_gpio) {
					ret = gh->handler.gpio_handler(port, 
								 pin, 
								 gh->priv);
				} else {
					ret = gh->handler.normal_handler(irq, 
								   gh->priv);
				}
				spin_unlock_irqrestore(&gpio_handlers_lock,
						       flags);
				break;
			}
		}
	}
	return ret;
}

/*************************
 * device initialization *
 *************************/

static int lf1000_alvgpio_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	sysfs_remove_group(&pdev->dev.kobj, &alive_attr_group);

	if(gpio.amem != NULL) {
		iounmap(gpio.amem);
		release_mem_region(res->start, (res->end - res->start) + 1);
	}
	return 0;
}

static int lf1000_gpio_remove(struct platform_device *pdev)
{
	struct resource *res;

#ifdef CONFIG_LF1000_STRESS_TEST
	sysfs_remove_group(&pdev->dev.kobj, &stress_attr_group);
#endif

	sysfs_remove_group(&pdev->dev.kobj, &gpio_attr_group);

	if(gpio.irq != -1) {
		free_irq(gpio.irq, NULL);
		gpio.irq = -1;
	}

	cdev_del(&gpio.cdev);

	if(gpio.mem_cur != NULL) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		iounmap(gpio.mem_cur);
		release_mem_region(res->start, (res->end - res->start) + 1);
	}

	if(gpio.mem != NULL) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		iounmap(gpio.mem);
		release_mem_region(res->start, (res->end - res->start) + 1);
	}

	return 0;
}

/*
 * Check for touchscreen
 *
 *	Emerald has GPIOs TOUCHSCREEN_X1 and TOUCHSCREEN_X2 connected to one
 *	sheet and GPIOs TOUCHSCREEN_Y1 and TOUCHSCREEN_Y2 on the other sheet.
 *	Toggle one GPIO and see if the other GPIO in the pair follows.
 *
 *	On Didj, TOUCHSCREEN GPIOs double as switch inputs SW2 5..8 on
 *	the ATAP card.
 */

u8 check_for_touchscreen(void)
{
	int gpio_in;

	// wiggle X1, see if X2 follows.  Wiggle Y1, see if Y2 follows
	// gpio_configure_pin(PORT, PIN, FUNCTION, OUT=1, PULLUP=1, VALUE)
	// ensure X2, Y1, and Y2 are inputs with pullups.
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2), GPIO_GPIOFN, 0, 1, 0);
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 0, 1, 0);
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2), GPIO_GPIOFN, 0, 1, 0);
	
	// Set X1 output high
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 1, 0, 1);
	udelay(100);		// give 100 us for values to settle
	// Read X2, then Set X1 back as input
	gpio_in = gpio_get_val(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2));
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 0, 1, 0);
	if (!gpio_in) {		// is X2 low?
		printk(KERN_INFO "%s.%s:%d X2 == low, no touchscreen\n",
			__FILE__, __FUNCTION__, __LINE__);
		return 0;
	}

	// Set X1 output low
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 1, 0, 0);
	udelay(100);		// give 100 us for values to settle
	// Read X2, then Set X1 back as input
	gpio_in = gpio_get_val(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2));
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 0, 1, 0);
	if (gpio_in) {	// X2 high?
		printk(KERN_INFO "%s.%s:%d X2 == high, no touchscreen\n",
			__FILE__, __FUNCTION__, __LINE__);
		return 0;
	}

	// Set Y1 as output high
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 1, 0, 1);
	udelay(100);		// give 100 us for values to settle
	// Read Y2, then Set Y1 back as input
	gpio_in = gpio_get_val(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2));
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 0, 1, 0);
	if (!gpio_in) {	// Y2 low?
		printk(KERN_INFO "%s.%s:%d Y2 == low, no touchscreen\n",
			__FILE__, __FUNCTION__, __LINE__);
		return 0;
	}

	// Set Y1 output low
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 1, 0, 0);
	udelay(100);		// give 100 us for values to settle
	// Read Y2, then Set Y1 back as input
	gpio_in = gpio_get_val(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2));
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 0, 1, 0);
	if (gpio_in) {	// Y2 high?
		printk(KERN_INFO "%s.%s:%d Y2 == high, no touchscreen\n",
			__FILE__, __FUNCTION__, __LINE__);
		return 0;
	}

	printk(KERN_INFO "%s.%s:%d found touchscreen\n",
		__FILE__, __FUNCTION__, __LINE__);
	return 1;	// have touchscreen
}


static int lf1000_gpio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct resource *res_cur;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "gpio: failed to get resource\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1, 
			"lf1000_gpio")) {
		printk(KERN_ERR "gpio: failed to get region\n");
		return -EBUSY;
	}

	gpio.mem = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(gpio.mem == NULL) {
		printk(KERN_ERR "gpio: failed to remap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	res_cur = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(!res_cur) {
		printk(KERN_ERR "gpio: failed to get resource\n");
		ret = -ENXIO;
		goto fail_remap;
	}

	if(!request_mem_region(res_cur->start,
			       (res_cur->end - res_cur->start) + 1, 
			       "lf1000_gpio_cur")) {
		printk(KERN_ERR "gpio: failed to get region\n");
		ret = -EBUSY;
		goto fail_remap;
	}

	gpio.mem_cur = ioremap_nocache(res_cur->start,
				       (res_cur->end - res_cur->start) + 1);
	if(gpio.mem_cur == NULL) {
		printk(KERN_ERR "gpio: failed to remap\n");
		ret = -ENOMEM;
		goto fail_remap_cur;
	}

	/* turn off GPIO interrupts */
	gpio_set_int32(GPIO_PORT_A, 0);
	gpio_set_int32(GPIO_PORT_B, 0);
	gpio_set_int32(GPIO_PORT_C, 0);

	gpio.devnum = MKDEV(GPIO_MAJOR, 0);
	cdev_init(&gpio.cdev, &gpio_fops);
	gpio.cdev.owner = THIS_MODULE;
	gpio.cdev.ops = &gpio_fops;
	ret = cdev_add(&gpio.cdev, gpio.devnum, 1);
	if(ret) {
		printk(KERN_ALERT "gpio: failed to get a device\n");
		goto fail_dev;
	}

	gpio.irq = platform_get_irq(pdev, 0);
	if(gpio.irq < 0) {
		printk(KERN_INFO "gpio: failed to get IRQ\n");
		ret = gpio.irq;
		goto fail_irq;
	}
	ret = request_irq(gpio.irq, gpio_irq, IRQF_DISABLED,
			"gpio", NULL);
	if(ret) {
		printk(KERN_ERR "gpio: requesting IRQ failed\n");
		goto fail_irq;
	}

	/* see if unit has touchscreen */
	gpio.touchscreen = check_for_touchscreen();

	/* put board ID in flight recorder */
	printk(KERN_INFO "Reading Board ID =  %d\n", gpio_get_board_config());
	
	sysfs_create_group(&pdev->dev.kobj, &gpio_attr_group);

#ifdef CONFIG_LF1000_STRESS_TEST
	sysfs_create_group(&pdev->dev.kobj, &stress_attr_group);
#endif

	return 0;

fail_irq:
	cdev_del(&gpio.cdev);
fail_dev:
	iounmap(gpio.mem);

fail_remap_cur:
	release_mem_region(res_cur->start, (res_cur->end - res_cur->start) + 1);

fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
	
	return ret;
}

static int lf1000_alvgpio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "alvgpio: failed to get resource\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1, 
			"Alive GPIO")) {
		printk(KERN_ERR "alvgpio: failed to get region\n");
		return -EBUSY;
	}
	gpio.amem = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(!gpio.amem) {
		printk(KERN_ERR "alvgpio: failed to remap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	/*
	 * When ALIVEPWRGATEREG is 1, software can write ALV bits.  Otherwise,
	 * last written values are held in flip flops.  This should be power
	 * down/up function of gpio driver, transparent to the user of this API.
	 * Perhaps in suspend/resume?
	 */

	/* init ALIVE S/R input registers to reset value */
	iowrite32(1 << NPOWERGATING, gpio.amem + ALIVEPWRGATEREG);
	iowrite32(0, gpio.amem + ALIVEGPIOSETREG);
	iowrite32(0, gpio.amem + ALIVEGPIORSTREG);
	iowrite32(0 << NPOWERGATING, gpio.amem + ALIVEPWRGATEREG);
	
	/* clear out all handlers */
	memset(gpio_handlers, 0,
	       sizeof(struct gpio_handler)*(GPIO_PORT_ALV+1)*(GPIO_PIN31+1));

	sysfs_create_group(&pdev->dev.kobj, &alive_attr_group);
	return 0;

fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
	
	return ret;
}

#ifdef CONFIG_PM
static int lf1000_gpio_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int lf1000_gpio_resume(struct platform_device *pdev)
{
	return 0;
}

static int lf1000_alvgpio_suspend(struct platform_device *pdev, 
				  pm_message_t mesg)
{
	return 0;
}

static int lf1000_alvgpio_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define lf1000_gpio_suspend	NULL
#define lf1000_gpio_resume	NULL
#define lf1000_alvgpio_suspend	NULL
#define lf1000_alvgpio_resume	NULL
#endif

static struct platform_driver lf1000_gpio_driver = {
	.probe      = lf1000_gpio_probe,
	.remove     = lf1000_gpio_remove,
	.suspend    = lf1000_gpio_suspend,
	.resume     = lf1000_gpio_resume,
	.driver     = {
		.name   = "lf1000-gpio",
		.owner  = THIS_MODULE,
	},
};

static struct platform_driver lf1000_alvgpio_driver = {
	.probe      = lf1000_alvgpio_probe,
	.remove     = lf1000_alvgpio_remove,
	.suspend    = lf1000_alvgpio_suspend,
	.resume     = lf1000_alvgpio_resume,
	.driver     = {
		.name   = "lf1000-alvgpio",
		.owner  = THIS_MODULE,
	},
};

static int __init gpio_init(void)
{
	int ret = platform_driver_register(&lf1000_alvgpio_driver);
	if(ret != 0)
		return ret;
	return platform_driver_register(&lf1000_gpio_driver);
}

static void __exit gpio_cleanup(void)
{
	platform_driver_unregister(&lf1000_gpio_driver);
	platform_driver_unregister(&lf1000_alvgpio_driver);
}

module_init(gpio_init);
module_exit(gpio_cleanup);
MODULE_AUTHOR("Andrey Yurovsky");
MODULE_VERSION("1:2.0");
MODULE_LICENSE("GPL");
