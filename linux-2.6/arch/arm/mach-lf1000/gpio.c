/* 
 * arch/arm/mach-lf1000/gpio.c
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * LF1000 General-Purpose IO (GPIO) API, see also 
 * include/asm/arch-lf1000/gpio.h
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Brian Cavagnolo <brian@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <mach/gpio_priv.h>
#include <mach/gpio.h>
#include <mach/common.h>
#include <mach/gpio_hal.h>
#include "board_ids.h"

#define IS_GPIO_PORT(p)	(p >= GPIO_PORT_A && p <= GPIO_PORT_ALV)

extern struct gpio_device gpio;

spinlock_t gpio_handlers_lock = SPIN_LOCK_UNLOCKED;
struct gpio_handler gpio_handlers[GPIO_PORT_ALV+1][GPIO_PIN31+1];

/* Set the pin function. */
int gpio_set_fn(enum gpio_port port, enum gpio_pin pin,
		 enum gpio_function f)
{
	void *reg;
	unsigned long tmp;

	if(!gpio.mem || !IS_GPIO_PORT(port) || pin >= 32)
		return -EINVAL;
   	reg = gpio.mem + GPIOAALTFN0 + port*0x40;

	if(pin >= 16) { /* use GPIOnALTFN1 */
		reg += 4;
		pin -= 16;
	}

	pin *= 2; /* setting two bits per pin */
	tmp = ioread32(reg);
	tmp &= ~(3<<pin);
	tmp |= (f<<pin);
	iowrite32(tmp, reg);

	return 0;
}

/* Get the pin function */
int gpio_get_fn(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;

	if(!gpio.mem || !IS_GPIO_PORT(port) || pin >= 32)
		return -EINVAL;
	reg = gpio.mem + GPIOAALTFN0 + port*0x40;

	if(pin >= 16) { /* use GPIOnALTFN1 */
		reg +=4;
		pin -= 16;
	}

	/* getting two bits per pin */
	return ((ioread32(reg) >> (pin*2)) & 0x3);
}

/* set or clear output enable.  Clearing output enable means this pin is an
 * input.
 */
int gpio_set_out_en(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	void *reg;
	unsigned long tmp;
	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		if(!en)
			printk(KERN_WARNING
			       "gpio: LF1000 ALIVE pins are outputs only!\n");
		return -EINVAL;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAOUTENB + port*0x40;
	else
		return -EINVAL;
	tmp = ioread32(reg);

	en ? BIT_SET(tmp, pin) : BIT_CLR(tmp, pin);
	iowrite32(tmp, reg);

	return 0;
}

/* set or clear the pull-up enable */
void gpio_set_pu(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	void *reg;
	unsigned long tmp;

	if(pin >= 32)
		return;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAPUENB + port*0x40;
	else
		return;
   	tmp = ioread32(reg);

	en ? BIT_SET(tmp, pin) : BIT_CLR(tmp, pin);
	iowrite32(tmp, reg);
}

/* get the input value of the pin */
int gpio_get_pu(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;
	unsigned long tmp;

	if(pin >= 32)
		return -EINVAL;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		return -EINVAL; /* LF1000 ALIVE pins don't have pullups */
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAPUENB + port*0x40;
	else
		return -EINVAL;
   	tmp = ioread32(reg);

	return (int)((ioread32(reg) >> pin) & 0x1);
}

