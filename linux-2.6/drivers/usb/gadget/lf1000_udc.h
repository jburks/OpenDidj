#ifndef _LF1000_UDC_H
#define _LF1000_UDC_H

/* Hardware register, offset, and value definitions */
#define UDC_EPINDEX		(0x0)

#define UDC_EPINT		(0x2)
#define EP3INT			(3)
#define EP2INT			(2)
#define EP1INT			(1)
#define EP0INT			(0)

#define UDC_EPINTEN		(0x4)
#define EP3INTEN		(3)
#define EP2INTEN		(2)
#define EP1INTEN		(1)
#define EP0INTEN		(0)

#define UDC_FUNCADDR		(0x6)
#define UDC_FRAMENUM		(0x8)

#define UDC_EPDIR		(0xA)
#define EP1DS			(0x1)
#define EP2DS			(0x2)
#define EP3DS			(0x3)

#define UDC_TEST		(0xC)
#define VBUS			(15)
#define EUERR			(13)
#define PERR			(12)
#define FDWR			(11)
#define SPDSEL			(6)
#define TMD			(4)
#define TEST_TPS		(3)
#define TKS			(2)
#define TJS			(1)
#define TSNS			(0)

#define UDC_SYSSTAT		(0xE)
#define BAERR			(15)
#define TMERR			(14)
#define BSERR			(13)
#define TCERR			(12)
#define DCERR			(11)
#define EOERR			(10)
#define VBUSOFF			(9)
#define VBUSON			(8)
#define TBM			(7)
#define DP			(6)
#define DM			(5)
#define HSP			(4)
#define SDE			(3)
#define HFRM			(2)
#define HFSUSP			(1)
#define HFRES			(0)

#define UDC_SYSCTL		(0x10)
#define DTZIEN			(14)
#define DIEN			(12)
#define VBUSOFFEN		(11)
#define VBUSONEN		(10)
#define RWDE			(9)
#define EIE			(8)
#define BIS			(7)
#define SPDEN			(6)
#define RRDE			(5)
#define IPS			(4)
#define MFRM			(2)
#define HSUSPE			(1)
#define HRESE			(0)

#define UDC_EP0STAT		(0x12)
#define EP0LWO			(6)
#define SHT			(4)
#define TST			(1)
#define RSR			(0)

#define UDC_EP0CTL		(0x14)
#define EP0TTE			(3)
#define EP0TTS			(2)
#define EP0ESS			(1)
#define EP0TZLS			(0)

#define UDC_EPSTAT		(0x16)
#define FUDR			(15)
#define FOVF			(14)
#define FPID			(11)
#define OSD			(10)
#define DTCZ			(9)
#define SPT			(8)
#define DOM			(7)
#define FFS			(6)
#define FSC			(5)
#define EPLWO			(4)
#define PSIF			(2)
#define TPS			(1)
#define RPS			(0)

#define UDC_EPCTL		(0x18)
#define SRE			(15)
#define INPKTHLD		(12)
#define OUTPKTHLD		(11)
#define TNPMF			(9)
#define IME			(8)
#define DUEN			(7)
#define FLUSH			(6)
#define TTE			(5)
#define TTS			(3)
#define CDP			(2)
#define ESS			(1)

/* IEMS=interrupt endpoint mode set.  Does this mean that interrupt endpoints
 * were not supported until now?
 */
#define IEMS			(0)

#define UDC_BRCR		(0x1A)
#define UDC_BWCR		(0x1C)
#define UDC_MPR			(0x1E)

#define UDC_DCR			(0x20)
#define ARDRD			(5)
#define FMDE			(4)
#define DMDE			(3)
#define TDR			(2)
#define RDR			(1)
#define DEN			(0)

#define UDC_DTCR		(0x22)
#define UDC_DFCR		(0x24)
#define UDC_DTTCR		(0x26)

/* This buffer has one 16-bit entry per EP? Do we DMA to here? */
#define UDC_EPBUFS		(0x30)

/* New registers in the USB */
#define UDC_PLICR		(0x50)
#define PLC			(8)
#define LPC			(4)

#define UDC_PCR			(0x52)
#define URSTC			(7)
#define SIDC			(6)
#define OPMC			(4)
#define TMSC			(3)
#define XCRC			(4)
#define SUSPC			(1)
#define PCE			(0)

/* The LF1000 CPU has some yet-undocumented registers */
#define UDC_CIKSEL		(0x840)

#define UDC_VBUSINTENB		(0x842)

#define UDC_VBUSPEND		(0x844)
#define VBUSPEND		(0)

#define UDC_POR			(0x846)

#define UDC_SUSPEND		(0x848)

#define UDC_USER0		(0x84A)
#define XOBLOCK_ON		(0)

#define UDC_USER1		(0x84C)
#define VBUSENB			(15)

#define UDC_CLKEN		(0x8C0)
#define UDC_PCLKMODE		(3)
#define UDC_CLKGENENB		(2)
#define UDC_CLKENB		(0)

#define UDC_CLKGEN		(0x8C4)
#define UDC_CLKSRCSEL		(1)
#define UDC_CLKDIV		(4)

/* The LF1000 uses one gpio pin to detect the vbus signal, and another to gate
 * the vbus signal to the UDC's USBVBUS pin.  In response to this latter signal,
 * the UDC pulls the D- line high to perform speed negotiation with the host.
 * In principle, it can also be used to detect the vbus status, but that's just
 * not how we do it.
 */
