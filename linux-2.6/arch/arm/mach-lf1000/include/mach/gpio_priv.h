/* LF1000 General-Purpose IO (GPIO) Driver 
 *
 * gpio.h -- GPIO control.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Brian Cavagnolo <brian@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef GPIO_H
#define GPIO_H

#include <linux/cdev.h>
#include <mach/gpio.h>

/* module-related definitions */

#define GPIO_MAJOR		246

struct gpio_device {
	void *mem;
	void *mem_cur;
	void *amem;
	struct cdev cdev;
	dev_t dev;
	int devnum;
	int irq;
	u8 board_id;
	u8 cart_id;
	u8 touchscreen;
};

/* There's one gpio_handler per pin.  This is where the handlers are stored. */
struct gpio_handler {
	union {
		gpio_irq_handler_t gpio_handler;
		irq_handler_t normal_handler;
		void *handler;
	} handler;
	char mode_gpio;
	void *priv;
};

/* Use this spin lock to access the gpio_handlers array! */
extern spinlock_t gpio_handlers_lock;
extern struct gpio_handler gpio_handlers[GPIO_PORT_ALV+1][GPIO_PIN31+1];

enum PAD_STATUS {
	PAD_GPIOIN	       		= 1<<0,
	PAD_GPIOIN_PULLUP		= 1<<1,
	PAD_GPIOOUT	       	 	= 1<<2,
	PAD_GPIOOUT_PULLUP	  	= 1<<3,
	PAD_ALT1	       		= 1<<4,
	PAD_ALT1_PULLUP		 	= 1<<5,
	PAD_ALT2	       		= 1<<6,
	PAD_ALT2_PULLUP		 	= 1<<7,
	PAD_NOTUSED	       	 	= 1<<8,
	PAD_NOTEXIST			= 1<<9,
};

#endif

