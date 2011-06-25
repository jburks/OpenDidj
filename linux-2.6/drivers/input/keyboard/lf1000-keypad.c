/*
 * drivers/input/keyboard/lf1000.c
 *
 * Keyboard/Buttons driver for the LF1000 boards
 *
 * Copyright 2008 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/platform_device.h>

#include <mach/platform.h>
#include <mach/gpio.h>
#include <mach/i2s.h>

/* FIXME this is a call into the DPC driver for backlight setting.  We should
 * move the backlight button handling to userspace or find a cleaner way to do
 * it in kernel space, if that's really needed.  Let's at least not have
 * setBacklightNext() if we're not using the DPC driver */
#if defined CONFIG_LF1000_DPC || defined CONFIG_LF1000_DPC_MODULE
extern int setBacklightNext(void);
#else
#define setBacklightNext()
#endif

/* 
 * Software debouncing: number of ISRs for which a button must be held before 
 * we change state.
 * sde: like it or not each tick at 393MHZ is 23.5us, or 42553 ticks / second
 */
#define BUTTON_DELAY		4
#define BRIGHTNESS_DELAY	20

#define INPUT_SAMPLING_J	HZ / 200

/*
 * Key Map
 */

struct button_entry {
	enum gpio_port port;
	enum gpio_pin pin;
	enum gpio_resource resource;
	unsigned int key;
	unsigned int type;			/* event: EV_KEY or EV_SW */
	unsigned int debounce_max;		/* debounce value	*/

	enum gpio_interrupt_mode push;		/* IRQ for 'pushed' */
	enum gpio_interrupt_mode release;	/* IRQ for 'released' */
	unsigned int debounce;			/* int count		*/
};

/* the physical button map
 * fill in port and pin values at run-time in probe()
 */
static struct button_entry button_map[] = {
 /*port, pin, resource, key, key type, delay value */
 {-1, -1, DPAD_UP, KEY_UP, EV_KEY, BUTTON_DELAY},
 {-1, -1, DPAD_DOWN, KEY_DOWN, EV_KEY, BUTTON_DELAY},
 {-1, -1, DPAD_RIGHT, KEY_RIGHT, EV_KEY, BUTTON_DELAY},
 {-1, -1, DPAD_LEFT, KEY_LEFT, EV_KEY, BUTTON_DELAY},
 {-1, -1, BUTTON_A, KEY_A, EV_KEY, BUTTON_DELAY},
 {-1, -1, BUTTON_B, KEY_B, EV_KEY, BUTTON_DELAY},
 {-1, -1, SHOULDER_LEFT, KEY_L, EV_KEY, BUTTON_DELAY},/* L 'shoulder' */
 {-1, -1, SHOULDER_RIGHT, KEY_R, EV_KEY, BUTTON_DELAY},/* R 'shoulder' */
 {-1, -1, BUTTON_HOME, KEY_M, EV_KEY, BUTTON_DELAY},/* menu / home / start */
 {-1, -1, BUTTON_HINT, KEY_H, EV_KEY, BUTTON_DELAY},/* hint */
 {-1, -1, BUTTON_PAUSE, KEY_P, EV_KEY, BUTTON_DELAY},/* pause */
 {-1, -1, BUTTON_BRIGHTNESS, KEY_X, EV_KEY, BRIGHTNESS_DELAY},/* brightness */
							/* volume up  */
 {-1, -1, BUTTON_VOLUMEUP, KEY_VOLUMEUP, EV_KEY, BUTTON_DELAY},
							/* volume down*/
 {-1, -1, BUTTON_VOLUMEDOWN, KEY_VOLUMEDOWN, EV_KEY, BUTTON_DELAY},
							/* volume down*/
 {-1, -1, BUTTON_RED, KEY_RED, EV_KEY, BUTTON_DELAY},	/* K2 'red' key	*/
							/* headphone jack */
 {-1, -1, HEADPHONE_JACK, SW_HEADPHONE_INSERT, EV_SW, BUTTON_DELAY},
};


