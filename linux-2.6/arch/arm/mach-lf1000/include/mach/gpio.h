/* mach-lf1000/include/mach/gpio.h -- General-Purpose IO (GPIO) API
 *
 * gpio.h -- GPIO control.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Brian Cavagnolo <brian@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef LF1000_GPIO_H
#define LF1000_GPIO_H

#define GPIO_NUM_PORTS 3
enum gpio_port {
	GPIO_PORT_A	= 0x0,
	GPIO_PORT_B,
	GPIO_PORT_C,
	GPIO_PORT_ALV,
};

/*
 * Pins for Board Configuration resistors.  These will be configured as input
 * pins and read to generate a board ID when gpio_get_board_config() is called.
 */
#define GPIO_CFG_PORT	GPIO_PORT_B
#define GPIO_CFG_LOW	27
#define GPIO_CFG_HIGH	31

/*
 * Pins for Cart Configuration resistors.  These will be configured as input
 * pins and read to generate a cart ID when gpio_get_cart_config() is called.
 */
#define GPIO_CART_CFG_PORT	GPIO_PORT_B
#define GPIO_CART_CFG_LOW	2
#define GPIO_CART_CFG_HIGH	5

/* Each pin can take one of three different functions */
enum gpio_function {
	GPIO_GPIOFN	= 0,
	GPIO_ALT1,
	GPIO_ALT2,
	GPIO_RESERVED,
};

/* Each port has 32 pins */
enum gpio_pin {
	GPIO_PIN0 = 0, GPIO_PIN1, GPIO_PIN2, GPIO_PIN3, GPIO_PIN4, GPIO_PIN5,
	GPIO_PIN6, GPIO_PIN7, GPIO_PIN8, GPIO_PIN9, GPIO_PIN10, GPIO_PIN11,
	GPIO_PIN12, GPIO_PIN13, GPIO_PIN14, GPIO_PIN15, GPIO_PIN16, GPIO_PIN17,
	GPIO_PIN18, GPIO_PIN19, GPIO_PIN20, GPIO_PIN21, GPIO_PIN22, GPIO_PIN23,
	GPIO_PIN24, GPIO_PIN25, GPIO_PIN26, GPIO_PIN27, GPIO_PIN28, GPIO_PIN29,
	GPIO_PIN30, GPIO_PIN31,
};

/* Each pin can interrupt on one of four different events */
enum gpio_interrupt_mode {
	GPIO_IMODE_LOW_LEVEL		= 0x0,
	GPIO_IMODE_HIGH_LEVEL		= 0x1,
	GPIO_IMODE_FALLING_EDGE		= 0x2,
	GPIO_IMODE_RISING_EDGE		= 0x3,
};

/* Each pin can drive with configurable current */
enum gpio_current {
	GPIO_CURRENT_2MA		= 0x0,
	GPIO_CURRENT_4MA		= 0x1,
	GPIO_CURRENT_6MA		= 0x2,
	GPIO_CURRENT_8MA		= 0x3,
};

/* virtual name for each gpio port/pin function */
enum gpio_resource {
	HEADPHONE_JACK			= 0,
	LED_ENA				= 1,
	LCD_RESET			= 2,
	AUDIO_POWER			= 3,
	DPAD_UP				= 4,
	DPAD_DOWN			= 5,
	DPAD_RIGHT			= 6,
	DPAD_LEFT			= 7,
	BUTTON_A			= 8,
	BUTTON_B			= 9,
	SHOULDER_LEFT			= 10,
	SHOULDER_RIGHT			= 11,
	BUTTON_HOME			= 12,
	BUTTON_HINT			= 13,
	BUTTON_PAUSE			= 14,
	BUTTON_BRIGHTNESS		= 15,
	BUTTON_VOLUMEUP			= 16,
	BUTTON_VOLUMEDOWN		= 17,
	CARTRIDGE_DETECT		= 18,
	TOUCHSCREEN_X1			= 19,
	TOUCHSCREEN_Y1			= 20,
	TOUCHSCREEN_X2			= 21,
	TOUCHSCREEN_Y2			= 22,
	BUTTON_RED			= 23,
	GPIO_NUMBER_VALUES		= 24,
};

