/* 
 * arch/arm/mach-lf1000/gpio_l2p.h
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * LF1000 map logical gpio names to physical port/pin addresses
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/gpio_map.h>

#define	GPIO_PIN	0
#define GPIO_PORT	1

/* logical to physical mapping table */
s8 l2p_gpio_port_map[GPIO_NUMBER_VALUES][2];

/* logical resource string names
 * FIXME:keep in sync with enum gpio_resource{} in include/mach/gpio.h
 */


char *resource_name[] = {
	"HEADPHONE_JACK",
	"LED_ENA",
	"LCD_RESET",
	"AUDIO_POWER",
	"DPAD_UP",
	"DPAD_DOWN",
	"DPAD_RIGHT",
	"DPAD_LEFT",
	"BUTTON_A",
	"BUTTON_B",
	"SHOULDER_LEFT",
	"SHOULDER_RIGHT",
	"BUTTON_HOME",
	"BUTTON_HINT",
	"BUTTON_PAUSE",
	"BUTTON_BRIGHTNESS",
	"BUTTON_VOLUMEUP",
	"BUTTON_VOLUMEDOWN",
	"CARTRIDGE_DETECT",
	"TOUCHSCREEN_X1",
	"TOUCHSCREEN_Y1",
	"TOUCHSCREEN_X2",
	"TOUCHSCREEN_Y2",
	"BUTTON_RED",
};

/*
 * Initialize the logical to physical GPIO map based on board type.
 * Translate only those GPIO pins that differ between boards.
 */

void init_dev(void)
{
	l2p_gpio_port_map[LCD_RESET]		[GPIO_PORT] = DEV_LCD_RESET_PORT;
	l2p_gpio_port_map[LCD_RESET]		[GPIO_PIN]  = DEV_LCD_RESET_PIN;
	l2p_gpio_port_map[AUDIO_POWER]		[GPIO_PORT] = DEV_AUDIO_POWER_PORT;
	l2p_gpio_port_map[AUDIO_POWER]		[GPIO_PIN]  = DEV_AUDIO_POWER_PIN;

	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PORT] = DEV_CARTRIDGE_DETECT_PORT;
	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PIN]  = DEV_CARTRIDGE_DETECT_PIN;
	
	l2p_gpio_port_map[LED_ENA]		[GPIO_PORT] = DEV_LED_ENA_PORT;
	l2p_gpio_port_map[LED_ENA]		[GPIO_PIN]  = DEV_LED_ENA_PIN;

	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PORT] = DEV_TOUCHSCREEN_X1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PIN]  = DEV_TOUCHSCREEN_X1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PORT] = DEV_TOUCHSCREEN_Y1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PIN]  = DEV_TOUCHSCREEN_Y1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PORT] = DEV_TOUCHSCREEN_X2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PIN]  = DEV_TOUCHSCREEN_X2_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PORT] = DEV_TOUCHSCREEN_Y2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PIN]  = DEV_TOUCHSCREEN_Y2_PIN;
	
	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PORT] = DEV_HEADPHONE_JACK_PORT;
	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PIN]  = DEV_HEADPHONE_JACK_PIN;

	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PORT] = DEV_DPAD_LEFT_PORT;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PIN]  = DEV_DPAD_LEFT_PIN;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PORT] = DEV_DPAD_RIGHT_PORT;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PIN]  = DEV_DPAD_RIGHT_PIN;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PORT] = DEV_DPAD_UP_PORT;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PIN]  = DEV_DPAD_UP_PIN;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PORT] = DEV_DPAD_DOWN_PORT;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PIN]  = DEV_DPAD_DOWN_PIN;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PORT] = DEV_SHOULDER_RIGHT_PORT;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PIN]  = DEV_SHOULDER_RIGHT_PIN;

	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PORT] = DEV_SHOULDER_LEFT_PORT;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PIN]  = DEV_SHOULDER_LEFT_PIN;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PORT] = DEV_BUTTON_A_PORT;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PIN]  = DEV_BUTTON_A_PIN;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PORT] = DEV_BUTTON_B_PORT;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PIN]  = DEV_BUTTON_B_PIN;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PORT] = DEV_BUTTON_HOME_PORT;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PIN]  = DEV_BUTTON_HOME_PIN;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PORT] = DEV_BUTTON_HINT_PORT;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PIN]  = DEV_BUTTON_HINT_PIN;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PORT] = DEV_BUTTON_PAUSE_PORT;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PIN]  = DEV_BUTTON_PAUSE_PIN;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PORT] = DEV_BUTTON_BRIGHTNESS_PORT;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PIN]  = DEV_BUTTON_BRIGHTNESS_PIN;
}

