/*
 * audio_hal.h
 * Hardware specific defines for Audio Controller
 *
 * Scott Esters
 * LeapFrog Enterprises
 */

#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

typedef struct tagBUFFER {
	u16 words;	// words of data in buffer
	u16 *buffer;	// pointer to DMA safe buffer
} BUFFER;

typedef struct tagAUDIO_REGS {
	volatile u16 AC97_CTRL;
	volatile u16 AC97_CONFIG;
	volatile u16 I2S_CTRL;
	volatile u16 I2S_CONFIG;
	volatile u16 AUDIO_BUFF_CTRL;
	volatile u16 AUDIO_BUFF_CONFIG;
	volatile u16 AUDIO_IRQ_ENA;
	volatile u16 AUDIO_IRQ_PEND;
	volatile u16 AC97_CODEC_ADDR;
	volatile u16 AC97_CODEC_WDATA;
	volatile u16 AC97_CODEC_RDATA;
	volatile u16 AUDIO_STATUS0;
	volatile u16 AUDIO_STATUS1;
	volatile u8 reserved1[0x66];
	volatile u16 AC97_CODEC_REGISTER[0x40];
	volatile u16 reserved2[0x160];
	volatile u32 CLKENB;
	volatile u16 CLKGEN0;
		 u16 reserved3;
	volatile u16 CLKGEN1;
} AUDIO_REGS;


/*********************
* AC97_CTRL Register *
*********************/
#define WARM_RST		3
#define ACLINK_RUN		2
#define CTRL_RST		1
#define COLD_RST		0

/***********************
* AC97_CONFIG Register *
***********************/
#define ADC2_EN			7
#define ADC1_EN			6
#define MIC_EN			5
#define PCMI_EN			4
#define SPDIF_EN		3
#define CLFE_EN			2
#define REAR_EN			1
#define FRONT_EN		0

/*********************
 * I2S_CTRL Register *
 ********************/
 #define I2SLINK_RUN		1
 #define I2S_EN			0

/**********************
* I2S_CONFIG Register *
**********************/
#define SYNC_PERIOD_32		0x00
#define SYNC_PERIOD_48		0x10
#define SYNC_PERIOD_64		0x20
#define SYNC_PERIOD_96		0x30
#define SYNC_PERIOD_MASK	(0x3 << 4)

#define LOOP_BACK		3
#define I2SIEN			2
#define I2SO_EN			1
#define MST_SLV			0

/***************************
* AUDIO_BUFF_CTRL Register *
***************************/
#define ADC2BUF_EN		5
#define ADC1BUF_EN		4
#define MICBUF_EN		3
#define PCMIBUF_EN		2
#define SPDIFBUF_EN		1
#define PCMOBUF_EN		0

/*****************************
* AUDIO_BUFF_CONFIG Register *
*****************************/
#define PI_WIDTH_16 		(0<<4)
#define PI_WIDTH_18		(1<<4)
#define PI_WIDTH_20		(2<<4)
#define PI_WIDTH_24		(3<<4)
#define PI_WIDTH_MASK		(3<<4)

#define SP_WIDTH_16		(0<<2)
#define SP_WIDTH_18		(1<<2)
#define SP_WIDTH_20		(2<<2)
#define SP_WIDTH_24		(3<<2)
#define SP_WIDTH_MASK		(0x3<<2)

#define PO_WIDTH_16		0
#define PO_WIDTH_18		1
#define PO_WIDTH_20		2
#define PO_WIDTH_24		3
#define PO_WIDTH_MASK		(0x3<<0)

/*************************
* AUDIO_IRQ_ENA Register *
*************************/
#define RDDONE_IRQ_EN		6
#define ADC2OVR_IRQ_EN		5
#define ADC1OVR_IRQ_EN		4
#define MICOVR_IRQ_EN		3
#define PIOVER_IRQ_EN		2
#define SPUDR_IRQ_EN		1
#define POUDR_IRQ_EN		0

/**************************
* AUDIO_IRQ_PEND Register *
**************************/
#define RDDONE_PEND		6
#define ADC2OVR_PEND		5
#define ADC1OVR_PEND		4
#define MICROVR_PEND		3
#define PIOVR_PEND		2
#define SPUDR_PEND		1
#define POUDR_PEND		0

/***************************
* AC97_CODEC_ADDR Register *
***************************/

/****************************
* AC97_CODEC_WDATA Register *
****************************/

/****************************
* AC97_CODEC_RDATA Register *
****************************/

