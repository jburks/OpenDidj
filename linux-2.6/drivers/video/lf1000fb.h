/*
 * drivers/video/lf1000fb.h
 *
 * Framebuffer support for the LF1000/Pollux SoC.
 * 
 * Enabled for Didj and Leapster Explorer by nirvous
 * 
 * Greetz and many thanks to zucchini for his valuable help and input, 
 * as well as to Claude, GrizzlyAdams, jburks, jkent, losinggeneration, 
 * MostAwesomeDude, NullMoogleCable, PhilKll, prpplague, ReggieUK, 
 * and everyone on #Didj (irc.freenode.org).	 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

/*
 * Settings
 */
#define X_RESOLUTION		320
#define Y_RESOLUTION		240

#define PALETTE_CLEAR		0x80000000
#define MLCCONTROL0			0x24 //0x4024
#define MLCHSTRIDE0			0x28 //0x4028
#define MLCVSTRIDE0			0x2C //0x402C
#define MLCCONTROL1         0x58 //0x4058
#define MLCHSTRIDE1			0x5C //0x405C
#define MLCVSTRIDE1			0x60 //0x4060


#define MLCBGCOLOR			0x8 //0x4008
#define DIRTYFLAG			4
#define LAYERENB			5				
#define FORMAT				16	/* see table 20-5 */


//#define BGCOLOR				0xFFFFFF  //RGBCODE
#define BYTESPP				2

#define BITSPP				BYTESPP*8
#define VISUALTYPE			FB_VISUAL_TRUECOLOR //FB_VISUAL_PSEUDOCOLOR, FB_VISUAL_TRUECOLOR
//#define FORMATCODE			0x4432

/* Formats:
 * 	RGB565		= 0x4432, 
	BGR565		= 0xC432, 
	XRGB1555	= 0x4342,
	XBGR1555	= 0xC342,
	XRGB4444	= 0x4211,
	XBGR4444	= 0xC211,
	XRGB8332	= 0x4120,
	XBGR8332	= 0xC120,
	ARGB1555	= 0x3342,
	ABGR1555	= 0xB342,
	ARGB4444	= 0x2211,
	ABGR4444	= 0xA211,
	ARGB8332	= 0x1120,
	ABGR8332	= 0x9120,
	RGB888		= 0x4653,
	BGR888		= 0xC653,
	XRGB8888	= 0x4653, 
	XBGR8888	= 0xC653, 
	ARGB8888	= 0x0653,
	ABGR8888	= 0x8653,
	PTRGB565	= 0x443A
 * */

//normally in drivers/lf1000/mlc_hal.h

#define MLCCONTROLT             0x00
#define MLCSCREENSIZE		    0x04
#define MLCADDRESS0    			0x38
#define MLCCONTROL1				0x58
#define MLCHSTRIDE1				0x5C
#define MLCVSTRIDE1				0x60
#define MLCADDRESS1				0x6C
#define MLCCONTROL2				0x7C
#define MLCADDRESS2     		0x8C    /* MLCADDRESS3 in databook */
#define MLCVSTRIDE2				0x80	/* MLCVSTRIDE2 in databook */
#define MLCSTRIDECB   		    0x98
#define MLCSTRIDECR     		0x9C

#define MLCTOPBOTTOM0			0x10
#define MLCLEFTRIGHT0			0x0C
#define MLCLEFTOP0				0x0C//????
#define MLCRIGHTBOTTOM0			0x10//????
#define MLCTOPBOTTOM1			0x44
#define MLCLEFTRIGHT1			0x40
#define MLCLEFTOP1				0x2C //????
#define MLCRIGHTBOTTOM1			0x30 //????

#define MLCTOPBOTTOM2			0x78
#define MLCLEFTRIGHT2			0x74
#define MLCLEFTOP2				0x4C //????
#define MLCRIGHTBOTTOM2			0x50 //?????


#define MLCTPCOLOR2				0x84 //databook error says tpcolor3
#define MLCTPCOLOR1				0x64
#define MLCTPCOLOR0				0x30

#define MLCINVCOLOR0			0x24
#define MLCINVCOLOR1			0x44
#define MLCINVCOLOR2			0x64

#define MLCHSCALE				0x9C
#define MLCVSCALE				0xA0


#define MLCLEFTRIGHT0_0			0x14
#define MLCLEFTRIGHT1_0			0x48

#define MLCTOPBOTTOM0_0			0x18
#define MLCTOPBOTTOM1_0			0x4C

