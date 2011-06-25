/* LF1000 Pulse Width Modulator (PWM) Driver 
 *
 * mach-lf1000/include/mach/pwm.h -- PWM API.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef LF1000_PWM_H
#define LF1000_PWM_H

#include <mach/platform.h>

#define PWM_MAX_VALUE	0x3FF

enum pwm_pol {
	POL_INV		= 0,
	POL_BYP 	= 1,
	POL_INVALID	= 2,
};

enum pwm_chan {
	PWM_CHAN0 = 0,
	PWM_CHAN1,
	PWM_CHAN2,
	PWM_CHAN_INVALID,
};

enum pwm_clk {
	PWM_CLK0 = 0,
	PWM_CLK1,
	PWM_CLK_INVALID,
};

int pwm_configure_pin(enum pwm_chan channel);
int pwm_set_prescale(enum pwm_chan channel, u32 prescale);
int pwm_set_polarity(enum pwm_chan channel, u8 polarity);
int pwm_set_period(enum pwm_chan channel, u32 period);
int pwm_set_duty_cycle(enum pwm_chan channel, u32 duty);
int pwm_set_clock(u8 source, u8 div, u8 mode, u8 enable);
int pwm_get_clock_rate(void);

#endif
