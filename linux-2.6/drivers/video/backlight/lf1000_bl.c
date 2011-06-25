/*
 * drivers/video/backlight/lf1000_bl.c
 *
 * PWM backlight support for the LF1000 LeapFrog boards.
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>

#include <mach/platform.h>
#include <mach/pwm.h>
#include <mach/gpio.h>

#define BL_PWM_CHANNEL	1

struct lf1000_bl {
	struct platform_device	*pdev;
	struct backlight_device *bl;

	u32			pwmds;
};

static int lf1000_bl_get_brightness(struct backlight_device *bd)
{
	struct lf1000_bl *priv = bl_get_data(bd);

	return priv->pwmds;
}

static int lf1000_bl_set_brightness(struct backlight_device *bd)
{
	struct lf1000_bl *priv = bl_get_data(bd);
	int intensity = bd->props.brightness;

	if (bd->props.power != FB_BLANK_UNBLANK)
		intensity = 0;
	if (bd->props.fb_blank != FB_BLANK_UNBLANK)
		intensity = 0;

	if (pwm_set_duty_cycle(BL_PWM_CHANNEL, intensity))
		return -EINVAL;

	priv->pwmds = intensity;

	return 0;
}

static struct backlight_ops lf1000_bl_ops = {
	.get_brightness	= lf1000_bl_get_brightness,
	.update_status	= lf1000_bl_set_brightness,
};

static int lf1000_bl_probe(struct platform_device *pdev)
{
	int ret;
	struct lf1000_bl *priv;

	priv = kzalloc(sizeof(struct lf1000_bl), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "can't allocate priv data\n");
		return -ENOMEM;
	}
	priv->pdev = pdev;

	priv->bl = backlight_device_register("lf1000-pwm-bl",
			&pdev->dev, priv, &lf1000_bl_ops);
	if (IS_ERR(priv->bl)) {
		ret = PTR_ERR(priv->bl);
		dev_err(&pdev->dev, "failed to register backlight: %d\n", ret);
		kfree(priv);
	}

	platform_set_drvdata(pdev, priv);

	priv->bl->props.power = FB_BLANK_UNBLANK;
	priv->bl->props.max_brightness = 512;
	priv->bl->props.brightness = priv->bl->props.max_brightness / 2;

	ret = pwm_get_clock_rate();
	if (ret < 1) {
		dev_err(&pdev->dev, "can't get PWM rate\n");
		priv->pwmds = 0;
	} else {
		dev_info(&pdev->dev, "PWM rate is %d\n", ret);
		pwm_configure_pin(BL_PWM_CHANNEL);
		pwm_set_prescale(BL_PWM_CHANNEL, 1);
		pwm_set_polarity(BL_PWM_CHANNEL, 1);
		pwm_set_period(BL_PWM_CHANNEL, 512);
	}

	lf1000_bl_set_brightness(priv->bl);

	return 0;
}

static int __exit lf1000_bl_remove(struct platform_device *pdev)
{
	struct lf1000_bl *priv = platform_get_drvdata(pdev);

	backlight_device_unregister(priv->bl);
	platform_set_drvdata(pdev, NULL);
	kfree(priv);

	return 0;
}

static struct platform_driver lf1000_bl_driver = {
	.probe	= lf1000_bl_probe,
	.remove	= __exit_p(lf1000_bl_remove),
	.driver = {
		.name	= "lf1000-bl",
		.owner	= THIS_MODULE,
	},
};

static int __init lf1000_bl_init(void)
{
	return platform_driver_register(&lf1000_bl_driver);
}

static void __exit lf1000_bl_exit(void)
{
	platform_driver_unregister(&lf1000_bl_driver);
}

module_init(lf1000_bl_init);
module_exit(lf1000_bl_exit);

MODULE_AUTHOR("Andrey Yurovsky");
MODULE_DESCRIPTION("LF1000 backlight driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:lf1000-bl");