#define MLCADDRESSCB			0x8C
#define MLCADDRESSCR			0x90

/* MLC RGB Layer n Control Register (MLCCONTROLn) */
#define GRP3DENB                8       /* set layer as output of 3D core */
#define LAYERENB                5       /* enable the layer */
#define PALETTEPWD              15      /* layer n palette table on/off */
#define PALETTESLD              14      /* layer n palette table sleep mode */
#define MLC_NUM_LAYERS			3
#define MLC_VIDEO_LAYER		(MLC_NUM_LAYERS-1)
#define LOCKSIZE		12	/* memory read size */
#define BLENDENB		2
#define INVENB			1
#define TPENB			0

/* MLC TOP CONTROL REGISTER (MLCCONTROLT) */
#define DITTYFLAG               3
#define PIXELBUFFER_SLD         10      /* pixel buffer sleep mode */
#define PIXELBUFFER_PWD         11      /* pixel buffer power on/off */
#define MLCENB                  1
#define PRIORITY		8
#define FIELDENB		0

/* MLC SCREEN SIZE REGISTER (MLCSCREENSIZE) */
#define SCREENHEIGHT		16
#define SCREENWIDTH		0

/* MLC RGB Layer n Left Right Register (MLCLEFTRIGHTn) */
#define LEFT			16
#define RIGHT			0

/* MLC RGB Layer n Top Bottom Register (MLCTOPBOTTOMn) */
#define TOP			16
#define BOTTOM			0

/* MLC RGB LAYER n TRANSPARENCY COLOR REGISTER (MLCTPCOLORn) */
#define ALPHA			28
#define TPCOLOR			0

/* MLC RGB LAYER n INVERSION COLOR REGISTER (MLCINVCOLORn) */
#define INVCOLOR		0

/* MLC Video layer Horizontal Scale (MLCHSCALE) Register */

#define HFILTERENB		28 /* bilinear filter enable */
#define HSCALE			0  /* horizonal scale ratio */

/* MLC Video layer Vertical Scale (MLCVSCALE) Register */

#define VFILTERENB		28 /* bilinear filter enable */
#define VSCALE			0  /* vertical scale ratio */


/* MLC RGB Layer n Invalid Area 0 left right register (MLCLEFTRIGHT0_0) */
#define INVALIDENB		28
#define INVALIDLEFT		16
#define INVALIDRIGHT		0

/* MLC RGB Layer n Invalid Area 0 Top Bottom Register */
#define INVALIDTOP		16
#define INVALIDBOTTOM		0

/* MLC CLOCK GENERATION ENABLE REGISTER (MLCCLKENB) */
#define MLCCLKENB	0x3C0
#define _PCLKMODE		3 /* FIXME: cleaner way to fix namespace? */
#define BCLKMODE		0

//enum {
//	VID_PRIORITY_FIRST	= 0,   /* video  > Cursor > Window > 3D */
//	VID_PRIORITY_SECOND	= 1,   /* Cursor > video  > Window > 3D */
//	VID_PRIORITY_THIRD	= 2,   /* Cursor > Window > video  > 3D */
//	VID_PRIORITY_INVALID	= 3,
//};