/* set the output value of the pin */
int gpio_set_val(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	void *reg, *set_reg, *clr_reg;
	unsigned long tmp;

	if(pin >= 32)
		return -EINVAL;

	if(port == GPIO_PORT_ALV) {
		if(gpio.amem == NULL)
			return -EINVAL;

		/* On the LF1000, we're operating an SR flip-flop, not a GPIO
		 * pin.
		 */

		/* enable writing to GPIO ALIVE registers */
		iowrite32(1 << NPOWERGATING, gpio.amem + ALIVEPWRGATEREG);
		
		if (en) { /* set R/S bit */
			set_reg = gpio.amem + ALIVEGPIOSETREG;
			clr_reg = gpio.amem + ALIVEGPIORSTREG;
		} else {  /* clear R/S bit */
			clr_reg = gpio.amem + ALIVEGPIOSETREG;
			set_reg = gpio.amem + ALIVEGPIORSTREG;
		}

		tmp = ioread32(clr_reg); /* clear bit first */
		BIT_CLR(tmp,pin);
		iowrite32(tmp,clr_reg);
		tmp = ioread32(set_reg); /* then set bit */
		BIT_SET(tmp,pin);
		iowrite32(tmp,set_reg);
	
		/* disable writing to GPIO ALIVE registers */
		iowrite32(0 << NPOWERGATING, gpio.amem + ALIVEPWRGATEREG);
		return 0;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAOUT + port*0x40;
	else
		return - EINVAL;
	
	tmp = ioread32(reg);
	en ? BIT_SET(tmp, pin) : BIT_CLR(tmp, pin);
	iowrite32(tmp, reg);

	return 0;
}

/* get the input value of the pin */
int gpio_get_val(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;
	unsigned long tmp;

	if(pin >= 32)
		return -EINVAL;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		/* LF1000 ALIVE pins are only outputs, but we can still read
		 * them.
		 */
		reg = gpio.amem + ALIVEGPIOREADREG;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAPAD + port*0x40;
	else
		return -EINVAL;
   	tmp = ioread32(reg);

	return (int)((ioread32(reg) >> pin) & 0x1);
}

/* request an interrupt handler for a given pin. Returns -EBUSY if that pin
 * already has a handler.
 */
int gpio_request_irq(enum gpio_port port, enum gpio_pin pin,
		     gpio_irq_handler_t handler, void *priv)
{
	unsigned long flags;
	struct gpio_handler *gh;
	int ret;

	if(pin >= 32)
		return -EBUSY;
	if(!IS_GPIO_PORT(port))
		return - EBUSY;

	spin_lock_irqsave(&gpio_handlers_lock, flags);
	gh = &gpio_handlers[port][pin];
	if(gh->handler.handler != NULL)
		ret = -EBUSY;
	else {
		gh->handler.gpio_handler = handler;
		gh->priv = priv;
		gh->mode_gpio = 1;
		ret = 0;
	}
	spin_unlock_irqrestore(&gpio_handlers_lock, flags);
	return ret;
}

int gpio_request_normal_irq(enum gpio_port port, enum gpio_pin pin,
		irq_handler_t handler, void *priv)
{
	unsigned long flags;
	struct gpio_handler *gh;
	int ret;

	if(pin >= 32)
		return -EBUSY;

	if(!IS_GPIO_PORT(port))
		return -EBUSY;

	spin_lock_irqsave(&gpio_handlers_lock, flags);
	gh = &gpio_handlers[port][pin];
	if(gh->handler.handler != NULL)
		ret = -EBUSY;
	else {
		gh->handler.normal_handler = handler;
		gh->priv = priv;
		gh->mode_gpio = 0;
		ret = 0;
	}
	spin_unlock_irqrestore(&gpio_handlers_lock, flags);
	return ret;
}

/* free the irq requested using gpio_request_irq.  To prevent accidental
 * freeing of somebody else's gpio irq, the handler must match the one that was
 * passed to gpio_request_irq.
 */
void gpio_free_irq(enum gpio_port port, enum gpio_pin pin,
		   gpio_irq_handler_t handler)
{
	unsigned long flags;
	struct gpio_handler *gh;

	if(pin >= 32)
		return;
	if(!IS_GPIO_PORT(port))
		return;

	spin_lock_irqsave(&gpio_handlers_lock, flags);
	gh = &gpio_handlers[port][pin];
	if(gh->handler.handler == handler) {
		gh->handler.handler = NULL;
		gh->priv = NULL;
	}
	spin_unlock_irqrestore(&gpio_handlers_lock, flags);
}

/* get the interrupt mode for a given pin */
enum gpio_interrupt_mode gpio_get_int_mode(enum gpio_port port,
					   enum gpio_pin pin)
{
	void *reg;
	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_get_int_mode\n");
		return 0;
	} else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32) {
		reg = gpio.mem + port*0x40;
		if(pin < 16) {
			reg += GPIOADETMODE0;
		} else {
			reg += GPIOADETMODE1;
			pin -= 16;
		}
	}
	else
		return 0; /*XXX*/
	return (enum gpio_interrupt_mode)((ioread32(reg) >> (pin<<1)) & 0x3);
}

