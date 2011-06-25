/*
 * mach-lf1000/include/mach/platform.h
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef _LF1000_PLATFORM_H
#define _LF1000_PLATFORM_H

/* macro to get at IO space when running virtually */
#define	IO_ADDRESS_OFF		0xf0000000
#define IO_ADDRESS(x)		(((x) & 0x00ffffff) + (((x) >> 4) & 0x0ff00000) + IO_ADDRESS_OFF)

/*
 * Platform CPU Selection
 */

#if	defined	CONFIG_MACH_ME_LF1000
	#define	CPU_LF1000
#elif	defined	CONFIG_MACH_LF_LF1000
	#define	CPU_LF1000
#else
	#error NO Platform Selected
#endif

/* ------------------------------------------------------------------------
 *  Lf1000 Registers
 * ------------------------------------------------------------------------
 * FIXME: these shoudn't be needed, clean up this file accordingly?
 */
#define LF1000_SYS_IO				0xc0000000
#define LF1000_SYS_BASE				0xc0000000
#define LF1000_ETH_BASE				0x88000000

/////////////////////////////////  PLL  //////////////////////////////////////////////

#ifndef	__ASSEMBLY__
struct bpsr {
    volatile unsigned int	reserved:8;
    volatile unsigned int	pdiv:8;
    volatile unsigned int	mdiv:8;
    volatile unsigned int	sdiv:8;
};

union pllsetreg {
    struct bpsr	bits __attribute__((packed));
    unsigned int		word;
};

struct lf1000_clock {
    volatile unsigned int	clkmodereg;
    volatile unsigned int	pllsetreg0;
    volatile unsigned int	pllsetreg1;
//    volatile union pllsetreg	pllsetreg1;
    volatile unsigned int	reserved0;
    volatile unsigned int	reserved1[0xc];
    volatile unsigned int	gpiowakeupenb;
    volatile unsigned int	rtcwakeupenb;
    volatile unsigned int	gpiowakeupriseenb;
    volatile unsigned int	gpiowakeupfallenb;
    volatile unsigned int	gpiopend;
    volatile unsigned int	reserved2;
    volatile unsigned int	intpendspad;
    volatile unsigned int	pwrrststatus;
    volatile unsigned int	intenb;
    volatile unsigned int	reserved3[0x6];
    volatile unsigned int	pwrmode;
};

#endif

//------------- SET ONLY THIS DEFINITIONS -------------//

#define	CRYSTAL_FREQ_HZ	27000000	/* 27 MHz hardware installed in Hz */

/*
 * Choose CPU speed to match Audio requirements.  Audio base rate is 32KHz
 * times the i2s oversampling rate of 256.  So CPU rate is a multiple of
 * of 32,000 Hz * 256 i2s rate, or 8,192,000 Hz.  Development rates have
 * been 8.192 Mhz * 47 = 385.024 Mhz and 8.192 Mhz * 48 = 393.216 Mhz.
 * Also 532.480 Mhz (8.192 Mhz * 65) is supported experimentally with a
 * higher CPU core voltage.
 */

// only one CPU_SPEED should be defined
#if defined CONFIG_CPU_SPEED_385024000
#define PLL0_OUT_HZ	385024000	/* desired PLL freq in Hz*/
#elif defined CONFIG_CPU_SPEED_393216000
#define PLL0_OUT_HZ	393216000	/* desired PLL freq in Hz*/
#elif defined CONFIG_CPU_SPEED_532480000
#define PLL0_OUT_HZ	532480000	/* desired PLL freq in Hz*/
#else
#error CPU SPEED not defined
#endif

#define PLL1_OUT_HZ	144000000	/* desired PLL frequency in Hz	*/

//------------- SET ONLY THIS DEFINITIONS -------------//

#define LF1000_CLOCK_BASE	(LF1000_SYS_IO+0xf000)	/* clock controller */


