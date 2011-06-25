/* 
 * drivers/lf1000/pwm/pwm.c
 * 
 * LF1000 Pulse Width Modulator (PWM) Driver
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation. 
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/io.h>

#include <mach/platform.h>
#include <mach/common.h>
#include <mach/pwm.h>
#include <mach/gpio.h>
#include <mach/pwm_hal.h>

#define RESSIZE(res) 		(((res)->end - (res)->start)+1)
#define DRIVER_NAME 		"lf1000-pwm"

/* number of PWM output channels */
#define PWM_NUM_CHANNELS	3
#define PWM_CLK_SRC		PLL1

/* PWM derived clock settings */
#define PWM_CLOCK_HZ		10240000
#define PWM_POLARITY		POL_BYP

static struct pwm_device {
	void __iomem 		*base;
	int 			clock_rate;
} pwm;

/*********************
 * PWM API Functions *
 *********************/

int pwm_configure_pin(enum pwm_chan channel)
{
	enum gpio_port port;
	enum gpio_pin pin;

	switch(channel) {
		case PWM_CHAN0:
		port = GPIO_PORT_A;
		pin = 30;
		break;
		case PWM_CHAN1:
		port = GPIO_PORT_A;
		pin = 31;
		break;
		case PWM_CHAN2:
		port = GPIO_PORT_C;
		pin = 7;
		break;
		default:
		return -1;
		break;
	}
	gpio_set_fn(port, pin, GPIO_ALT1);
	gpio_set_pu(port, pin, 0);
	gpio_set_out_en(port, pin, 1);
	return 0;
}
EXPORT_SYMBOL(pwm_configure_pin);

int pwm_set_prescale(enum pwm_chan channel, u32 prescale)
{
	u16 tmp;
	u8 shift = 0;
	void *reg;

	if(channel >= PWM_CHAN_INVALID || prescale >= 128)
		return -EINVAL;

	if(channel <= 1) { /* 0/1 */
		reg = pwm.base+PWM01PRES;
	}
	else { /* 2/3 */
		reg = pwm.base+PWM23PRES;
		channel /= 2; /* treat 2 like 0, 3 like 1 */
	}

	shift = channel == 0 ? PWM0PRESCALE : PWM1PRESCALE;

	tmp = readw(reg);
	tmp &= ~(0x3F<<shift); /* clear prescaler bits */
	tmp |=  (prescale<<shift); /* set prescaler */
	writew(tmp, reg);
	return 0;
}
EXPORT_SYMBOL(pwm_set_prescale);

int pwm_set_polarity(enum pwm_chan channel, u8 polarity)
{
	void *reg;
	u16 tmp;
	u8 shift = 0;

	if( channel >= PWM_CHAN_INVALID || polarity >= POL_INVALID )
		return -EINVAL;

	if( channel <= 1 ) { /* 0/1 */
		reg = pwm.base+PWM01PRES;
	}
	else { /* 2/3 */
		reg = pwm.base+PWM23PRES;
		channel /= 2; /* treat 2 like 0, 3 like 1 */
	}

	shift = channel == 0 ? PWM0POL : PWM1POL;

	tmp = readw(reg);
	if(polarity == POL_INV)
		tmp &= ~(1<<shift);
	else
		tmp |= (1<<shift);
	return 0;
}
EXPORT_SYMBOL(pwm_set_polarity);

int pwm_set_period(enum pwm_chan channel, u32 period)
{
	void *reg = NULL;

	if(channel >= PWM_CHAN_INVALID || period >= 1024)
		return -EINVAL;

	switch(channel) {
		case PWM_CHAN0:
		reg = pwm.base+PWM0PERIOD;
		break;
		case PWM_CHAN1:
		reg = pwm.base+PWM1PERIOD;
		break;
		case PWM_CHAN2:
		reg = pwm.base+PWM2PERIOD;
		break;
		default:
		return -1;
		break;
	}

	if(reg == NULL)
		return -EINVAL;

	writew(period,reg);
	return 0;

}
EXPORT_SYMBOL(pwm_set_period);

int pwm_set_duty_cycle(enum pwm_chan channel, u32 duty)
{
	void *reg = NULL;

	if(channel >= PWM_CHAN_INVALID || duty >= 1024)
		return -EINVAL;

	switch(channel) {
		case PWM_CHAN0:
		reg = pwm.base+PWM0DUTY;
		break;
		case PWM_CHAN1:
		reg = pwm.base+PWM1DUTY;
		break;
		case PWM_CHAN2:
		reg = pwm.base+PWM2DUTY;
		break;
		default:
		return -1;
		break;
	}

	if(reg == NULL)
		return -EINVAL;

	writew(duty,reg);
	return 0;
}
EXPORT_SYMBOL(pwm_set_duty_cycle);

int pwm_set_clock(u8 source, u8 div, u8 mode, u8 enable)
{
	u32 tmp;

	if(source >= PWM_CLK_INVALID || div > 63)
		return -EINVAL;

	writew((u16)((div<<PWMCLKDIV)|(source<<PWMCLKSRCSEL)),
		  pwm.base+PWMCLKGEN);

	tmp = ioread32(pwm.base+PWMCLKENB);
	mode ? BIT_SET(tmp, PWMPCLKMODE) : BIT_CLR(tmp, PWMPCLKMODE);
	enable ? BIT_SET(tmp, PWMCLKGENENB) : BIT_CLR(tmp, PWMCLKGENENB);
	iowrite32(tmp,pwm.base+PWMCLKENB);
	return 0;
}
EXPORT_SYMBOL(pwm_set_clock);

int pwm_get_clock_rate(void)
{
	return pwm.clock_rate;
}
EXPORT_SYMBOL(pwm_get_clock_rate);

static int lf1000_pwm_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	int div;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		dev_err(&pdev->dev, "failed to get resource\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, RESSIZE(res), DRIVER_NAME)) {
		dev_err(&pdev->dev, "failed to map memory region.");
		return -EBUSY;
	}

	div = lf1000_CalcDivider(get_pll_freq(PWM_CLK_SRC), PWM_CLOCK_HZ);
	if(div < 0) {
		dev_err(&pdev->dev, "failed to a get clock divider\n");
		return -EFAULT;
	}

	pwm.base = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(pwm.base == NULL) {
		printk(KERN_ERR "pwm: failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	ret = pwm_set_clock(PWM_CLK_SRC, div, 1, 1);
	if(ret < 0) {
		dev_err(&pdev->dev, "divider too high, using 63\n");
		pwm_set_clock(PWM_CLK_SRC, 63, 1, 1);
		div = 63;
	}

	pwm.clock_rate = get_pll_freq(PWM_CLK_SRC)/(div+1);

	return 0;

fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
	return ret;
}

static int lf1000_pwm_remove(struct platform_device *pdev)
{
	struct resource *res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if(pwm.base != NULL)
		iounmap(pwm.base);
	release_mem_region(res->start, (res->end - res->start) + 1);
	return 0;
}

static struct platform_driver lf1000_pwm_driver = {
	.probe		= lf1000_pwm_probe,
	.remove		= lf1000_pwm_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static void __exit pwm_cleanup(void)
{
	platform_driver_unregister(&lf1000_pwm_driver);
}

static int __init pwm_init(void)
{
	return platform_driver_register(&lf1000_pwm_driver);
}

module_init(pwm_init);
module_exit(pwm_cleanup);

MODULE_AUTHOR("Andrey Yurovsky");
MODULE_LICENSE("GPL");
MODULE_VERSION("1:2.0");
