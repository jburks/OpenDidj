/* LF1000 Audio Driver
 *
 * i2s.h -- AUDIO control.
 *
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef I2S_H
#define I2S_H

/* module-related definitions */
#define I2S_MAJOR     241

/* audio control */

enum volume_setting {
	AUDIO_VOLUME_DOWN = 0,
	AUDIO_VOLUME_UP
};

void audio_set_volume(enum volume_setting volume);
uint audio_get_volumeDownCount(void);
uint audio_get_volumeUpCount(void);
#endif