#define	PLL0		0
#define	PLL1		1

#define	MAX_PDIV	63
#define	MIN_PDIV	1
#define	MAX_MDIV	1023
#define	MIN_MDIV	56
#define	MAX_SDIV	5
#define	MIN_SDIV	0

#define	PLL_MUL( n)	( 1)

#define  MAX_VCOF_HZ(n)((n==PLL0)?600000000:  \
			 (n==PLL1)?600000000:0)

// choose PLL post-scaler based on output frequency
#define	SDIV_HZ( n, f)	(((f) > (MAX_VCOF_HZ(n)/ 2))?0:	\
			 ((f) > (MAX_VCOF_HZ(n)/ 4))?1:	\
			 ((f) > (MAX_VCOF_HZ(n)/ 8))?2:	\
			 ((f) > (MAX_VCOF_HZ(n)/16))?3:0)

#define clock_p		((struct lf1000_clock*)(IO_ADDRESS( LF1000_CLOCK_BASE)))

/////////////////////// CPU, SDRAM, BCLK & PCLK //////////////////////////////////////
#define	CPU_PLL		PLL0
#define CPU_DIV		1
#define HCLK_DIV	3
#define	SDRAM_PLL	PLL1
#define BCLK_PLL	PLL0
#define BCLK_DIV	3
#define	PCLK_PLL	BCLK_PLL

/*
 * Memory Controller Settings
 */
#define	LF1000_SYS_IO_VIRT		IO_ADDRESS(LF1000_SYS_IO)
#define LF1000_TSACC0			0	/* bits 3:0 */
#define LF1000_TSACC1			1	/* bits 7:4 */
#define LF1000_NAND			11
#define	LF1000_ETH			2

/* TODO: replace with mem_controller.h */
#define	LF1000_MEMBW_OFF		0x00015800
#define	LF1000_MEMBW_SR1BW	(1<<LF1000_ETH) /*XXX*/
#define	LF1000_MEMTIMEACS_OFF		0x00015804
#define	LF1000_MEMTIMECOS_OFF		0x00015808
#define	LF1000_MEMTIMEACCL_OFF		0x0001580c
#define	LF1000_MEMTIMEACCH_OFF		0x00015810
#define	LF1000_MEMTIMESACCL_OFF		0x00015814
#define	LF1000_MEMTIMECOH_OFF		0x00015824
#define	LF1000_MEMTIMECAH_OFF		0x00015828
#define	LF1000_MEMTIMEBURSTL_OFF	0x0001582c
#define	LF1000_MEMWAIT_OFF		0x00015834

#define	mrs_read(off)		(*((ulong*)(LF1000_SYS_IO_VIRT+(off))))
#define	mrs_write(off, val)	(*((ulong*)(LF1000_SYS_IO_VIRT+(off))) = (val))

#define	MASK_MRS(size)			((1<<size)-1)

// set Memory Register Static
#define	SET_MRS( num, size, off, val)				\
	    mrs_write( off, test = (mrs_read( off) &		\
		(     ~(MASK_MRS( size)<<(size*num)))) |	\
		((val & MASK_MRS( size))<<(size*num)));

/*****************************************************************************
 * LF1000 Devices Memory Map and IRQ Assignments                             *
 *****************************************************************************/