void init_didj(void)
{
	l2p_gpio_port_map[LCD_RESET]		[GPIO_PORT] = DIDJ_LCD_RESET_PORT;
	l2p_gpio_port_map[LCD_RESET]		[GPIO_PIN]  = DIDJ_LCD_RESET_PIN;
	l2p_gpio_port_map[AUDIO_POWER]		[GPIO_PORT] = DIDJ_AUDIO_POWER_PORT;
	l2p_gpio_port_map[AUDIO_POWER]		[GPIO_PIN]  = DIDJ_AUDIO_POWER_PIN;

	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PORT] = DIDJ_CARTRIDGE_DETECT_PORT;
	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PIN]  = DIDJ_CARTRIDGE_DETECT_PIN;

	l2p_gpio_port_map[LED_ENA]		[GPIO_PORT] = DIDJ_LED_ENA_PORT;
	l2p_gpio_port_map[LED_ENA]		[GPIO_PIN]  = DIDJ_LED_ENA_PIN;

	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PORT] = DIDJ_TOUCHSCREEN_X1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PIN]  = DIDJ_TOUCHSCREEN_X1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PORT] = DIDJ_TOUCHSCREEN_Y1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PIN]  = DIDJ_TOUCHSCREEN_Y1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PORT] = DIDJ_TOUCHSCREEN_X2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PIN]  = DIDJ_TOUCHSCREEN_X2_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PORT] = DIDJ_TOUCHSCREEN_Y2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PIN]  = DIDJ_TOUCHSCREEN_Y2_PIN;
	
	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PORT] = DIDJ_HEADPHONE_JACK_PORT;
	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PIN]  = DIDJ_HEADPHONE_JACK_PIN;

	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PORT] = DIDJ_DPAD_LEFT_PORT;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PIN]  = DIDJ_DPAD_LEFT_PIN;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PORT] = DIDJ_DPAD_RIGHT_PORT;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PIN]  = DIDJ_DPAD_RIGHT_PIN;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PORT] = DIDJ_DPAD_UP_PORT;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PIN]  = DIDJ_DPAD_UP_PIN;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PORT] = DIDJ_DPAD_DOWN_PORT;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PIN]  = DIDJ_DPAD_DOWN_PIN;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PORT] = DIDJ_SHOULDER_LEFT_PORT;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PIN]  = DIDJ_SHOULDER_LEFT_PIN;

	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PORT] = DIDJ_SHOULDER_RIGHT_PORT;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PIN]  = DIDJ_SHOULDER_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PORT] = DIDJ_BUTTON_A_PORT;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PIN]  = DIDJ_BUTTON_A_PIN;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PORT] = DIDJ_BUTTON_B_PORT;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PIN]  = DIDJ_BUTTON_B_PIN;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PORT] = DIDJ_BUTTON_HOME_PORT;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PIN]  = DIDJ_BUTTON_HOME_PIN;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PORT] = DIDJ_BUTTON_HINT_PORT;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PIN]  = DIDJ_BUTTON_HINT_PIN;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PORT] = DIDJ_BUTTON_PAUSE_PORT;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PIN]  = DIDJ_BUTTON_PAUSE_PIN;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PORT] = DIDJ_BUTTON_BRIGHTNESS_PORT;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PIN]  = DIDJ_BUTTON_BRIGHTNESS_PIN;
}