/* Keycodes that we can generate.  Although codes like KEY_MENU are defined, it
 * is easier to test with the usual alphabet keys, so we do not use the special
 * definitions. */
static unsigned int lf1000_keycode[]= {
	KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, 
	KEY_A, KEY_B, KEY_L, KEY_R, KEY_M, KEY_H, KEY_P, KEY_X,
	KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_RED};

/*
 * device
 */

struct lf1000_kp {
	unsigned int keycode[ARRAY_SIZE(lf1000_keycode)];
	struct input_dev *input;
	struct timer_list input_timer;
	uint last_audio_volumeDownCount; // monitor volume slider on Didj 08
	uint last_audio_volumeUpCount;	 // monitor volume slider on Didj 08
};

static void input_monitor_task(unsigned long data)
{
	struct lf1000_kp *i_dev = (struct lf1000_kp *)data;
	struct button_entry *bme;
	int i;
	int val;
	int old;

	for(i = 0; i < ARRAY_SIZE(button_map); i++) {
		bme = &button_map[i];

		/* synthesize Vol UP/DOWN buttons if system is Didj 08 with
		 * volume slider */
		/* note gpio = 0 when button pressed, so mimic this too */
		if (bme->key == KEY_VOLUMEDOWN && (!gpio_have_volume_buttons())) { // volume slider
			val =
(audio_get_volumeDownCount() != i_dev->last_audio_volumeDownCount) ? 0 : 1;
			if (!val) {
				i_dev->last_audio_volumeDownCount =
					audio_get_volumeDownCount();
				bme->debounce = 0x1E;
			}
		} else if (bme->key == KEY_VOLUMEUP && (!gpio_have_volume_buttons())) { //vol slider
			val =
(audio_get_volumeUpCount() != i_dev->last_audio_volumeUpCount ? 0 : 1);
			if (!val) {
				i_dev->last_audio_volumeUpCount =
					audio_get_volumeUpCount();
				bme->debounce = 0x1E;
			}
		} else { /* assume push GPIO is normally low, invert if needed	*/
			val = gpio_get_val(bme->port, bme->pin) ^ bme->push;
		}
		old = bme->debounce & 0x10;
		bme->debounce = old | ((bme->debounce << 1 | val) & 0xf);
		switch(bme->debounce)
		{
		case 0x03://0 0011 (key release)
			input_event(i_dev->input, bme->type, bme->key, 0);
			bme->debounce = 0x13;
			break;
		case 0x1C://1 1100  (key press)
			/* synthesize volume slider on systems with volume */
			/* buttons, ie Didj 09 and Leapster 3		   */
			if(gpio_have_volume_buttons()) {
				if (bme->key == KEY_VOLUMEDOWN)
					audio_set_volume(AUDIO_VOLUME_DOWN);
				else if (bme->key == KEY_VOLUMEUP)
					audio_set_volume(AUDIO_VOLUME_UP);
//				else if (bme->key == KEY_X)
//					setBacklightNext();
			}
			input_event(i_dev->input, bme->type, bme->key, 1);
			bme->debounce = 0x0C;
			break;
		}
	}

	i_dev->input_timer.expires += INPUT_SAMPLING_J;
	i_dev->input_timer.function = input_monitor_task;
	i_dev->input_timer.data = data;
	add_timer(&i_dev->input_timer);
}

/*
 * platform device
 */