/* set the interrupt mode for a given pin */
void gpio_set_int_mode(enum gpio_port port, enum gpio_pin pin,
		       enum gpio_interrupt_mode mode)
{
	void *reg;
	unsigned long tmp;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_set_int_mode\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32) {
		reg = gpio.mem + port*0x40;
		if(pin < 16) {
			reg += GPIOADETMODE0;
		} else {
			reg += GPIOADETMODE1;
			pin -= 16;
		}
	}
	else
		return;

	tmp = ioread32(reg);
	tmp &= ~(0x3 << (pin<<1));
	tmp |= (mode << (pin<<1));
	iowrite32(tmp, reg);
}

/* toggle the interrupt mode for a pin.  If the mode is currently
 * IMODE_RISING_EDGE it becmoes IMODE_FALLING_EDGE and vice versa.  If the mode
 * is IMODE_HIGH_LEVEL it becomes IMODE_LOW_LEVEL
 */
void gpio_toggle_int_mode(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;
	unsigned long tmp;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_toggle_int_mode\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32) {
		reg = gpio.mem + port*0x40;
		if(pin < 16) {
			reg += GPIOADETMODE0;
		} else {
			reg += GPIOADETMODE1;
			pin -= 16;
		}
	}
	else
		return;

	tmp = ioread32(reg);
	tmp ^= (0x1 << (pin<<1));
	iowrite32(tmp, reg);
}

/* enable or disable interrupt for a given pin */
void gpio_set_int(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	void *reg;
	unsigned long tmp;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_set_int\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32) {
		reg = gpio.mem + GPIOAINTENB + port*0x40;
	}
	else {
		printk(KERN_WARNING "gpio: gpio_set_int when uninitialized\n");
		return;
	}

	tmp = ioread32(reg);
	en ? BIT_SET(tmp, pin) : BIT_CLR(tmp, pin);
	iowrite32(tmp, reg);

}

/* get the interrupt enable bit for a given pin */
unsigned char gpio_get_int(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;
	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_get_int\n");
		return 0;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32)
		reg = gpio.mem + GPIOAINTENB + port*0x40;
	else
		return 0; /*XXX*/

	return (unsigned char)((ioread32(reg) >> pin) & 0x1);
}

/* get interrupt enable bits for all 32 pins in a given port */
unsigned long gpio_get_int32(enum gpio_port port)
{
	void *reg;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_get_int32\n");
		return 0;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAINTENB + port*0x40;
	else
		return 0; /*XXX*/

	return ioread32(reg);
}

/* set the interrupt enable bits for all 32 pins in a given port.  Use this
 * function in conjunction with gpio_get_int32 to enable or disable
 * interrupts on many pins at a time.
 */
void gpio_set_int32(enum gpio_port port, unsigned long en)
{
	void *reg;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_set_int32\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOAINTENB + port*0x40;
	else
		return;

	iowrite32(en, reg);
}

/* clear the interrupt pending bit for a given pin */
void gpio_clear_pend(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_clear_pend\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32)
		reg = gpio.mem + GPIOADET + port*0x40;
	else
		return;
	
	iowrite32(1<<pin, reg);
}

/* get the interrupt pending bit for a given pin */
unsigned char gpio_get_pend(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;
	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_get_pend\n");
		return 0;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port) && pin < 32)
		reg = gpio.mem + GPIOADET + port*0x40;
	else
		return 0; /*XXX*/

	return (unsigned char)((ioread32(reg) >> pin) & 0x1);
}

/* get the interrupt pending bits for all pins in a given port */
unsigned long gpio_get_pend32(enum gpio_port port)
{
	void *reg;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_clear_pend32\n");
		return 0;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOADET + port*0x40;
	else
		return 0; /*XXX*/

	return ioread32(reg);
}

/* clear the interrupt pending bits for all pins in a given port.  Use this
 * function in conjunction with gpio_get_pend32 to clear all pending interrupts
 * at once.
 */