//enum RGBFMT
//{
	//VID_RGBFMT_R5G6B5	= 0x4432,   /* 16bpp { R5, G6, B5 }. */
	//VID_RGBFMT_B5G6R5	= 0xC432,   /* 16bpp { B5, G6, R5 }. */

	//VID_RGBFMT_X1R5G5B5	= 0x4342,   /* 16bpp { X1, R5, G5, B5 }. */
	//VID_RGBFMT_X1B5G5R5	= 0xC342,   /* 16bpp { X1, B5, G5, R5 }. */
	//VID_RGBFMT_X4R4G4B4	= 0x4211,   /* 16bpp { X4, R4, G4, B4 }. */
	//VID_RGBFMT_X4B4G4R4	= 0xC211,   /* 16bpp { X4, B4, G4, R4 }. */
	//VID_RGBFMT_X8R3G3B2	= 0x4120,   /* 16bpp { X8, R3, G3, B2 }. */
	//VID_RGBFMT_X8B3G3R2	= 0xC120,   /* 16bpp { X8, B3, G3, R2 }. */

	//VID_RGBFMT_A1R5G5B5	= 0x3342,   /* 16bpp { A1, R5, G5, B5 }. */
	//VID_RGBFMT_A1B5G5R5	= 0xB342,   /* 16bpp { A1, B5, G5, R5 }. */
	//VID_RGBFMT_A4R4G4B4	= 0x2211,   /* 16bpp { A4, R4, G4, B4 }. */
	//VID_RGBFMT_A4B4G4R4	= 0xA211,   /* 16bpp { A4, B4, G4, R4 }. */
	//VID_RGBFMT_A8R3G3B2	= 0x1120,   /* 16bpp { A8, R3, G3, B2 }. */
	//VID_RGBFMT_A8B3G3R2	= 0x9120,   /* 16bpp { A8, B3, G3, R2 }. */

	//VID_RGBFMT_G8R8_G8B8	= 0x4ED3,   /* 16bpp { G8, R8, G8, B8 }. */
	//VID_RGBFMT_R8G8_B8G8	= 0x4F84,   /* 16bpp { R8, G8, B8, G8 }. */
	//VID_RGBFMT_G8B8_G8R8	= 0xCED3,   /* 16bpp { G8, B8, G8, R8 }. */
	//VID_RGBFMT_B8G8_R8G8	= 0xCF84,   /* 16bpp { B8, G8, R8, G8 }. */
	//VID_RGBFMT_X8L8		= 0x4003,   /* 16bpp { X8, L8 }. */
	//VID_RGBFMT_A8L8		= 0x1003,   /* 16bpp { A8, L8 }. */
	//VID_RGBFMT_L16		= 0x4554,   /* 16bpp { L16 }. */

	//VID_RGBFMT_R8G8B8	= 0x4653,   /* 24bpp { R8, G8, B8 }. */
	//VID_RGBFMT_B8G8R8	= 0xC653,   /* 24bpp { B8, G8, R8 }. */

	//VID_RGBFMT_X8R8G8B8	= 0x4653,   /* 32bpp { X8, R8, G8, B8 }. */
	//VID_RGBFMT_X8B8G8R8	= 0xC653,   /* 32bpp { X8, B8, G8, R8 }. */
	//VID_RGBFMT_A8R8G8B8	= 0x0653,   /* 32bpp { A8, R8, G8, B8 }. */
	//VID_RGBFMT_A8B8G8R8	= 0x8653,   /* 32bpp { A8, B8, G8, R8 }. */
//};

/* bit masking */
#define BIT_SET(v,b)	(v |= (1<<(b)))
#define BIT_CLR(v,b)	(v &= ~(1<<(b)))
#define IS_SET(v,b)	(v & (1<<(b)))
#define IS_CLR(v,b)	!(v & (1<<(b)))
#define BIT_MASK_ONES(b) ((1<<(b))-1)


//#define FBIO_MAGIC			'D' //used in the pollux driver. not sure why
//#define MLC_IOCTBACKGND		_IOW(FBIO_MAGIC, 100, unsigned int)
//#define MLC_IOCTLAYEREN		_IOW(FBIO_MAGIC, 101, unsigned int)
//#define MLC_IOCTADDRESS		_IOW(FBIO_MAGIC, 102, unsigned int)
//#define MLC_IOCTHSTRIDE		_IOW(FBIO_MAGIC, 103, unsigned int)
//#define MLC_IOCTVSTRIDE		_IOW(FBIO_MAGIC, 104, unsigned int)
//#define MLC_IOCT3DENB		_IOW(FBIO_MAGIC, 105, unsigned int)
//#define MLC_IOCTDIRTY		_IOW(FBIO_MAGIC, 106, unsigned int)
//#define MLC_IOCQDIRTY		_IOR(FBIO_MAGIC, 107, unsigned int)
//#define MLC_IOCQHSTRIDE		_IOR(FBIO_MAGIC, 108, unsigned int)

//ioctl.h
struct position_cmd {
	unsigned int top;
   	unsigned int left;
	unsigned int right;
	unsigned int bottom;
};

struct screensize_cmd {
	unsigned int width;
	unsigned int height;
};

struct overlaysize_cmd {
	unsigned int srcwidth;
	unsigned int srcheight;
	unsigned int dstwidth;
	unsigned int dstheight;
};

union mlc_cmd {
	struct position_cmd position;
	struct screensize_cmd screensize;
	struct overlaysize_cmd overlaysize;
};

//normally in arch/arm/lf1000/mach/mlc.h
int mlc_GetAddressCb(u8 layer, int *addr);
int mlc_GetAddressCr(u8 layer, int *addr);