static int lf1000_kp_probe(struct platform_device *pdev)
{
	struct lf1000_kp *lf1000_kp_dev;
	struct input_dev *input_dev;
	int i;
	int ret;

	lf1000_kp_dev = kzalloc(sizeof(struct lf1000_kp), GFP_KERNEL);
	if(!lf1000_kp_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, lf1000_kp_dev);

	input_dev = input_allocate_device();
	if(!input_dev) {setup_timer(&lf1000_kp_dev->input_timer, input_monitor_task, (unsigned long)lf1000_kp_dev);
		ret = -ENOMEM;
		goto fail_input;
	}

	memcpy(lf1000_kp_dev->keycode, lf1000_keycode, 
			sizeof(lf1000_kp_dev->keycode));

	input_dev->name = "LF1000 Keyboard";
	input_dev->phys = "lf1000/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;
	lf1000_kp_dev->input = input_dev;	

	/* On Didj 08 generate button_down and button_up messages */
	lf1000_kp_dev->last_audio_volumeDownCount = audio_get_volumeDownCount();
	lf1000_kp_dev->last_audio_volumeUpCount   = audio_get_volumeUpCount();

	
	/* event types that we support */
	input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_SW);

	input_dev->keycode = lf1000_kp_dev->keycode;
	input_dev->keycodesize = sizeof(unsigned int);
	input_dev->keycodemax = ARRAY_SIZE(lf1000_keycode);

	for(i = 0; i < ARRAY_SIZE(lf1000_keycode); i++)
		set_bit(lf1000_kp_dev->keycode[i], input_dev->keybit);

	/* audio jack */
	set_bit(SW_HEADPHONE_INSERT, input_dev->swbit);

	ret = input_register_device(lf1000_kp_dev->input);
	if(ret)
		goto fail_register;

	/*
 	 * map button port and pin values at run-time
 	 */

	for (i = 0; i < ARRAY_SIZE(button_map); i++) {
		button_map[i].port = lf1000_l2p_port(button_map[i].resource);
		button_map[i].pin  = lf1000_l2p_pin(button_map[i].resource);
	}

	/* 
	 * initialize and claim the buttons/switches, enable interrupts 
	 */

	for(i = 0; i < ARRAY_SIZE(button_map); i++) {
		button_map[i].debounce = 0x1f;
		button_map[i].push = GPIO_IMODE_LOW_LEVEL;
		button_map[i].release = GPIO_IMODE_HIGH_LEVEL;
		if(button_map[i].key == SW_HEADPHONE_INSERT) {
			/* reverse headphone jack on all boards except DEV */
			if(!gpio_have_gpio_dev()) {
				button_map[i].push = GPIO_IMODE_HIGH_LEVEL;
				button_map[i].release = GPIO_IMODE_LOW_LEVEL;
			}
		}
		gpio_configure_pin(button_map[i].port, button_map[i].pin,
			GPIO_GPIOFN, 0, 0, 0);
	}
	setup_timer(&lf1000_kp_dev->input_timer, input_monitor_task, (unsigned long)lf1000_kp_dev);
	lf1000_kp_dev->input_timer.expires = get_jiffies_64() + INPUT_SAMPLING_J;
	lf1000_kp_dev->input_timer.function = input_monitor_task;
	lf1000_kp_dev->input_timer.data = (unsigned long)lf1000_kp_dev;
	add_timer(&lf1000_kp_dev->input_timer);

	return 0;

fail_register:
	input_free_device(input_dev);
fail_input:
	kfree(lf1000_kp_dev);
	return ret;
}

static int lf1000_kp_remove(struct platform_device *pdev)
{
	struct lf1000_kp *lf1000_kp_dev = platform_get_drvdata(pdev);

	del_timer_sync(&lf1000_kp_dev->input_timer);
	input_unregister_device(lf1000_kp_dev->input);
	kfree(lf1000_kp_dev);

	return 0;
}

static struct platform_driver lf1000_kp_driver = {
	.probe		= lf1000_kp_probe,
	.remove		= lf1000_kp_remove,
	.driver		= {
		.name	= "lf1000-keypad",
	},
};

/*
 * module stuff
 */

static int __devinit lf1000_kp_init(void)
{
	return platform_driver_register(&lf1000_kp_driver);
}

static void __exit lf1000_kp_exit(void)
{
	platform_driver_unregister(&lf1000_kp_driver);
}

module_init(lf1000_kp_init);
module_exit(lf1000_kp_exit);

MODULE_AUTHOR("Andrey Yurovsky <andrey@cozybit.com>");
MODULE_DESCRIPTION("LF1000 Development Board buttons driver");
MODULE_LICENSE("GPL");
