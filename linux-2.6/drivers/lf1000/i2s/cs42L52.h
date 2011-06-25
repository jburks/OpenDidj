#ifndef CS42L52_H
#define CS42L52_H

#define CS42L52_ADDR    	0x94

/* partial register list */
#define CS42L52_CHIP_ID		0x01
#define CS42L52_POWER_CTL_1	0x02
#define CS42L52_SPKCTL		0x04
#define CS42L52_PLAYBACK_CONTROL_2 0x0F
#define CS42L52_HEADPHONE_A	0x22
#define CS42L52_HEADPHONE_B	0x23
#define CS42L52_SPEAKER_A	0x24
#define CS42L52_SPEAKER_B	0x25
#define CS42L52_MIXER		0x26

/*  register values */
#define CS42L52_MSTA_DEFAULT	0x06	// Didj and Emerald max volume
#define CS42L52_MSTB_DEFAULT	0x06

#define CS42L52_MSTA_K2		0x0C	// K2 max volume
#define CS42L52_MSTB_K2		0x0C

u8 cs42L52_settings[][2] = {
	/*REG, VALUE*/
	{0x02, 0x01},// Power Control 1
	{0x03, 0x07},// Power Control 2
		     // Power Control 3 -- See cs42L52_reg4_settings table
	{0x05, 0xA0},// Clocking Control
	{0x06, 0x27},// Interface Control 1, Slave, I2S
	{0x07, 0x00},// Interface Control 2
	{0x08, 0x81},// Input A Select ADCA and PGAA
	{0x09, 0x81},// Input B Select ADCB and PGAB
	{0x0A, 0xA5},// Analog & HPF Control
	{0x0B, 0x00},// ADC HPF Corner Frequency
	{0x0C, 0x00},// Misc ADC Control
	{0x0D, 0x10},// Playback Control 1
	{0x0E, 0x02},// Passthru Analog, adjust volume at zero-crossings
	{0x0F, 0xFA},// Playback Control 2, mute headphone and speaker
	{0x10, 0x00},// MIC A
	{0x11, 0x00},// MIC B
	{0x12, 0x00},// ALC, PGA A
	{0x13, 0x00},// ALC, PGA B
	{0x14, 0x00},// Passthru A Volume
	{0x15, 0x00},// Passthru B Volume
	{0x16, 0x00},// ADC A Volume
	{0x17, 0x00},// ADC B Volume
	{0x18, 0x80},// ADC Mixer Channel A Mute
	{0x19, 0x80},// ADC Mixer Channel B Mute
	{0x1A, 0x00},// PCMA Mixer Volume
	{0x1B, 0x00},// PCMB Mixer Volume
	{0x1C, 0x00},// Beep Frequency
	{0x1D, 0x00},// Beep On Time
	{0x1E, 0x3F},// Beep & Tone Configuration
	{0x1F, 0xFF},// Tone Control
	{0x20, CS42L52_MSTA_DEFAULT},// Master Volume Control MSTA
	{0x21, CS42L52_MSTB_DEFAULT},// Master Volume Control MSTB
	{0x22, 0x81},// Headphone Volume Control HPA
	{0x23, 0x81},// Headphone Volume Control HPB
	{0x24, 0xCB},// Speaker Volume Control SPKA
	{0x25, 0xCB},// Speaker Volume Control SPKB
	{0x26, 0x50},// ADC & PCM Channel Mixer
	{0x27, 0x04},// Limiter Control 1
	{0x28, 0x8A},// Limiter Control 2
	{0x29, 0xC3},// Limiter Attack Rate
	{0x2A, 0x00},// ALC Enable & Attack Rate
	{0x2B, 0x00},// ALC Release Rate
	{0x2C, 0x00},// ALC Threshold
	{0x2D, 0x00},// Noise Gate Control
	{0x2F, 0x00},// Battery Compensation
	{0x32, 0x00},// Temperature monitor Control
	{0x33, 0x00},// Thermal Foldback
	{0x34, 0x5F},// Charge Pump Frequency
	{0x02, 0x9F},// Power Control 1
};