/*************************
* AUDIO_STATUS0 Register *
*************************/
#define CODEC_BUSY		11
#define CODEC_RDDONE		10
#define CODEC_WRDONE		9
#define CODEC_RDY		8
#define AC_FSM_IDLE		(1<<3)
#define AC_FSM_INITIALIZE	(2<<3)
#define AC_FSM_ACTIVE		(4<<3)
#define AC_FSM_POWERDOWN	(8<<3)
#define AC_FSM_WARMRESET	(16<<3)
#define I2S_FSM_NONE		0
#define I2S_FSM_IDLE		1
#define I2S_FSM_READY		2
#define I2S_FSM_RUN		4

/**************************
 * AUDIO_STATUS1 Register *
 *************************/
#define ADC2BUF_RDY		5
#define ADC1BUF_RDY		4
#define MICBUF_RDY		3
#define PIBUF_RDY		2
#define SPBUF_RDY		1
#define POBUF_RDY		0

/*******************
 * CLKENB Register *
 ******************/
#define CLKENB_PCLKMODE		3
#define CLKENB_CLKGENENB	2

/********************
 * CLKGEN0 Register *
 *******************/
#define CLKGEN0_CLKDIV0			4
#define CLKGEN0_CLKDIV0_MASK		(0x3F<<4)
#define CLKGEN0_CLKSRCSEL0_PLL0		(0<<1)
#define CLKGEN0_CLKSRCSEL0_PLL1		(1<<1)
#ifdef CPU_MF2530F
#define CLKGEN0_CLKSRCSEL0_PLL2		(2<<1)
#endif
#define CLKGEN0_CLKSRCSEL0_BITCLOCK	(3<<1)
#define CLKGEN0_CLKSRCSEL0_INV_BITCLOCK	(4<<1)
#define CLKGEN0_CLKSRCSEL0_AVCLOCK	(5<<1)
#define CLKGEN0_CLKSRCSEL0_INV_AVCLOCK	(6<<1)
#define CLKGEN0_CLKSRCSEL0_NONE		(7<<1)
#define CLKGEN0_CLKSRCSEL0_MASK		(0x3<<1)
#define CLKGEN0_OUTCLKINV0		0

/********************
 * CLKGEN1 Register *
 *******************/
#define CLKGEN1_OUTCLKENB1		15
#define CLKGEN1_CLKDIV1			4
#define CLKGEN1_CLKDIV1_MASK		(0x3F<<4)
#define CLKGEN1_CLKSRCSEL1_PLL0		(0<<1)
#define CLKGEN1_CLKSRCSEL1_PLL1		(1<<1)
#ifdef CPU_MF2530F
#define CLKGEN1_CLKSRCSEL1_PLL2		(2<<1)
#endif
#define CLKGEN1_CLKSRCSEL1_BITCLOCK	(3<<1)
#define CLKGEN1_CLKSRCSEL1_INV_BITCLOCK	(4<<1)
#define CLKGEN1_CLKSRCSEL1_AVCLOCK	(5<<1)
#define CLKGEN1_CLKSRCSEL1_INV_AVCLOCK	(6<<1)
#define CLKGEN1_CLKSRCSEL1_CLKGEN0	(7<<1)
#define CLKGEN1_CLKSRCSEL1_MASK		(0x3<<1)
#define CLKGEN1_OUTCLKINV1		0


/* LF1000 DMA Registers */

typedef struct tagDMA_REGS {
	volatile u32 DMASRCADDR;
	volatile u32 DMADSTADDR;
	volatile u16 DMALENGTH;
	volatile u16 DMAREQID;
	volatile u32 DMAMODE;
} DMA_REGS;

/********************
 * DMAMODE Register *
 *******************/
 
 #define DMA_RUN		19
 #define DMA_INTENB		18
 #define DMA_INTPEND		17
 #define DMA_BUSY		16
 #define DSTNOTREQCHK		13
 #define DSTNOTINC		12
 #define DESTIMODE		10
 #define DSTIOSIZE_BYTE		(0<<8)
 #define DSTIOSIZE_HWORD	(1<<8)
 #define DSTIOSIZE_WORD		(2<<8)
 #define SRCNOTREQCHK		5
 #define SRCNOTINC		4
 #define PACKMODE		3
 #define SRCIOMODE		2
 #define SRCIOSIZE_BYTE		0
 #define SRCIOSIZE_HWORD	1
 #define SRCIOSIZE_WORD		2
 
/* DMAREQID options */
#define DMAREQID_PCM		24

#endif