/*
 * Scratchpad Register usage
 */

#define SCRATCH_POWER_POS          0
#define SCRATCH_POWER_SIZE         2

#define SCRATCH_SHUTDOWN_POS       2
#define SCRATCH_SHUTDOWN_SIZE      1

#define SCRATCH_REQUEST_POS 	   3
#define SCRATCH_REQUEST_SIZE  	   3

#define SCRATCH_BOOT_IMAGE_POS     6
#define SCRATCH_BOOT_IMAGE_SIZE    2

#define SCRATCH_BOARD_ID_POS       8
#define SCRATCH_BOARD_ID_SIZE      5

#define SCRATCH_CART_ID_POS       13
#define SCRATCH_CART_ID_SIZE       4

#define SCRATCH_BOOT_SOURCE_POS   17
#define SCRATCH_BOOT_SOURCE_SIZE   3

#define SCRATCH_PANIC_POS	  20
#define SCRATCH_PANIC_SIZE         2

#define SCRATCH_USER_0_POS        22
#define SCRATCH_USER_0_SIZE       (32-SCRATCH_USER_0_POS)

/*
 * SCRATCHPAD Enums
 * Note that scratchpad register is cleared to zero on first power up
 */

/* Track power state */
enum scratch_power {                 // set by bootstrap, read by others
        SCRATCH_POWER_FIRSTBOOT = 0, // only seen in bootstrap
        SCRATCH_POWER_COLDBOOT  = 1, // batteries replaced
        SCRATCH_POWER_WARMBOOT  = 2, // second and subsequent boots
};

/* Shutdown semaphore.  Set by Linux shutdown to signal clean system shutdown */
enum scratch_shutdown {
        SCRATCH_SHUTDOWN_CLEAN = 0,  // set by Linux at clean shutdown
        SCRATCH_SHUTDOWN_DIRTY = 1,  // cleared by bootstrap at boot
};

/* Choose boot partition.  Used by REQUEST_BOOT and ACTUAL_BOOT bits to
 * indicate the preferred boot partition and the partition booted
 */
enum scratch_boot_image {
        SCRATCH_BOOT_IMAGE_RECOVERY = 0,   // boot recovery image
        SCRATCH_BOOT_IMAGE_PLAY     = 1,   // normal boot
        SCRATCH_BOOT_IMAGE_2        = 2,   // unused
        SCRATCH_BOOT_IMAGE_3        = 3,   // unused
};

/* Choose boot partition.  Used by REQUEST_BOOT and ACTUAL_BOOT bits to
 * indicate the preferred boot partition and the partition booted
 */
enum scratch_boot_source {
        SCRATCH_BOOT_SOURCE_UNKNOWN = 0,   // 
        SCRATCH_BOOT_SOURCE_NOR     = 1,   // 
        SCRATCH_BOOT_SOURCE_NAND    = 2,   // 
        SCRATCH_BOOT_SOURCE_UART    = 3,   // 
        SCRATCH_BOOT_SOURCE_USB     = 4,   // 
};

/* save boot source. */
enum scratch_request {
	SCRATCH_REQUEST_PLAY    = 0,  // Launch Play if possible
	SCRATCH_REQUEST_RETURN  = 1,  // Return to Play if possible
	SCRATCH_REQUEST_UPDATE  = 2,  // Enter recovery in update mode
	SCRATCH_REQUEST_BATTERY = 3,  // Enter play: battery failed
	SCRATCH_REQUEST_UNCLEAN = 4,  // Enter play: dirty shutdown
	SCRATCH_REQUEST_FAILED  = 5,  // Enter recovery in update mode: boot failed
	SCRATCH_REQUEST_SHORT   = 6,  // Enter play in short-circuit mode
	SCRATCH_REQUEST_TRAPDOOR= 7,  // Enter recovery in trapdoor mode
};