/*
 *  setup headphone jack control value in Cirrus Logic register 4,
 *  Power Control 3 register, SPK/HP_SW mutes speaker or headphone
 *
 *  Index into this table using board revision.
 *
 *  Earlier board versions have id code 0 on GPIO_B[31:27];
 *  set index 0 of the array for these boards at compile time.
 *
 *  Typical register 4 settings:
 *      0xAA -- Speaker and Headphones always on
 *      0x05 -- Pin 6 low:  Speaker on,  Headphones off
 *              Pin 6 high: Speaker off, Headphones on
 *      0x50 -- Pin 6 low:  Speaker off, Headphones on
 *              Pin 6 high: Speaker on,  Headphones off
 *      0xFF -- Speaker and Headphones always off
 */

u8 cs42L52_reg4_settings[] = {
	/* VALUE*/
#if defined (CONFIG_MACH_ME_LF1000)
	0x50,	// index [0], older board, set at compile time
#elif defined (CONFIG_MACH_LF_LF1000) 
	0x05,	// index [0], older form-factor board
#else
	#error Board not defined, unable to select Cirrus Logic speaker control
#endif
		
	0x50,	// index [1], rev 1 not used, use standard speaker control
	0x50,	// index [2], rev 2 Emerald NOTV NOCAP
	0x50,	// index [3], rev 3 EP1 board
};

#define cs42L52_reg4_settings_MAX_VERSION (sizeof(cs42L52_reg4_settings) - 1)

/*
 *  Initial settings for Cirrus Logic ADC & PCM Channel Mixer Register 26
 *
 *  Mix together Left and Right channels when using mono speaker, disable
 *  mixing when listening with headphones.
 *
 *  Index into this table using board revision and GPIO jack level.  On older
 *  development boards jack level is 0 for headphones and 1 for speaker.
 *
 *  Earlier board versions have id code 0 on GPIO_B[31:27];
 *  set index 0 of the array for these boards at compile time.
 *
 *  Typical register 26 settings:
 *      0x00 -- Mixer off
 *      0x50 -- Mix Left and Right channels
 */
u8 cs42L52_reg26_settings[][2] = {
	/* <jack_low>, <jack_high> */
#if defined (CONFIG_MACH_ME_LF1000)
	{0x50, 0x00},	// index [0], older board, set at compile time
#elif defined (CONFIG_MACH_LF_LF1000) 
	{0x00, 0x50},	// index [0], older form-factor board
#else
	#error Board not defined, unable to select Cirrus Logic speaker control
#endif
		
	{0x50, 0x00},	// index [1], rev 1 not used, use standard setup
	{0x50, 0x00},	// index [2], rev 2 Emerald NOTV, NOCAP
	{0x50, 0x00}	// index [3], rev 3 EP1 board
};

#define cs42L52_reg26_settings_MAX_VERSION ((sizeof(cs42L52_reg26_settings)/2) - 1)

/*
 *  audio jack value for speaker
 *  Index into this table using board revision.
 *
 *  Earlier board versions have id code 0 on GPIO_B[31:27];
 *  set index 0 of the array for these boards at compile time.
 *
 */

int detectSpeaker[] = {
        /* VALUE*/
#if defined (CONFIG_MACH_ME_LF1000)
        0x1,   // index [0], older board, set at compile time
#elif defined (CONFIG_MACH_LF_LF1000)
        0x0,   // index [0], older form-factor board
#else
        #error Board not defined, unable to select Cirrus Logic speaker control
#endif

        0x1,   // index [1], rev 1 not used, use standard setup
        0x1,   // index [2], rev 2 Emerald NOTV, NOCAP
        0x1,   // index [3], rev 3 EP1 board
};

#define MAX_detectSpeaker (sizeof(detectSpeaker) - 1)

#endif
