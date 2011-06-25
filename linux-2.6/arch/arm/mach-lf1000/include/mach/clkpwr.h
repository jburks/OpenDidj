/* mach-lf1000/include/mach/clkpwr.h -- LF1000 Clock & Power Management Hardware 
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#ifndef _LF1000_CLKPWR
#define _LF1000_CLKPWR

#include <mach/platform.h> /* for CPU variant */

#define NUM_PLLS	2

/* Clock and Power Control Registers (offsets from LF1000_CLKPWR_BASE) */
#define CLKMODEREG		0x000
#define PLLSETREG0		0x004
#define PLLSETREG1		0x008
#define GPIOWAKEUPENB		0x040
#define RTCWAKEUPENB		0x044
#define GPIOWAKEUPRISEENB	0x048
#define GPIOWAKEUPFALLENB	0x04C
#define GPIOPEND		0x050
#define INTPENDSPAD		0x058
#define PWRRSTSTATUS		0x05C
#define INTENB			0x060
#define PWRMODE			0x07C
#define PADSTRENGTHGPIOAL	0x100
#define PADSTRENGTHGPIOAH	0x104
#define PADSTRENGTHGPIOBL	0x108
#define PADSTRENGTHGPIOBH	0x10C
#define PADSTRENGTHGPIOCL	0x110
#define PADSTRENGTHGPIOCH	0x114
#define PADSTRENGTHBUS		0x118

/* Clock Mode Register (CLKMODEREG) */
#define PLLPWDN1	30
#define CLKSELBCLK	24
#define CLKDIV1BCLK	20
#define CLKDIV2CPU0	6
#define CLKSELCPU0	4
#define CLKDIVCPU0	0

/* PLL0 Setting Register (PLLSETREG0) */
#define PDIV_0		18
#define MDIV_0		8
#define SDIV_0		0

/* PLL1 Setting Register (PLLSETREG1) */
#define PDIV_1		18
#define MDIV_1		8
#define SDIV_1		0

/* Power Mode Control Register (PWRMODE) */
#define CHGPLL		15
#define GPIOSWRSTENB	13
#define SWRST		12
#define LASTPWRMODE	4
#define CURPWRMODE	0

/* Interrupt Pending & Scratch Pad Register (INTPENDSPAD) */
#define BATFW		14
#define GPIORESETW	13
#define WATCHDOGRSTW	12
#define POWERONRSTW	11

#endif