/*
 * NOR Flash -- Note boot depends on ATAP switch settings:
 *
 *Didj|Acorn|      |                |                |            |   |
 * SW2|SW16 |Mode  |Didj            |Acorn/Emerald   |Signal      |J13|TP
 * S1 |     |Open  |Onboard NAND/NOR|Cartridge NAND  |CfgSELCS    |  1|30
 *    |     |Closed|Cartridge NAND  |Onboard NAND/NOR|            |   |
 * S2 | S2  |Open  |not used        |Onboard NOR     |CfgBOOTMODE1|  2|31
 *    |     |Closed|Boot NAND/UART  |Boot NAND/UART  |            |   |
 * S3 | S1  |Open  |Boot from NAND  |Boot from NAND  |CfgBOOTMODE0|  3|32
 *    |     |Closed|Boot from UART  |Boot from UART  |            |   |
 * S4 | S3  |Open  |unused keep Open|Boot NAND/UART  |CfgSHADOW   |  4|33
 *    |     |Closed|not used        |Boot from NOR   |            |   |
 *
 * Acorn/Emerald UART BOOT -- SW16 S1=ON  S2=ON  S3=OFF
 *               NAND BOOT -- SW16 S1=OFF S2=ON  S3=OFF
 *               NOR  BOOT -- SW16 S1=ON  S2=OFF S3=ON
 */


#define LF1000_NOR_FLASH_BASE_LOW	0x00000000
#define	LF1000_NOR_FLASH_BASE_HIGH0	0x80000000
#define	LF1000_NOR_FLASH_BASE_HIGH1	0x84000000

#define LF1000_NOR_FLASH_SIZE		( 512 * 1024 )
#define LF1000_NOR_MFGDATA_SIZE		( 4096 )
#define LF1000_NOR_BOOT_SIZE ( LF1000_NOR_FLASH_SIZE - 2*LF1000_NOR_MFGDATA_SIZE )

/* DMA Controller */
#define LF1000_DMA_BASE		0xC0000000
#define LF1000_DMA_IRQ		3

/* Interrupt controller */
#define LF1000_IC_BASE		0xC0000800	

/* Timers */
#define LF1000_TIMER0_BASE	0xC0001800
#define LF1000_TIMER0_IRQ	4
#define LF1000_TIMER1_BASE	0xC0001880
#define LF1000_TIMER1_IRQ	11
#define LF1000_TIMER2_BASE	0xC0001900
#define LF1000_TIMER2_IRQ	15
#define LF1000_TIMER3_BASE	0xC0001980
#define LF1000_TIMER3_IRQ	21
#define LF1000_TIMER4_BASE	0xC0001A00
#define LF1000_TIMER4_IRQ	43

/* Display Controller (DPC) */
#define LF1000_DPC_BASE		0xC0003000
#define LF1000_DPC_END		0xC00031C8

/* Multi-Layer Controller (MLC) */
#define LF1000_MLC_BASE		0xC0004000
#define LF1000_MLC_END		0xC00041C0

/* Analog to Digital Converter (ADC) */
#define LF1000_ADC_BASE		0xC0005000
#define LF1000_ADC_END		0xC0005040
#define LF1000_ADC_IRQ		25

/* Serial Peripheral Interface (SPI) */
#define LF1000_SPI0_BASE	0xC0007800
#define LF1000_SPI0_END		0xC0007844
#define LF1000_SPI0_IRQ		12
#define LF1000_SPI1_BASE	0xC0008000
#define LF1000_SPI1_END		0xC0008044
#define LF1000_SPI1_IRQ		39
#define LF1000_SPI2_BASE	0xC0008800
#define LF1000_SPI2_END		0xC0008844
#define LF1000_SPI2_IRQ		40

/* SD/MMC Controller */
#define LF1000_SDIO0_BASE	0xC0009800
#define LF1000_SDIO0_END	0xC0009FC4
#define LF1000_SDIO0_IRQ	14
#define LF1000_SDIO1_BASE	0xC000C800
#define LF1000_SDIO1_END	0xC000CFC4
#define LF1000_SDIO1_IRQ	42

/* GPIO */
#define LF1000_GPIO_BASE	0xC000A000
#define LF1000_GPIO_END		0xC000A0E4
#define LF1000_GPIO_IRQ		13

/* Pulse Width Modulator (PWM) */
#define LF1000_PWM_BASE		0xC000C000
#define LF1000_PWM_END		0xC000C044