void gpio_clear_pend32(enum gpio_port port, unsigned long flag)
{
	void *reg;

	if(port == GPIO_PORT_ALV && gpio.amem != NULL) {
		printk(KERN_ALERT "gpio: ALV in gpio_clear_pend32\n");
		return;
	}
	else if(gpio.mem != NULL && IS_GPIO_PORT(port))
		reg = gpio.mem + GPIOADET + port*0x40;
	else
		return;

	iowrite32(flag, reg);
}


/* get gpio pin drive current setting */
unsigned long gpio_get_cur(enum gpio_port port, enum gpio_pin pin)
{
	void *reg;

	if((port == GPIO_PORT_ALV) ||
	    ((port == GPIO_PORT_C) && (pin > GPIO_PIN19))) {
		/* We can't set current for GPIO ALIVE block or for GPIOC pins
		 * above 19 
		 */
		return 0;
	} else if(gpio.mem_cur != NULL && IS_GPIO_PORT(port) && pin <= 32) {
		reg = gpio.mem_cur + port*8;
		if(pin < 16) {
			reg += GPIOPADSTRENGTHGPIOAL;
		} else {
			reg += GPIOPADSTRENGTHGPIOAH;
			pin -= 16;
		}
	}
	else
		return 0; /*XXX*/
	return ((ioread32(reg) >> (pin<<1)) & 0x3);
}

/* set the drive current for the gpio pin */
void gpio_set_cur(enum gpio_port port, enum gpio_pin pin, enum gpio_current cur)
{
	void *reg;
	unsigned long tmp;

	if( (port == GPIO_PORT_ALV) ||
	    ((port == GPIO_PORT_C) && (pin > GPIO_PIN19)) ) {
		/* We can't set current for GPIO ALIVE block or for GPIOC pins
		 * above 19 
		 */
		return;
	} else if(gpio.mem_cur != NULL && IS_GPIO_PORT(port) && pin < 32) {
		reg = gpio.mem_cur + port*8;
		if(pin < 16) {
			reg += GPIOPADSTRENGTHGPIOAL;
		} else {
			reg += GPIOPADSTRENGTHGPIOAH;
			pin -= 16;
		}
	}
	else
		return;

	tmp = ioread32(reg);
	tmp &= ~(0x3 << (pin<<1));
	tmp |= (cur << (pin<<1));
	iowrite32(tmp, reg);

}

/* gpio_get_scratch() -- get ALIVE scratch register value */
unsigned long gpio_get_scratch(void)
{
	return(ioread32(gpio.amem + ALIVESCRATCHREADREG));
}

/* gpio_set_scratch() -- set ALIVE scratch register value */
void gpio_set_scratch(unsigned long value)
{
	unsigned int reg32 = 0;

	if(gpio.amem == NULL)
		return;

	/* enable writing to GPIO ALIVE registers */
	reg32 = ioread32(gpio.amem + ALIVEPWRGATEREG);
	BIT_SET(reg32, NPOWERGATING);	
	iowrite32(reg32, gpio.amem + ALIVEPWRGATEREG);

	iowrite32(0, gpio.amem + ALIVESCRATCHRSTREG);
	iowrite32(0, gpio.amem + ALIVESCRATCHSETREG);

	iowrite32(~value, gpio.amem + ALIVESCRATCHRSTREG);
	iowrite32(value, gpio.amem + ALIVESCRATCHSETREG);

	iowrite32(0, gpio.amem + ALIVESCRATCHRSTREG);	
	iowrite32(0, gpio.amem + ALIVESCRATCHSETREG);
	
	/* disable writing to GPIO ALIVE registers */
	reg32 = ioread32(gpio.amem + ALIVEPWRGATEREG);
	BIT_CLR(reg32, NPOWERGATING);	
	iowrite32(reg32, gpio.amem + ALIVEPWRGATEREG);
}

/* gpio_get_power_config() -- get power bits of register */
u8 gpio_get_power_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_POWER_POS) & 
			BIT_MASK_ONES(SCRATCH_POWER_SIZE));
}