// Priority: _TRAPDOOR, _SHORT, _FAILED, _UNCLEAN, _BATTERY, _UPDATE, _RETURN, _PLAY

/* Configure a GPIO pin */

void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin,
		enum gpio_function f, unsigned char out_en,
		unsigned char pu_en, unsigned char val);

/* Set the pin function. */
int gpio_set_fn(enum gpio_port port, enum gpio_pin pin,
		 enum gpio_function f);

/* get the input value of the pin */
int gpio_get_val(enum gpio_port port, enum gpio_pin pin);

/* get the pullup resistor setting */
int gpio_get_pu(enum gpio_port port, enum gpio_pin pin);

#ifndef _LF1000_BOOTLOADER /* not building a bootloader */

#include <linux/interrupt.h>

/* Interrupt handler type for gpio interrupts.  Both the gpio_port and gpio_pin
 * are passed to the handler when the interrupt occurs.  The handler is
 * expected, at minimum, to handle the interrupt, clear the interrupt pending
 * bit, and return IRQ_HANDLED.  Drivers who handle multiple gpio pins are
 * permitted to inspect the gpio registers freely and handle any simultaneous
 * interrupts.  priv is passed to handler when an interrupt occurs.  It is
 * opaque to the gpio system.
 */
typedef irqreturn_t (*gpio_irq_handler_t)(enum gpio_port, enum gpio_pin,
					  void *);

/* Get the pin function */
int gpio_get_fn(enum gpio_port port, enum gpio_pin pin);

/* set or clear output enable.  Clearing output enable means this pin is an
 * input.
 */
int gpio_set_out_en(enum gpio_port port, enum gpio_pin pin, unsigned char en);

/* set or clear the pull-up enable */
void gpio_set_pu(enum gpio_port port, enum gpio_pin pin, unsigned char en);

/* set the output value of the pin */
int gpio_set_val(enum gpio_port port, enum gpio_pin pin, unsigned char en);

/* request an interrupt handler for a given pin. Returns -EBUSY if that pin
 * already has a handler.
 */
int gpio_request_irq(enum gpio_port port, enum gpio_pin pin,
		     gpio_irq_handler_t handler, void *priv);

/* request an interrupt handler for a given pin, with a normal irq_handler_t
 * routine called as a result.  Returns -EBUSY if that pin already has a 
 * handler. */
int gpio_request_normal_irq(enum gpio_port port, enum gpio_pin pin,
		irq_handler_t handler, void *priv);

/* free the irq requested using gpio_request_irq.  To prevent accidental
 * freeing of somebody else's gpio irq, the handler must match the one that was
 * passed to gpio_request_irq.
 */
void gpio_free_irq(enum gpio_port port, enum gpio_pin pin,
		   gpio_irq_handler_t handler);

/* get the interrupt mode for a given pin */
enum gpio_interrupt_mode gpio_get_int_mode(enum gpio_port port,
					   enum gpio_pin pin);

/* set the interrupt mode for a given pin */
void gpio_set_int_mode(enum gpio_port port, enum gpio_pin pin,
		       enum gpio_interrupt_mode mode);

/* toggle the interrupt mode for a pin.  If the mode is currently
 * IMODE_RISING_EDGE it becmoes IMODE_FALLING_EDGE and vice versa.  If the mode
 * is IMODE_HIGH_LEVEL it becomes IMODE_LOW_LEVEL
 */
void gpio_toggle_int_mode(enum gpio_port port, enum gpio_pin pin);

/* enable or disable interrupt for a given pin */
void gpio_set_int(enum gpio_port port, enum gpio_pin pin, unsigned char en);

/* get the interrupt enable bit for a given pin */
unsigned char gpio_get_int(enum gpio_port port, enum gpio_pin pin);

/* get interrupt enable bits for all 32 pins in a given port */
unsigned long gpio_get_int32(enum gpio_port port);

/* set the interrupt enable bits for all 32 pins in a given port.  Use this
 * function in conjunction with gpio_get_int32 to enable or disable
 * interrupts on many pins at a time.
 */