/* AC97 and I2S (audio) */
#define LF1000_AUDIO_BASE	0xC000D800
#define LF1000_AUDIO_END	0xC000DBC8
#define LF1000_AUDIO_IRQ	24

/* I2C */
#define LF1000_I2C0_BASE	0xC000E000
#define LF1000_I2C0_END		0xC000E028
#define LF1000_I2C0_IRQ		32
#define LF1000_I2C1_BASE	0xC000E800
#define LF1000_I2C1_END		0xC000E828
#define LF1000_I2C1_IRQ		33

/* CLOCK and POWER */
#define LF1000_CLKPWR_BASE	0xC000F000
#define LF1000_CLKPWR_END	0xC000F07F

/* RTC controller */
#define LF1000_RTC_BASE		0xC000F080
#define LF1000_RTC_END		0xC000F097
#define LF1000_RTC_IRQ		31

/* IDCT Macro Block Decoder */
#define LF1000_IDCT_BASE	0xC000F800
#define LF1000_IDCT_END		0xC000FFC4

/* GPIO drive current control registers */
#define LF1000_GPIOCURRENT_BASE	0xC000F100
#define LF1000_GPIOCURRENT_END	0xC000F11B

/* MCU-Y Memory Controller */
#define LF1000_MCU_Y_BASE	0xC0014800
#define LF1000_MCU_Y_END	0xC0014811
#define LF1000_MEMCFG		0x00
#define LF1000_MEMTIME0		0x02
#define LF1000_MEMTIME1		0x04
#define LF1000_MEMREFRESH	0x08
#define LF1000_MEMCONTROL	0x0A
#define LF1000_MEMCLKDELAY	0x0C
#define LF1000_MEMDQSOUTDELAY	0x0E
#define LF1000_MEMDQSINDELAY	0x10

/* MCU-S Memory Controller */
#define LF1000_MCU_S_BASE	0xC0015800
#define LF1000_MCU_S_END	0xC001587C

/* UART */
#define LF1000_UART0_BASE	0xC0016000
#define LF1000_UART0_END	0xC0016044
#define LF1000_UART0_IRQ	10
#define LF1000_UART1_BASE	0xC0016080
#define LF1000_UART1_END	0xC00160C4
#define LF1000_UART1_IRQ	34
#define LF1000_UART2_BASE	0xC0016800
#define LF1000_UART2_END	0xC0016844
#define LF1000_UART2_IRQ	35
#define LF1000_UART3_BASE	0xC0016880
#define LF1000_UART3_END	0xC00168C4
#define LF1000_UART3_IRQ	36

/* HDC (USB) */
#define LF1000_UHC_BASE		0xC000D000
#define LF1000_UHC_END		0xC000D0C8
#define LF1000_UHC_IRQ		28

/* UDC (USB) */
#define LF1000_UDC_BASE		0xC0018000
#define LF1000_UDC_END		0xC00188C8
//#define LF1000_UDC_END		0xC0018880
#define LF1000_UDC_IRQ		20

/* Alive GPIO on LF1000 */
#define LF1000_ALIVE_BASE	0xC0019000
#define LF1000_ALIVE_END	0xC0019018

/* 3D Engine Control (GA3D) */
#define LF1000_GA3D_BASE	0xC001A000
#define LF1000_GA3D_END		0xC001BFFF

/* Processor ID (ECID) */
#define LF1000_ECID_BASE	0xC001F800

#define LF1000_3DGE_BASE	0xE0000000

/* Extint 0-1 */
#define	LF1000_EXTINT0		8
#define	LF1000_EXTINT1		9

///////////////////////////////// TIMER //////////////////////////////////////////////
/* FIXME: this needs to go away, see timer.h */

#ifndef	__ASSEMBLY__
struct lf1000_timer {
    volatile unsigned int	tmrcount;
    volatile unsigned int	tmrmatch;
    volatile unsigned int	tmrcontrol;
    volatile unsigned int	tmrreserved[0x0d];
    volatile unsigned int	tmrclkenb;
    volatile unsigned int	tmrclkgen;
};