/* gpio_get_shutdown_config() -- get shutdown bits of register */
enum scratch_shutdown gpio_get_shutdown_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_SHUTDOWN_POS) & 
			BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE));
}

/* gpio_set_shutdown_config() -- set shutdown bits of register */
void gpio_set_shutdown_config(enum scratch_shutdown value)
{
	ulong scratch = gpio_get_scratch();	// get bits
						// remove bits
	scratch &=
		~(BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE) << SCRATCH_SHUTDOWN_POS);
	scratch |= (value << SCRATCH_SHUTDOWN_POS);
	gpio_set_scratch(scratch);
}

/* gpio_get_request_config() -- get request boot bits of register */
enum scratch_request gpio_get_request_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_REQUEST_POS) & 
			BIT_MASK_ONES(SCRATCH_REQUEST_SIZE));
}

/* gpio_set_request_config() -- set request boot bits of register */
void gpio_set_request_config(enum scratch_request value)
{
	ulong scratch = gpio_get_scratch();	// get bits
						// remove bits
	scratch &=
		~(BIT_MASK_ONES(SCRATCH_REQUEST_SIZE)
			<< SCRATCH_REQUEST_POS);
	scratch |= (value << SCRATCH_REQUEST_POS);
	gpio_set_scratch(scratch);
}

/* gpio_get_boot_image_config() -- get actual boot bits of register */
enum scratch_boot_image gpio_get_boot_image_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_BOOT_IMAGE_POS) & 
			BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE));
}

/* gpio_set_boot_image_config() -- set actual boot bits of register */
void gpio_set_boot_image_config(enum scratch_boot_image value)
{
	ulong scratch = gpio_get_scratch();	// get bits
						// remove bits
	scratch &=
	  ~(BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE) << SCRATCH_BOOT_IMAGE_POS);
	scratch |= (value << SCRATCH_BOOT_IMAGE_POS);
	gpio_set_scratch(scratch);
}

/* gpio_get_boot_source_config() -- get boot source bits of register */
enum scratch_boot_source gpio_get_boot_source_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_BOOT_SOURCE_POS) & 
			BIT_MASK_ONES(SCRATCH_BOOT_SOURCE_SIZE));
}

/* gpio_get_panic_config() -- get panic boot bits of register */
int gpio_get_panic_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_PANIC_POS) & 
			BIT_MASK_ONES(SCRATCH_PANIC_SIZE));
}

/* gpio_set_panic_config() -- set panic boot bits of register */
void gpio_set_panic_config(int value)
{
	ulong scratch = gpio_get_scratch();	// get bits
						// remove bits
	scratch &=
		~(BIT_MASK_ONES(SCRATCH_PANIC_SIZE)
			<< SCRATCH_PANIC_POS);
	scratch |= (value << SCRATCH_PANIC_POS);
	gpio_set_scratch(scratch);
}

void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin, 
		enum gpio_function f, unsigned char out_en, 
		unsigned char pu_en, unsigned char val)
{
	gpio_set_fn(port, pin, f);
	gpio_set_out_en(port, pin, out_en);
	gpio_set_pu(port, pin, pu_en);
	gpio_set_val(port, pin, val);
}

/*
 * Volume control is either volume slider (Didj) or up/down buttons (Emerald).
 * Assume if there is a touchscreen then we have up/down buttons
 */

int gpio_have_volume_buttons(void)
{
	return(gpio.touchscreen);
}

/*
 * Touchscreen test is down during modprobe.
 */

int gpio_have_touchscreen(void)
{
	return(gpio.touchscreen);
}

/*
 * Get board ID from circuit board.
 * Use this as last resort on figuring out hardware configuration.
 */

u8 gpio_get_board_config(void)
{
#ifndef CONFIG_LF1000_OVERRIDE_BOARD_ID
	return ((gpio_get_scratch() >> SCRATCH_BOARD_ID_POS) & 
			BIT_MASK_ONES(SCRATCH_BOARD_ID_SIZE));
#else
	return CONFIG_LF1000_OVERRIDE_BOARD_ID_VALUE;
#endif

}

/*
 * Board ID helper functions
 */

int gpio_have_gpio_acorn(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_ACORN:
		return(1);
	}
	return(0);
}

