/* LF1000 I2S AUDIO Driver
 *
 * i2sAudio.c -- Main driver functionality.
 *
 * Scott Esters
 * LeapFrog Enterprises
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <mach/platform.h>

#include "audio_hal.h"
#include "i2sAudio.h"

/* desired I2S clock rate, in MHz */
#define I2S_AUDIO_RATE		32000	/* desired I2S audio rate in HZ */
#define I2S_AUDIO_ERR		(I2S_AUDIO_RATE / 100) /* max allowed error */
#define I2S_AUDIO_RATE_MIN	(I2S_AUDIO_RATE - I2S_AUDIO_ERR) /* min rate */
#define I2S_AUDIO_RATE_MAX	(I2S_AUDIO_RATE + I2S_AUDIO_ERR) /* max rate */
AUDIO_REGS *audioRegs = NULL; // LF1000 Audio registers

/*
 * getAudioRate()
 */
int getAudioRate(void)
{
	u16 reg16;
	int div;
	int rate;

	/* retrive rate from CLKGEN0 */
	reg16 = ioread16(&audioRegs->CLKGEN0);	// get prior value
	div   = reg16 & CLKGEN0_CLKDIV0_MASK;	// capture bits from register
	div   = div >> CLKGEN0_CLKDIV0;		// convert from bits to value
	div++;					// finish conversion to divisor
	rate = get_pll_freq(PLL0)/div/256;
	printk(KERN_INFO "%s: sample rate: %dHz, divisor: %d, CLKGEN0: 0x%8.8X\n",
			__FUNCTION__, rate, div, reg16);
	return(rate);				// return sample rate
}
EXPORT_SYMBOL(getAudioRate);

/*
 * setAudioRate(int sample_rate)
 *   sample_rate: audio bit rate in hertz
 */
void setAudioRate(int sample_rate)
{
	u16 reg16;
	int div;
	int rate;

	/* set up hardware rate, sampling rate is 256 fs in master mode */

	div = lf1000_CalcDivider(get_pll_freq(PLL0), (256 * sample_rate));
	if(div < 0) {
		printk(KERN_ERR "audio: failed to find a clock divider!\n");
		return;
	}

	/* calculate actual audio rate */
	rate = get_pll_freq(PLL0)/div/256;
	
	printk(KERN_INFO "%s: expected rate %dHz, actual: %dHz\n",
			__FUNCTION__, sample_rate, rate);

	/* setting CLKGEN0  -- output clock is 256 * I2S_AUDIO_RATE */
	reg16 = ioread16(&audioRegs->CLKGEN0);	// get prior value
	reg16 &= ~CLKGEN0_CLKDIV0_MASK;		// remove old divisor
	reg16 &= ~CLKGEN0_CLKSRCSEL0_MASK;	// remove old clock source
	reg16 |= CLKGEN0_CLKSRCSEL0_PLL0;	// Use PLL0
	reg16 |= ((div-1)<<CLKGEN0_CLKDIV0);	// divide
	BIT_CLR(reg16, CLKGEN0_OUTCLKINV0);	// don't invert clock
	iowrite16(reg16, &audioRegs->CLKGEN0);	// update register
}
EXPORT_SYMBOL(setAudioRate);