void init_acorn(void)
{
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PORT] = ACORN_TOUCHSCREEN_X1_PORT; 
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PIN]  = ACORN_TOUCHSCREEN_X1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PORT] = ACORN_TOUCHSCREEN_Y1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PIN]  = ACORN_TOUCHSCREEN_Y1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PORT] = ACORN_TOUCHSCREEN_X2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PIN]  = ACORN_TOUCHSCREEN_X2_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PORT] = ACORN_TOUCHSCREEN_Y2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PIN]  = ACORN_TOUCHSCREEN_Y2_PIN;

	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PORT] = ACORN_CARTRIDGE_DETECT_PORT;
	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PIN]  = ACORN_CARTRIDGE_DETECT_PIN;

	l2p_gpio_port_map[LED_ENA]		[GPIO_PORT] = ACORN_LED_ENA_PORT;
	l2p_gpio_port_map[LED_ENA]		[GPIO_PIN]  = ACORN_LED_ENA_PIN;

	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PORT] = ACORN_DPAD_RIGHT_PORT;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PIN]  = ACORN_DPAD_RIGHT_PIN;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PORT] = ACORN_DPAD_DOWN_PORT;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PIN]  = ACORN_DPAD_DOWN_PIN;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PORT] = ACORN_BUTTON_HINT_PORT;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PIN]  = ACORN_BUTTON_HINT_PIN;
	l2p_gpio_port_map[BUTTON_VOLUMEUP]	[GPIO_PORT] = ACORN_BUTTON_VOLUMEUP_PORT;
	l2p_gpio_port_map[BUTTON_VOLUMEUP]	[GPIO_PIN]  = ACORN_BUTTON_VOLUMEUP_PIN;
	l2p_gpio_port_map[BUTTON_VOLUMEDOWN]	[GPIO_PORT] = ACORN_BUTTON_VOLUMEDOWN_PORT;
	l2p_gpio_port_map[BUTTON_VOLUMEDOWN]	[GPIO_PIN]  = ACORN_BUTTON_VOLUMEDOWN_PIN;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PORT] = ACORN_SHOULDER_RIGHT_PORT;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PIN]  = ACORN_SHOULDER_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PORT] = ACORN_BUTTON_B_PORT;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PIN]  = ACORN_BUTTON_B_PIN;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PORT] = ACORN_BUTTON_A_PORT;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PIN]  = ACORN_BUTTON_A_PIN;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PORT] = ACORN_BUTTON_BRIGHTNESS_PORT;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PIN]  = ACORN_BUTTON_BRIGHTNESS_PIN;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PORT] = ACORN_DPAD_LEFT_PORT;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PIN]  = ACORN_DPAD_LEFT_PIN;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PORT] = ACORN_SHOULDER_LEFT_PORT;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PIN]  = ACORN_SHOULDER_LEFT_PIN;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PORT] = ACORN_DPAD_UP_PORT;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PIN]  = ACORN_DPAD_UP_PIN;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PORT] = ACORN_BUTTON_PAUSE_PORT;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PIN]  = ACORN_BUTTON_PAUSE_PIN;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PORT] = ACORN_BUTTON_HOME_PORT;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PIN]  = ACORN_BUTTON_HOME_PIN;

	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PORT] = ACORN_HEADPHONE_JACK_PORT;
 	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PIN]  = ACORN_HEADPHONE_JACK_PIN;
}