//normally in include/linux/lf1000
#define MLC_IOC_MAGIC   'm'
#define MLC_IOCTENABLE		_IO(MLC_IOC_MAGIC,  0)
#define MLC_IOCTBACKGND		_IO(MLC_IOC_MAGIC,  1)
#define MLC_IOCQBACKGND		_IO(MLC_IOC_MAGIC,  2)
#define MLC_IOCTPRIORITY	_IO(MLC_IOC_MAGIC,  3)
#define MLC_IOCQPRIORITY	_IO(MLC_IOC_MAGIC,  4)
#define MLC_IOCTTOPDIRTY	_IO(MLC_IOC_MAGIC,  5)
#define MLC_IOCSSCREENSIZE	_IOW(MLC_IOC_MAGIC, 6, struct screensize_cmd *)
#define MLC_IOCGSCREENSIZE	_IOR(MLC_IOC_MAGIC, 7, struct screensize_cmd *)



#define MLC_IOCTLAYEREN		_IO(MLC_IOC_MAGIC,  8)
#define MLC_IOCTADDRESS		_IO(MLC_IOC_MAGIC,  9)
#define MLC_IOCQADDRESS		_IO(MLC_IOC_MAGIC,  25)
#define MLC_IOCQFBSIZE		_IO(MLC_IOC_MAGIC,  29)


#define MLC_IOCTHSTRIDE		_IO(MLC_IOC_MAGIC,  10)
#define MLC_IOCQHSTRIDE		_IO(MLC_IOC_MAGIC,  26)
#define MLC_IOCTVSTRIDE		_IO(MLC_IOC_MAGIC,  11)
#define MLC_IOCQVSTRIDE		_IO(MLC_IOC_MAGIC,  27)

	
#define MLC_IOCTLOCKSIZE	_IO(MLC_IOC_MAGIC,  12)
#define MLC_IOCQLOCKSIZE	_IO(MLC_IOC_MAGIC,  47) //PATCH

#define MLC_IOCSPOSITION	_IOW(MLC_IOC_MAGIC, 13, struct position_cmd *)
#define MLC_IOCGPOSITION	_IOR(MLC_IOC_MAGIC, 14, struct position_cmd *)
#define MLC_IOCTFORMAT		_IO(MLC_IOC_MAGIC,  15)
#define MLC_IOCQFORMAT		_IO(MLC_IOC_MAGIC,  16)

#define MLC_IOCTDIRTY		_IO(MLC_IOC_MAGIC,  17)
#define MLC_IOCQDIRTY		_IO(MLC_IOC_MAGIC,  32)
#define MLC_IOCT3DENB		_IO(MLC_IOC_MAGIC,  18)
#define MLC_IOCQ3DENB		_IO(MLC_IOC_MAGIC,  33)

#define MLC_IOCTALPHA		_IO(MLC_IOC_MAGIC,  19)
#define MLC_IOCQALPHA		_IO(MLC_IOC_MAGIC,  28)
#define MLC_IOCTTPCOLOR		_IO(MLC_IOC_MAGIC,  20)
#define MLC_IOCQTPCOLOR		_IO(MLC_IOC_MAGIC,  40) //PATCH
#define MLC_IOCTBLEND		_IO(MLC_IOC_MAGIC,  21)
#define MLC_IOCQBLEND		_IO(MLC_IOC_MAGIC,  41) //PATCH
#define MLC_IOCTTRANSP		_IO(MLC_IOC_MAGIC,  22)
#define MLC_IOCQTRANSP		_IO(MLC_IOC_MAGIC,  42) //PATCH

#define MLC_IOCTINVERT		_IO(MLC_IOC_MAGIC,  23)
#define MLC_IOCQINVERT		_IO(MLC_IOC_MAGIC,  43) //PATCH

#define MLC_IOCTINVCOLOR	_IO(MLC_IOC_MAGIC,  24)
#define MLC_IOCQINVCOLOR	_IO(MLC_IOC_MAGIC,  44)  //PATCH

#define MLC_IOCSOVERLAYSIZE	_IOW(MLC_IOC_MAGIC, 30, struct overlaysize_cmd *)
#define MLC_IOCGOVERLAYSIZE	_IOR(MLC_IOC_MAGIC, 31, struct overlaysize_cmd *)
#define MLC_IOCTINVISIBLE	_IO(MLC_IOC_MAGIC,  34)
#define MLC_IOCQINVISIBLE	_IO(MLC_IOC_MAGIC,  35)

#define MLC_IOCSINVISIBLEAREA	_IOW(MLC_IOC_MAGIC, 36, struct position_cmd *)
#define MLC_IOCGINVISIBLEAREA	_IOR(MLC_IOC_MAGIC, 37, struct position_cmd *)

