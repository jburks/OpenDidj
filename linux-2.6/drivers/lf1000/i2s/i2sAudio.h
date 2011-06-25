#ifndef I2SAUDIO_H_
#define I2SAUDIO_H_

#include <linux/platform_device.h>
#include <mach/common.h>

int __init initAudio(struct resource *);
AUDIO_REGS *getAudioRegs(void);
void setAudioPcmOutBuffer(u8 enableFlag);
void deinitAudio(struct resource *res);
int getAudioRate(void);
void setAudioRate(int sample_rate);

#endif /*I2SAUDIO_H_*/