void init_emerald(void)
{
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PORT] = EMERALD_TOUCHSCREEN_X1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PIN]  = EMERALD_TOUCHSCREEN_X1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PORT] = EMERALD_TOUCHSCREEN_Y1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PIN]  = EMERALD_TOUCHSCREEN_Y1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PORT] = EMERALD_TOUCHSCREEN_X2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PIN]  = EMERALD_TOUCHSCREEN_X2_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PORT] = EMERALD_TOUCHSCREEN_Y2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PIN]  = EMERALD_TOUCHSCREEN_Y2_PIN;

	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PORT] = EMERALD_CARTRIDGE_DETECT_PORT;
	l2p_gpio_port_map[CARTRIDGE_DETECT]	[GPIO_PIN]  = EMERALD_CARTRIDGE_DETECT_PIN;

	l2p_gpio_port_map[LED_ENA]		[GPIO_PORT] = EMERALD_LED_ENA_PORT;
	l2p_gpio_port_map[LED_ENA]		[GPIO_PIN]  = EMERALD_LED_ENA_PIN;

	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PORT] = EMERALD_DPAD_DOWN_PORT;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PIN]  = EMERALD_DPAD_DOWN_PIN;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PORT] = EMERALD_DPAD_RIGHT_PORT;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PIN]  = EMERALD_DPAD_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_VOLUMEDOWN]	[GPIO_PORT] = EMERALD_BUTTON_VOLUMEDOWN_PORT;
	l2p_gpio_port_map[BUTTON_VOLUMEDOWN]	[GPIO_PIN]  = EMERALD_BUTTON_VOLUMEDOWN_PIN;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PORT] = EMERALD_BUTTON_A_PORT;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PIN]  = EMERALD_BUTTON_A_PIN;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PORT] = EMERALD_BUTTON_PAUSE_PORT;
	l2p_gpio_port_map[BUTTON_PAUSE]		[GPIO_PIN]  = EMERALD_BUTTON_PAUSE_PIN;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PORT] = EMERALD_BUTTON_HINT_PORT;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PIN]  = EMERALD_BUTTON_HINT_PIN;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PORT] = EMERALD_SHOULDER_RIGHT_PORT;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PIN]  = EMERALD_SHOULDER_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_VOLUMEUP]	[GPIO_PORT] = EMERALD_BUTTON_VOLUMEUP_PORT;
	l2p_gpio_port_map[BUTTON_VOLUMEUP]	[GPIO_PIN]  = EMERALD_BUTTON_VOLUMEUP_PIN;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PORT] = EMERALD_DPAD_LEFT_PORT;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PIN]  = EMERALD_DPAD_LEFT_PIN;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PORT] = EMERALD_DPAD_UP_PORT;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PIN]  = EMERALD_DPAD_UP_PIN;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PORT] = EMERALD_BUTTON_BRIGHTNESS_PORT;
	l2p_gpio_port_map[BUTTON_BRIGHTNESS]	[GPIO_PIN]  = EMERALD_BUTTON_BRIGHTNESS_PIN;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PORT] = EMERALD_BUTTON_B_PORT;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PIN]  = EMERALD_BUTTON_B_PIN;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PORT] = EMERALD_BUTTON_HOME_PORT;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PIN]  = EMERALD_BUTTON_HOME_PIN;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PORT] = EMERALD_SHOULDER_LEFT_PORT;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PIN]  = EMERALD_SHOULDER_LEFT_PIN;

	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PORT] = EMERALD_HEADPHONE_JACK_PORT;
 	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PIN]  = EMERALD_HEADPHONE_JACK_PIN;
}