int __init initAudio(struct resource *res)
{
	int ret = 0;
	u16 reg16 = 0;
	u32 reg32 = 0;
	int div;
	int rate;

	/* set up hardware rate, sampling rate is 256 fs in master mode */

	div = lf1000_CalcDivider(get_pll_freq(PLL0), (256 * I2S_AUDIO_RATE));
	if(div < 0) {
		printk(KERN_ERR "audio: failed to find a clock divider!\n");
		return -EFAULT;
	}

	/* show actual audio rate */
	rate = get_pll_freq(PLL0)/div/256;
	
	/* complain if not within range */
	if((rate < I2S_AUDIO_RATE_MIN) || (I2S_AUDIO_RATE_MAX < rate))
		printk(KERN_INFO "audio: expected rate %dHz, actual: %dHz\n",
			I2S_AUDIO_RATE, rate);

	if(!request_mem_region(res->start, (res->end - res->start)+1, 
				"audio")) {
		printk(KERN_ERR "audio: failed to map region\n");
		return -EBUSY;
	}

	audioRegs = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(audioRegs == NULL) {
		printk(KERN_ERR "audio: failed to ioremap.\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	/* initialize audio registers */

	iowrite16(0, &audioRegs->AC97_CTRL);
	iowrite16(0, &audioRegs->AC97_CONFIG);
	iowrite16(0, &audioRegs->I2S_CTRL);
	iowrite16(0, &audioRegs->I2S_CONFIG);
	iowrite16(0, &audioRegs->AUDIO_BUFF_CTRL);
	iowrite16(0, &audioRegs->AUDIO_BUFF_CONFIG);
	iowrite16(0, &audioRegs->AUDIO_IRQ_ENA);
	iowrite16(0xFF, &audioRegs->AUDIO_IRQ_PEND);

	/* setting CLKGEN0  -- output clock is 256 * I2S_AUDIO_RATE */
	setAudioRate(I2S_AUDIO_RATE);
	
	// setting CLKGEN1
	reg16 = ioread16(&audioRegs->CLKGEN1);	// get prior value
	reg16 &= ~CLKGEN1_CLKSRCSEL1_MASK;	// remove prior clock source
	reg16 |= CLKGEN1_CLKSRCSEL1_CLKGEN0;	// use output of CLKGEN0
	reg16 &= ~CLKGEN1_CLKDIV1_MASK;		// remove prior clock divisor
	reg16 |= (3<<CLKGEN1_CLKDIV1);		// divide by (3+1) for BITCLK
	BIT_CLR(reg16, CLKGEN1_OUTCLKINV1);	// don't invert clock
	BIT_CLR(reg16, CLKGEN1_OUTCLKENB1);	// enable clock output
	iowrite16(reg16, &audioRegs->CLKGEN1);	// update register
	
	// setting CLKENB
	reg32 = ioread32(&audioRegs->CLKENB);
	BIT_SET(reg32, CLKENB_PCLKMODE);	// use PCLK
	iowrite32(reg32, &audioRegs->CLKENB);
	BIT_CLR(reg32, CLKENB_CLKGENENB);	// disable clock
	iowrite32(reg32, &audioRegs->CLKENB);
	BIT_SET(reg32, CLKENB_CLKGENENB);	// enable clock
	iowrite32(reg32, &audioRegs->CLKENB);
	
	// cold reset controller
	reg16 = ioread16(&audioRegs->AC97_CTRL);// get reg
	BIT_CLR(reg16, COLD_RST);		// reset AC97
	iowrite16(reg16, &audioRegs->AC97_CTRL);
	BIT_SET(reg16, COLD_RST);		// reset AC97
	iowrite16(reg16, &audioRegs->AC97_CTRL);
	
	// AC97 off
	reg16 = ioread16(&audioRegs->AC97_CTRL);// get reg
	BIT_CLR(reg16, CTRL_RST);
	iowrite16(reg16, &audioRegs->AC97_CTRL);
	BIT_CLR(reg16, ACLINK_RUN);
	iowrite16(reg16, &audioRegs->AC97_CTRL);

	// I2S Setting to FALSE
	reg16 = ioread16(&audioRegs->I2S_CTRL);	// get reg
	BIT_CLR(reg16, I2S_EN);
	BIT_CLR(reg16, I2SLINK_RUN);		// reset I2S controller
	iowrite16(reg16, &audioRegs->I2S_CTRL);
		
	// AC97 off
	reg16 = ioread16(&audioRegs->AC97_CTRL);// get reg
	BIT_CLR(reg16, CTRL_RST);
	iowrite16(reg16, &audioRegs->AC97_CTRL);
	BIT_CLR(reg16, ACLINK_RUN);
	iowrite16(reg16, &audioRegs->AC97_CTRL);
	
	// I2S Setting to TRUE
	reg16 = ioread16(&audioRegs->I2S_CTRL);	// get reg
	BIT_SET(reg16, I2S_EN);			// I2S controller on
	iowrite16(reg16, &audioRegs->I2S_CTRL);
		
	// set output master mode, 256fs, and 16-bit data width
	reg16 = ioread16(&audioRegs->I2S_CONFIG);// get reg
	BIT_CLR(reg16, MST_SLV);		// codec is slave
	iowrite16(reg16, &audioRegs->I2S_CONFIG);
	reg16 &= ~SYNC_PERIOD_MASK;		// remove old Sync Period
	reg16 |= SYNC_PERIOD_64;		// bit clock of 64 for 256 fs
	iowrite16(reg16, &audioRegs->I2S_CONFIG);
	
	reg16 = ioread16(&audioRegs->AUDIO_BUFF_CONFIG);
	reg16 &= ~PO_WIDTH_MASK;
	reg16 |= PO_WIDTH_16;			// set PCM out width to 16 bit
	iowrite16(reg16, &audioRegs->AUDIO_BUFF_CONFIG);
		
	reg16=ioread16(&audioRegs->AUDIO_BUFF_CTRL);
	BIT_SET(reg16, PCMOBUF_EN); // set PCM buffer out, opposite page 11-9
	iowrite16(reg16, &audioRegs->AUDIO_BUFF_CTRL);
	
	reg16=ioread16(&audioRegs->I2S_CONFIG);
	BIT_SET(reg16, I2SO_EN);		// enable I2S link out
	iowrite16(reg16, &audioRegs->I2S_CONFIG);
	
	reg16=ioread16(&audioRegs->AUDIO_IRQ_ENA); // enable interrupt
	BIT_SET(reg16, POUDR_IRQ_EN);
	iowrite16(reg16, &audioRegs->AUDIO_IRQ_ENA);

	reg16=ioread16(&audioRegs->I2S_CTRL);
	BIT_SET(reg16, I2SLINK_RUN);			// place in idle mode
	iowrite16(reg16, &audioRegs->I2S_CTRL);
		
	reg16=ioread16(&audioRegs->AUDIO_IRQ_PEND);	// clear IRQ pending
	BIT_SET(reg16, POUDR_PEND);
	iowrite16(reg16, &audioRegs->AUDIO_IRQ_PEND);

	return 0;

fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);

	return ret;
}

AUDIO_REGS *getAudioRegs(void)
{
	return(audioRegs);
}

void setAudioPcmOutBuffer(u8 enableFlag)
{
	u16 reg16;
	reg16 = ioread16(&audioRegs->AUDIO_BUFF_CTRL);
	BIT_CLR(reg16, PCMOBUF_EN);		// assume flag is cleared
	if (enableFlag)
		BIT_SET(reg16, PCMOBUF_EN);	//enable PCM output buffer
	iowrite16(reg16, &audioRegs->AUDIO_BUFF_CTRL);
} 

void deinitAudio(struct resource *res)
{
	setAudioPcmOutBuffer(0); /* disable PCM audio out */
	if(audioRegs != NULL) 
		iounmap(audioRegs);
	release_mem_region(res->start, (res->end - res->start)+1);
}