struct lf1000_timer_init {
    struct lf1000_timer*	timer_pnt;
    int			 	irq_number;
};

#endif

//------------- SET ONLY THIS DEFINITIONS -------------//

#define	TIMER_PLL		PLL1		/* timer clock source is PLL1 */

#define	LF1000_SYS_TIMER	0		/* system   timer is TIMER0   */
#define LF1000_SYS_TIMER_IRQ	LF1000_TIMER0_IRQ
#define	LF1000_FREE_TIMER	1		/* free run timer is TIMER1   */

#define LF1000_INTERTICK_TIMER	2		/* Time since SYS_TIMER tick  */
						/* reset in SYS_TIMER isr     */

#define	LF1000_INTERVAL_IN_USEC	(USEC_PER_SEC/HZ) /* system timer cycle */

#define	TIMER_FREE_RUN		0xffffffff
#define	CLKDIV			32		/* timer predivider */
#define	SELTCLK			0x03		/* TCLK/1 */

#define	SELTCLK_MASK		0x03 /*XXX: duplicate? */
#define	CLKDIVR			((CLKDIV<1)?0:(CLKDIV>256)?255:(CLKDIV-1))

#define	TIMER_DIV		((1<<((SELTCLK+1)&SELTCLK_MASK))*CLKDIV)
#define TICKS_PER_mSEC		(get_pll_freq(TIMER_PLL)/TIMER_DIV/1000)
#define TICKS_PER_uSEC          (get_pll_freq(TIMER_PLL)/TIMER_DIV/1000000)
#define	TIMER_SYS_TICK	\
	(TICKS_PER_mSEC * (LF1000_INTERVAL_IN_USEC / 1000 /* for mSec */))

#define TIMER_INTERVAL		TIMER_SYS_TICK
#define TIMER_RELOAD		(TIMER_INTERVAL)

#define	TCLK_MAX_HZ		50000000	/* maximum allowed TCLK freq */

#define	TIMER_PLL_OUT_HZ	((TIMER_PLL == PLL0)? PLL0_OUT_HZ:	\
				 (TIMER_PLL == PLL1)? PLL1_OUT_HZ:	\
							(TCLK_MAX_HZ+1))

#if ((TIMER_PLL_OUT_HZ/TIMER_DIV) > TCLK_MAX_HZ)
    #error	Maximum allowed Timer Frequency is 50 MHz !!!
#endif

#undef TIMER_PLL_OUT_HZ		// used just for checking max freq

/////////////////////////////// FUNCTIONS /////////////////////////////////////

#ifndef	__ASSEMBLY__
// PLL
extern void			lf1000_clock_init( void);
extern int			lf1000_CalcDivider( unsigned int pll_khz,
						   unsigned int desired_hz);
extern unsigned int		get_pll_div( int pll,int fin_hz,int fout_hz);
extern int			get_pll_freq( unsigned int pll);
// CPU
extern unsigned int		get_cpu_freq( void);
extern void			calc_freq_ratio( unsigned long * loops);
// Memory
extern unsigned int		lf1000_is_shadow(void);
// Timer
extern struct lf1000_timer*	get_sys_timer_pnt( void);
extern struct lf1000_timer*	get_timer_pnt( int timer);

extern void			clear_timer_irq( int irq);
extern int			get_timer_irq( int timer);
extern unsigned long		get_timer_cnt( int timer);
extern unsigned long		get_timer_match( int timer);
//extern unsigned long		get_timer_div( int timer);
extern int			get_timer_freq( int timer);
extern unsigned int 		get_cpu_freq(void);
extern void	 		set_cpu_freq(unsigned int hertz);

// UART
extern void			set_uart_baud( int uart, int baud);
#endif

#define MAX_TIMER                       2
#define MAX_PERIOD                      699050

#endif