void init_k2(void)
{
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PORT] = K2_TOUCHSCREEN_X1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X1]	[GPIO_PIN]  = K2_TOUCHSCREEN_X1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PORT] = K2_TOUCHSCREEN_Y1_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y1]	[GPIO_PIN]  = K2_TOUCHSCREEN_Y1_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PORT] = K2_TOUCHSCREEN_X2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_X2]	[GPIO_PIN]  = K2_TOUCHSCREEN_X2_PIN;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PORT] = K2_TOUCHSCREEN_Y2_PORT;
	l2p_gpio_port_map[TOUCHSCREEN_Y2]	[GPIO_PIN]  = K2_TOUCHSCREEN_Y2_PIN;

	l2p_gpio_port_map[LED_ENA]		[GPIO_PORT] = K2_LED_ENA_PORT;
	l2p_gpio_port_map[LED_ENA]		[GPIO_PIN]  = K2_LED_ENA_PIN;

	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PORT] = K2_DPAD_DOWN_PORT;
	l2p_gpio_port_map[DPAD_DOWN]		[GPIO_PIN]  = K2_DPAD_DOWN_PIN;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PORT] = K2_DPAD_RIGHT_PORT;
	l2p_gpio_port_map[DPAD_RIGHT]		[GPIO_PIN]  = K2_DPAD_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PORT] = K2_BUTTON_A_PORT;
	l2p_gpio_port_map[BUTTON_A]		[GPIO_PIN]  = K2_BUTTON_A_PIN;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PORT] = K2_BUTTON_HINT_PORT;
	l2p_gpio_port_map[BUTTON_HINT]		[GPIO_PIN]  = K2_BUTTON_HINT_PIN;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PORT] = K2_SHOULDER_RIGHT_PORT;
	l2p_gpio_port_map[SHOULDER_RIGHT]	[GPIO_PIN]  = K2_SHOULDER_RIGHT_PIN;
	l2p_gpio_port_map[BUTTON_RED]		[GPIO_PORT] = K2_BUTTON_RED_PORT;
	l2p_gpio_port_map[BUTTON_RED]		[GPIO_PIN]  = K2_BUTTON_RED_PIN;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PORT] = K2_DPAD_LEFT_PORT;
	l2p_gpio_port_map[DPAD_LEFT]		[GPIO_PIN]  = K2_DPAD_LEFT_PIN;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PORT] = K2_DPAD_UP_PORT;
	l2p_gpio_port_map[DPAD_UP]		[GPIO_PIN]  = K2_DPAD_UP_PIN;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PORT] = K2_BUTTON_B_PORT;
	l2p_gpio_port_map[BUTTON_B]		[GPIO_PIN]  = K2_BUTTON_B_PIN;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PORT] = K2_BUTTON_HOME_PORT;
	l2p_gpio_port_map[BUTTON_HOME]		[GPIO_PIN]  = K2_BUTTON_HOME_PIN;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PORT] = K2_SHOULDER_LEFT_PORT;
	l2p_gpio_port_map[SHOULDER_LEFT]	[GPIO_PIN]  = K2_SHOULDER_LEFT_PIN;

	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PORT] = K2_HEADPHONE_JACK_PORT;
 	l2p_gpio_port_map[HEADPHONE_JACK]	[GPIO_PIN]  = K2_HEADPHONE_JACK_PIN;
}


void lf1000_l2p_init(void)
{
	static int isInited = 0;

	if (isInited) return;	// table is initialized

	/* mark all translations as invalid */
	memset(l2p_gpio_port_map, -1, sizeof(l2p_gpio_port_map));

	/* put most recent board at top of list */
	if      (gpio_have_gpio_k2())      init_k2();
	else if (gpio_have_gpio_emerald()) init_emerald();
	else if (gpio_have_gpio_didj())    init_didj();
	else if (gpio_have_gpio_acorn())   init_acorn();
	else if (gpio_have_gpio_dev())     init_dev();
	else {
		printk(KERN_ERR "%s.%d:%s() unknown board type (%d)\n",
			__FILE__, __LINE__, __FUNCTION__,
			gpio_get_board_config());
	}
	
	isInited = 1;	// finished initializing table
}


int lf1000_l2p_port(enum gpio_resource logical_value)
{
	lf1000_l2p_init();	// initialize translation table if needed

	if (0 <= logical_value && logical_value < GPIO_NUMBER_VALUES) {
		if (l2p_gpio_port_map[logical_value][GPIO_PORT] == -1) {
			printk(KERN_INFO "resource %s (%d) port is undefined (-1)\n",
				resource_name[logical_value], logical_value);
		}
		return(l2p_gpio_port_map[logical_value][GPIO_PORT]);
	} else {
		printk(KERN_INFO "Resource '%d' is out of defined range.",
			logical_value);
		return -1;
	}
}
EXPORT_SYMBOL(lf1000_l2p_port);

int lf1000_l2p_pin(enum gpio_resource logical_value)
{
	lf1000_l2p_init();	// initialize translation table if needed

	if (0 <= logical_value && logical_value < GPIO_NUMBER_VALUES) {
		if (l2p_gpio_port_map[logical_value][GPIO_PORT] == -1) {
			printk(KERN_INFO "resource %s (%d) pin is undefined (-1)\n",
				resource_name[logical_value], logical_value);
		}
		return(l2p_gpio_port_map[logical_value][GPIO_PIN]);
	} else {
		printk(KERN_INFO "Resource '%d' is out of defined range.",
			logical_value);
		return -1;
	}
}
EXPORT_SYMBOL(lf1000_l2p_pin);
 