int gpio_have_gpio_dev(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_DEV:
		return(1);
	}
	return(0);
}

int gpio_have_gpio_didj(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_DIDJ:
	case LF1000_BOARD_DIDJ_09:
		return(1);
	}
	return(0);
}

int gpio_have_gpio_emerald(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_EMERALD_POP:
	case LF1000_BOARD_EMERALD_NOTV_NOCAP:
	case LF1000_BOARD_EMERALD_TV_NOCAP:
	case LF1000_BOARD_EMERALD_NOTV_CAP:
	case LF1000_BOARD_EMERALD_CIP_POP:
	case LF1000_BOARD_EMERALD_CIP_TV:
		return(1);
	}
	return(0);
}

int gpio_have_gpio_k2(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_K2:
		return(1);
	}
	return(0);
}

int gpio_have_supercap(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_ACORN:
	case LF1000_BOARD_EMERALD_NOTV_CAP:
		return(1);
	}
	return(0);
}

int gpio_have_tvout(void)
{
	switch(gpio_get_board_config()) {
	case LF1000_BOARD_ACORN:
	case LF1000_BOARD_EMERALD_POP:
	case LF1000_BOARD_EMERALD_TV_NOCAP:
	case LF1000_BOARD_EMERALD_CIP_POP:
	case LF1000_BOARD_EMERALD_CIP_TV:
		return(1);
	}
	return(0);
}

/* gpio_get_boot_image_config() -- get actual boot bits of register */
u32 gpio_get_user_0_config(void)
{
	return ((gpio_get_scratch() >> SCRATCH_USER_0_POS) & 
			BIT_MASK_ONES(SCRATCH_USER_0_SIZE));
}

/* gpio_set_boot_image_config() -- set actual boot bits of register */
void gpio_set_user_0_config(u32 value)
{
	ulong scratch = gpio_get_scratch();	// get bits
						// remove bits
	scratch &= ~(BIT_MASK_ONES(SCRATCH_USER_0_SIZE) 
		     << SCRATCH_USER_0_POS);
	scratch |= ((value & BIT_MASK_ONES(SCRATCH_USER_0_SIZE))
		     << SCRATCH_USER_0_POS);
	gpio_set_scratch(scratch);
}

EXPORT_SYMBOL(gpio_set_fn);
EXPORT_SYMBOL(gpio_get_fn);
EXPORT_SYMBOL(gpio_set_out_en);
EXPORT_SYMBOL(gpio_get_pu);
EXPORT_SYMBOL(gpio_set_pu);
EXPORT_SYMBOL(gpio_set_val);
EXPORT_SYMBOL(gpio_get_val);
EXPORT_SYMBOL(gpio_request_irq);
EXPORT_SYMBOL(gpio_request_normal_irq);
EXPORT_SYMBOL(gpio_free_irq);
EXPORT_SYMBOL(gpio_get_int_mode);
EXPORT_SYMBOL(gpio_set_int_mode);
EXPORT_SYMBOL(gpio_toggle_int_mode);
EXPORT_SYMBOL(gpio_set_int);
EXPORT_SYMBOL(gpio_get_int);
EXPORT_SYMBOL(gpio_get_int32);
EXPORT_SYMBOL(gpio_set_int32);
EXPORT_SYMBOL(gpio_get_pend);
EXPORT_SYMBOL(gpio_clear_pend);
EXPORT_SYMBOL(gpio_get_pend32);
EXPORT_SYMBOL(gpio_clear_pend32);
EXPORT_SYMBOL(gpio_configure_pin);
EXPORT_SYMBOL(gpio_get_board_config);
EXPORT_SYMBOL(gpio_get_cur);
EXPORT_SYMBOL(gpio_set_cur);
EXPORT_SYMBOL(gpio_have_supercap);
EXPORT_SYMBOL(gpio_have_gpio_acorn);
EXPORT_SYMBOL(gpio_have_gpio_dev);
EXPORT_SYMBOL(gpio_have_gpio_didj);
EXPORT_SYMBOL(gpio_have_gpio_emerald);
EXPORT_SYMBOL(gpio_have_tvout);