void gpio_set_int32(enum gpio_port port, unsigned long en);

/* clear the interrupt pending bit for a given pin */
void gpio_clear_pend(enum gpio_port port, enum gpio_pin pin);

/* get the interrupt pending bit for a given pin */
unsigned char gpio_get_pend(enum gpio_port port, enum gpio_pin pin);

/* get the interrupt pending bits for all pins in a given port */
unsigned long gpio_get_pend32(enum gpio_port port);

/* clear the interrupt pending bits for all pins in a given port.  Use this
 * function in conjunction with gpio_get_pend32 to clear all pending interrupts
 * at once.
 */
void gpio_clear_pend32(enum gpio_port port, unsigned long flag);

/* get power bits of scratch register */
u8 gpio_get_power_config(void);

/* get shutdown bits of scratch register */
enum scratch_shutdown gpio_get_shutdown_config(void);

/* set shutdown bits of scratch register */
void gpio_set_shutdown_config(enum scratch_shutdown);

/* get requested bits of scratch register */
enum scratch_request gpio_get_request_config(void);

/* set requested bits of scratch register */
void gpio_set_request_config(enum scratch_request);

/* get actual boot partition bits of scratch register */
enum scratch_boot_image gpio_get_boot_image_config(void);

/* set actual boot partition bits of scratch register */
void gpio_set_boot_image_config(enum scratch_boot_image);

/* get boot mode bits of scratch register */
enum scratch_boot_source gpio_get_boot_source_config(void);

/* get panic bits of scratch register */
int gpio_get_panic_config(void);

/* set panic bits of scratch register */
void gpio_set_panic_config(int);

/* have supercap? 1=yes */
int gpio_have_supercap(void);

/* have acorn board gpio layout? 1=yes */
int gpio_have_gpio_acorn(void);

/* have dev board gpio layout? 1=yes */
int gpio_have_gpio_dev(void);

/* have didj board gpio layout? 1=yes */
int gpio_have_gpio_didj(void);

/* have emerald board gpio layout? 1=yes */
int gpio_have_gpio_emerald(void);

/* have k2 board gpio layout? 1=yes */
int gpio_have_gpio_k2(void);

/* have tvout? 1=yes */
int gpio_have_tvout(void);

/* does system use up/down volume buttons? 1=yes */
int gpio_have_volume_buttons(void);

/* does system have a touchscreen? 1=yes */
int gpio_have_touchscreen(void);

/* read the Board Configuration resistors and return a Board ID.  Calling this
 * function will cause the Board Configuration pins to be configured as GPIO
 * input pins. */
u8 gpio_get_board_config(void);

/* get actual boot partition bits of scratch register */
u32 gpio_get_user_0_config(void);

/* set actual boot partition bits of scratch register */
void gpio_set_user_0_config(u32);

/* get gpio pin drive current setting */
unsigned long gpio_get_cur(enum gpio_port port, enum gpio_pin pin);

/* set the drive current for the gpio pin */
void gpio_set_cur(enum gpio_port port, enum gpio_pin pin, enum gpio_current cur);


/* get Power-down reason from ALIVE power scratch register value */
unsigned long gpio_get_scratch_power(void);

/* set Power-down reason in ALIVE power scratch register value */
void gpio_set_scratch_power(unsigned long value);

/* translate GPIO port for different boards */
int lf1000_l2p_port(enum gpio_resource logical_value);

/* translate GPIO pin for different boards */
int lf1000_l2p_pin(enum gpio_resource logical_value);

#ifdef CONFIG_LF1000_STRESS_TEST
#define POWER_ERASE     0
#define POWER_WRITE     1
#define CART_READ       2

void stress_config_power(void);
void stress_cut_power(void);
void stress_cut_cart(int cut);
#endif /* CONFIG_LF1000_STRESS_TEST */

#endif /* _LF1000_BOOTLOADER */

#endif /* LF1000_GPIO_H */