#include <mach/gpio.h>
#define VBUS_DET_PIN GPIO_PIN5
/* On the LF1000 board made by MagicEyes, the VBUS pin goes directly to the
 * USBVBUS pin on the chip, so the GPIO pins are not used for VBUS handling.
 */

struct lf1000_ep {
	struct list_head		queue;
	unsigned long			last_io;	/* jiffies timestamp */
	struct usb_gadget		*gadget;
	struct lf1000_udc		*dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep			ep;
	u8				num;

	unsigned short			fifo_size;
	u8				bEndpointAddress;
	u8				bmAttributes;

	unsigned			halted : 1;
	unsigned			already_seen : 1;
	unsigned			setup_stage : 1;
	unsigned short			status;
	unsigned int			is_in;
};


/* Warning : ep0 has a fifo of 8 bytes */
#define EP0_FIFO_SIZE		64
#define EP_FIFO_SIZE		512 /* 1024 is the max */
#define DEFAULT_POWER_STATE	0x00

static const char ep0name [] = "ep0";

static const char *const ep_name[] = {
	ep0name, /* everyone has ep0 */
	/* lf1000 three bidirectional endpoints */
	"ep1", "ep2",
};

#define LF1000_ENDPOINTS       ARRAY_SIZE(ep_name)

struct lf1000_request {
	struct list_head		queue;		/* ep's requests */
	struct usb_request		req;
};

enum ep0_state {
        EP0_IDLE,
        EP0_IN_DATA_PHASE,
        EP0_OUT_DATA_PHASE,
        EP0_STATUS_PHASE,
        EP0_STALL,
};

static const char *ep0states[]= {
        "EP0_IDLE",
        "EP0_IN_DATA_PHASE",
        "EP0_OUT_DATA_PHASE",
        "EP0_STATUS_PHASE",
        "EP0_STALL",
};

struct lf1000_udc {
	void __iomem            	*base_addr;
	struct resource			*res;
	struct platform_device		*pdev;

	struct lf1000_ep		ep[LF1000_ENDPOINTS];
	int				address;
	int				irq;
	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;
	u8				fifo_buf[EP_FIFO_SIZE];
	u16				devstatus;
	u16				ifstatus;

	u32				port_status;
    	int 	    	    	    	ep0state;

	bool				vbus;

	// count VBUS ints, whether high, low or high and low
	u32				vbus_int_high;		// high only
	u32				vbus_int_high_low;	// high and low
	u32				vbus_int_low;		// low only
	u32				vbus_report_high;	// reported high
	u32				vbus_report_low;	// reported low

	struct lf1000_request		statreq;

	struct input_dev		*input;

	struct dentry			*debug;
};

/****************** MACROS ******************/
/* #define BIT_MASK	BIT_MASK*/
#define BIT_MASK_FF	0xFF

#define maskb(base,v,m,a)      \
	        writeb((readb(base+a) & ~(m))|((v)&(m)), (base+a))

#define maskw(base,v,m,a)      \
	        writew((readw(base+a) & ~(m))|((v)&(m)), (base+a))

#define maskl(base,v,m,a)      \
	        writel((readl(base+a) & ~(m))|((v)&(m)), (base+a))

#define clear_ep0_se(base) do {				\
    	LF1000_UDC_SETIX(base,EP0); 			\
	maskl(base,LF1000_UDC_EP0_CSR_SSE,		\
	    	BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG); 	\
} while(0)

#define clear_ep0_opr(base) do {			\
   	LF1000_UDC_SETIX(base,EP0);			\
	maskl(base,LF1000_UDC_EP0_CSR_SOPKTRDY,	\
		BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG); 	\
} while(0)

#define set_ep0_ipr(base) do {				\
   	LF1000_UDC_SETIX(base,EP0);			\
	maskl(base,LF1000_UDC_EP0_CSR_IPKRDY,		\
		BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG); 	\
} while(0)

#define set_ep0_de_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP0);			\
	maskl(base,(LF1000_UDC_EP0_CSR_SOPKTRDY 	\
		| LF1000_UDC_EP0_CSR_DE),		\
		BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG);	\
} while(0)

#define set_ep0_sse_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP0);			\
	maskl(base,(LF1000_UDC_EP0_CSR_SOPKTRDY 	\
		| LF1000_UDC_EP0_CSR_SSE),		\
		BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG);	\
} while(0)

#define set_ep0_de_in(base) do {			\
   	LF1000_UDC_SETIX(base,EP0);			\
	maskl(base,(LF1000_UDC_EP0_CSR_IPKRDY		\
		| LF1000_UDC_EP0_CSR_DE),		\
		BIT_MASK_FF, LF1000_UDC_EP0_CSR_REG);		\
} while(0)



#define clear_stall_ep1_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP1);			\
	orl(0,base+LF1000_UDC_OUT_CSR1_REG);		\
} while(0)


#define clear_stall_ep2_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP2);			\
	orl(0, base+LF1000_UDC_OUT_CSR1_REG);		\
} while(0)


#define clear_stall_ep3_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP3);			\
	orl(0,base+LF1000_UDC_OUT_CSR1_REG);		\
} while(0)


#define clear_stall_ep4_out(base) do {			\
   	LF1000_UDC_SETIX(base,EP4);			\
	orl(0, base+LF1000_UDC_OUT_CSR1_REG);		\
} while(0)

#endif
