/* LF1000 Analog to Digital Converter (ADC) Driver
 *
 * mach-lf1000/include/mach/adc.h -- ADC settings and hardware abstrcation.
 *
 * Andrey Yurovsky <andrey@cozybit.com> */

#ifndef LF1000_ADC_H
#define LF1000_ADC_H

/*
 * Multiplexor Settings
 */
	#define LF1000_ADC_VBATSENSE	2
	#define LF1000_ADC_LEDVSENSE	0
	#define LF1000_ADC_VOLUMESENSE	1
	#define LF1000_ADC_UNUSED	3
	#define LF1000_ADC_MAX_CHANNEL	7

/*
 * ADC Registers (offsets from LF1000_ADC_BASE) 
 */
#define ADCCON			0x00
#define ADCDAT			0x04
#define ADCINTENB		0x08
#define ADCINTCLR		0x0C
#define ADCCLKENB		0x40

/* ADC control register (ADCCON) */
#define APEN			14
#define APSV			6
#define ASEL			3
#define STBY			2
#define ADEN			0

/*
 * model ADC as a line:
 * milliVolts = ((ADC_SLOPE * 256) * READING) / 256 + ADC_CONSTANT
 */

#define ADC_SLOPE_256_ME_LF1000 2322    /* ADC slope * 256              */
#define ADC_CONSTANT_ME_LF1000  1267    /* ADC constant                 */
#define ADC_SLOPE_256_LF_LF1000 2012    /* ADC slope * 256              */
#define ADC_CONSTANT_LF_LF1000     0    /* ADC constant                 */

/* ADC-to-millivolts, as per above */
#if defined (CONFIG_MACH_ME_LF1000)
#define ADC_TO_MV(r)    (((ADC_SLOPE_256_ME_LF1000*r)/256)+ADC_CONSTANT_ME_LF1000)
#elif defined (CONFIG_MACH_LF_LF1000)
#define ADC_TO_MV(r)    (((ADC_SLOPE_256_LF_LF1000*r)/256)+ADC_CONSTANT_LF_LF1000)
#else /* undefined */
#error NO Platform Selected
#endif

/*
 * Driver API
 */
int adc_GetReading(u8 channel);

#endif