#define MLC_IOCTADDRESSCB	_IO(MLC_IOC_MAGIC,  38)
#define MLC_IOCQADDRESSCB	_IO(MLC_IOC_MAGIC,  45) //PATCH

#define MLC_IOCTADDRESSCR	_IO(MLC_IOC_MAGIC,  39)
#define MLC_IOCQADDRESSCR	_IO(MLC_IOC_MAGIC,  46) //PATCH

#define FBIO_ENABLE_TVOUT	_IO(MLC_IOC_MAGIC,  48) //PATCH
#define FBIO_DISABLE_TVOUT	_IO(MLC_IOC_MAGIC,  49) //PATCH




/*
 * 
 * DPC Header - registers as offsets from DPC_BASE 
 * 
 * 
 */

/* DPC CONTROL 0 REGISTER (DPCCTRL0) */
#define DPCCTRL0		0x08C
#define DPCENB		15
#define ENCENB		14
#define DACENB		13
#define RGBMODE		12
#define _INTENB		11
#define _INTPEND	10
#define SCANMODE	9
#define SEAVENB		8
#define DELAYRGB	4
#define POLFIELD	2
#define POLVSYNC	1
#define POLHSYNC	0

/* DPC CONTROL 1 REGISTER (DPCCTRL1) */
#define DPCCTRL1		0x08E
#define SWAPRB		15
#define YCRANGE		13
#define FORMAT1		8
#define YCORDER		6
#define BDITHER		4
#define GDITHER		2
#define RDITHER		0



/* DPC CONTROL 2 REGISTER (DPCCTRL2) */
#define DPCCTRL2		0x098

#define CPCYC		12	/* STN LCD CP (Shift Clock) Cycle, in VCLKs */
#define STNLCDBITWIDTH	9	/* STN LCD bus bit width */
#define LCDTYPE		8	/* 0: TFT or Video Encoder, 1: STN LCD */
#define PADCLKSEL	0

/* DPC CLOCK GENERATION CONTROL 0 REGISTER (DPCCLKGEN0) */
#define DPCCLKGEN0		0x1C4
#define OUTCLKENB	15
#define OUTCLKDELAY0	12
#define CLKDIV0		4
#define CLKSRCSEL0	1
#define OUTCLKINV0	0

/* DPC CLOCK GENERATION CONTROL 1 REGISTER (DPCCLKGEN1) */
#define DPCCLKGEN1		0x1C8
#define OUTCLKDELAY1	12
#define CLKDIV1		4
#define CLKSRCSEL1	1
#define OUTCLKINV1	0

/* DPC CLOCK GENERATION ENABLE REGISTER (DPCCLKENB) */
#define DPCCLKENB		0x1C0
#define _PCLKMODE	3
#define _CLKGENENB	2

#define DPCHTOTAL		0x07C
#define DPCHSWIDTH		0x07E
#define DPCHASTART		0x080
#define DPCHAEND		0x082
#define DPCVTOTAL		0x084
#define DPCVSWIDTH		0x086
#define DPCVASTART		0x088
#define DPCVAEND		0x08A
#define DPCEVTOTAL		0x090
#define DPCEVSWIDTH		0x092
#define DPCEVASTART		0x094
#define DPCEVAEND		0x096
#define DPCVSEOFFSET		0x09A
#define DPCVSSOFFSET		0x09C
#define DPCEVSEOFFSET		0x09E
#define DPCEVSSOFFSET		0x0A0
/* DPC SYNC DELAY 0 REGISTER (DPCDELAY0) */
#define DPCDELAY0		0x0A2
#define DELAYDE		8
#define DELAYVS		4
#define DELAYHS		0

/* registers (offsets from DPCBASE) for internal video encoder */

#define VENCCTRLA		0x002 
#define VENCCTRLB		0x004 
#define VENCSCH 		0x008 
#define VENCHUE 		0x00A 
#define VENCSAT 		0x00C 
#define VENCCRT 		0x00E 
#define VENCBRT 		0x010 
#define VENCFSCADJH		0x012 
#define VENCFSCADJL		0x014 
#define VENCDACSEL		0x020 
#define VENCICNTL		0x040 
#define VENCHSVS0		0x048 
#define VENCHSOS		0x04A 
#define VENCHSOE		0x04C 
#define VENCVSOS		0x04E 
#define VENCVSOE		0x050  
#define DPUPSCALECON0	0x0A4
#define DPUPSCALECON1	0x0A6
#define DPUPSCALECON2	0x0A8


/* hardware-related definitions */

/* PCLK modes (Already in mlc.h*/
//enum
//{
	//PCLKMODE_ONLYWHENCPUACCESS,	/* Operate When CPU Acces */
	//PCLKMODE_ALWAYS,		/* Operate Always */
//};

/* clock sources */
enum {
	VID_VCLK_SOURCE_PLL0	= 0,
	VID_VCLK_SOURCE_PLL1	= 1,
	VID_VCLK_SOURCE_XTI	= 5,
	VID_VCLK_SOURCE_VCLK2	= 7,	/* clock generator 0 */
};

/* yc orders */
enum {
	VID_ORDER_CbYCrY	= 0,
	VID_ORDER_CrYCbY	= 1,
	VID_ORDER_YCbYCr	= 2,
	VID_ORDER_YCrYCb	= 3
};

/* pad clocks */
enum {
	VID_PADVCLK_VCLK	= 0,
	VID_PADVCLK_nVCLK	= 1,
	VID_PADVCLK_VCLK2	= 2,
	VID_PADVCLK_nVCLK2	= 3
};

/* RGB dithering mode. */
enum DITHER
{
	DITHER_BYPASS		= 0,  /* bypass mode. */
	DITHER_5BIT		= 2,  /* 8 bit -> 5 bit mode. */
	DITHER_6BIT		= 3,  /* 8 bit -> 6 bit mode. */
};

/* video formats */
enum {
	VID_FORMAT_RGB555	= 0,
	VID_FORMAT_RGB565	= 1,
	VID_FORMAT_RGB666	= 2,
	VID_FORMAT_RGB888	= 3,
	VID_FORMAT_MRGB555A	= 4,
	VID_FORMAT_MRGB555B	= 5,
	VID_FORMAT_MRGB565	= 6,
	VID_FORMAT_MRGB666	= 7,
	VID_FORMAT_MRGB888A	= 8,
	VID_FORMAT_MRGB888B	= 9,
	VID_FORMAT_CCIR656	= 10,
	VID_FORMAT_CCIR601A	= 12,
	VID_FORMAT_CCIR601B	= 13,
};

//from DPC Config
#define DISPLAY_VID_PRI_OUTORDER	VID_ORDER_CbYCrY
#define DISPLAY_VID_PRI_OUTPUT_FORMAT	VID_FORMAT_RGB888
	#define DISPLAY_VID_PRI_VCLK_SOURCE		VID_VCLK_SOURCE_PLL1
	#define DISPLAY_VID_PRI_VCLK2_SOURCE		VID_VCLK_SOURCE_VCLK2
	#define DISPLAY_VID_PRI_SWAP_RGB		0
	#define DPC_DESIRED_CLOCK_HZ			6400000
	#define DISPLAY_VID_PRI_PAD_VCLK		VID_PADVCLK_nVCLK2
	#define DISPLAY_VID_PRI_VCLK_DELAY		0
	#define DISPLAY_VID_PRI_VCLK_INV		1
	#define DISPLAY_VID_PRI_VCLK_OUT_ENB		0
	#define DISPLAY_VID_PRI_VCLK2_DIV		0
	#define DISPLAY_VID_PRI_MAX_X_RESOLUTION	320
	#define DISPLAY_VID_PRI_MAX_Y_RESOLUTION	240
	#define DISPLAY_VID_PRI_HSYNC_SWIDTH		2
	#define DISPLAY_VID_PRI_HSYNC_FRONT_PORCH	50
	#define DISPLAY_VID_PRI_HSYNC_BACK_PORCH	36
	#define DISPLAY_VID_PRI_HSYNC_ACTIVEHIGH	0
	#define DISPLAY_VID_PRI_VSYNC_SWIDTH		1
	#define DISPLAY_VID_PRI_VSYNC_FRONT_PORCH	4
	#define DISPLAY_VID_PRI_VSYNC_BACK_PORCH	17
	#define DISPLAY_VID_PRI_VSYNC_ACTIVEHIGH	0

/*
 * driver private data
 */

struct lf1000fb_info {
	struct fb_info			fb;
	struct device			*dev;

	void				*fbmem;

	int pseudo_pal[16];
	int palette_buf[256];
	int                     pix_fmt;
};
static void *mlcregs;
static void *dpcregs;

static void enable_tvout_dpc(struct fb_info *info);
static void enable_tvout_mlc(struct fb_info *info);
static void disable_tvout(void);

