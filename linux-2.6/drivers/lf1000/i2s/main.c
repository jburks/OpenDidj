/* LF1000 I2S (AUDIO) Driver
 *
 * main.c -- Main driver functionality.
 *
 * Scott Esters
 * LeapFrog Enterprises
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Brian Cavagnolo <brian@cozybit.com>
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/device.h>

#include <linux/delay.h>

#include <linux/soundcard.h>	/* OSS API */
#include <linux/sound.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <linux/i2c.h>

#include <linux/init.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/adc.h>
#include <mach/common.h>
#include <mach/dma.h>
#include <mach/gpio.h>
#include <mach/i2s.h>
#include <mach/platform.h>

#include "audio_hal.h"
#include "i2sAudio.h"
#include "volumeTable.h"
#include "cs42L52.h"

#define AUDIO_FRAG_SIZE		2048
#define AUDIO_NUM_FRAGS		8	/* Total in system */
#define AUDIO_INIT_NUM_FRAGS	8	/* Initially avail == not "fooled" */
#define VOLUME_SAMPLING_J	HZ / 10	// sample wheel every 100 ms

/* Audio slider / wheel replaced with buttons */
#define DIDJ_BUTTON_MIN_VOLUME		   0
#define DIDJ_BUTTON_MAX_VOLUME		1023
#define DIDJ_BUTTON_VOLUME_STEP		 128			 // 8 levels
#define DIDJ_BUTTON_START_VOLUME DIDJ_BUTTON_VOLUME_STEP * 5 - 1 //initial vol

#define VOLUME_CAPTURE_RANGE	3	// Min Volume wheel change +/-

#define MUTE_HIGH		(0x0 + 2 * VOLUME_CAPTURE_RANGE) // MUTE if less

#define HEADPHONE_SAMPLING_J	HZ / 10	// sample headphone jack every 100 ms

struct frag {
	struct list_head list;
	char *buf;
	struct lf1000_dma_desc desc;
};

static void lf1000_set_volume(struct work_struct *work);
static void lf1000_set_headphones(struct work_struct *work);

static struct audio_device {
	struct platform_device *pdev;
	int minor;
	struct i2c_adapter* i2c;
	struct i2c_msg i2c_messages[2];

	int dmachan;

	/* All fragments are either on the empty list, the empty list, or the
	 * scheduled list.  Grab locks in the following order: empty_lock,
	 * filled_lock, scheduled_lock.
	 */
	struct list_head empty_frags;
	spinlock_t empty_lock;

	struct list_head filled_frags, fooled_frags;
	spinlock_t filled_lock;

	struct list_head scheduled_frags;
	spinlock_t scheduled_lock;

	wait_queue_head_t wqueue;

	struct timer_list volume_timer;
	struct timer_list headphone_timer;
	int ADC_reading;    /* actual ADC reading, simulated if volume buttons*/
	int last_reading;
	int headphone_mode; /* whether we're on headphone or speaker */
	u8 headphone_volume;
	u8 speaker_volume;
	int isMute;			/* audio muted?  not muted=0, muted=1 */
	struct workqueue_struct *codec_tasks;
	struct work_struct volume_work; /* monitor volume knob */
	struct work_struct headphone_work; /* monitor headphone_mode */

	unsigned int hardware_addr;
	int chip_id;

	int reg26_mixer_jack_low;	/* mixer value when jack detect gpio pin = 0 */
	int reg26_mixer_jack_high;	/* mixer value when jack detect gpio pin = 1 */

	int irq;

#ifdef CONFIG_LF1000_AUDIO_STATS
	unsigned int underrun;
	unsigned int underrun_busy;
	unsigned int filled_frag[AUDIO_NUM_FRAGS+1];
	unsigned int scheduled_frag[AUDIO_NUM_FRAGS+1];
#endif
	int isOpen;			/* monitor open/close state		*/
	uint volumeUpCount;		/* number of times volume raised	*/
	uint volumeDownCount;		/* number of times volume lowered	*/

} audio_dev;

static struct audio_device *dev = &audio_dev;

/* Number of audio frags to move onto fooled ring, removing them from play */
/* 0 = use them all.  AUDIO_NUM_FRAGS-2 is the most we can reserve */
static int n_fooled_frags = AUDIO_NUM_FRAGS - AUDIO_INIT_NUM_FRAGS;

#define MIN(x, y) (((x) < (y))?(x):(y))

static void lw_erase (void);

/*********************
 * I2C communication *
 *********************/

static int cs42L52_write_byte(u8 reg, u8 value)
{
	u8 buf[2];
	int ret;

	buf[0] = reg;
	buf[1] = value;

	/* write portion */
	dev->i2c_messages[0].addr = CS42L52_ADDR;
	dev->i2c_messages[0].buf = &buf[0];
	dev->i2c_messages[0].len = 2;
	dev->i2c_messages[0].flags = 0; /* write */

	ret = i2c_transfer(dev->i2c, dev->i2c_messages, 1);
	if (ret < 0) printk(KERN_ERR "i2c_transfer() failed, errno=%d\n", ret);
	return ret;
}

static int cs42L52_read_byte(u8 reg)
{
	u8 buf[2];
	int res;

	buf[0] = reg;
	buf[1] = 0;

	/* write portion */
	dev->i2c_messages[0].addr = CS42L52_ADDR;
	dev->i2c_messages[0].buf = buf;
	dev->i2c_messages[0].len = 1;
	dev->i2c_messages[0].flags = 0; /* write */

	/* read portion */
	dev->i2c_messages[1].addr = CS42L52_ADDR;
	dev->i2c_messages[1].buf = buf;
	dev->i2c_messages[1].len = 2;
	dev->i2c_messages[1].flags = I2C_M_RD;

	res = i2c_transfer(dev->i2c, dev->i2c_messages, 2);
	return res < 0 ? res : buf[1];
}

static int cs42L52_write_array(u8 list[][2], int entries)
{
	int i;
	for(i = 0; i < entries; i++) 
		cs42L52_write_byte(list[i][0], list[i][1]);
	return 0;
}

static int cs42L52_set_mixer(int jack)
{
	if(!jack) /* jack low */
		return(cs42L52_write_byte(CS42L52_MIXER, dev->reg26_mixer_jack_low));
	else 	  /* jack high */
		return(cs42L52_write_byte(CS42L52_MIXER, dev->reg26_mixer_jack_high));
}

static int cs42L52_init(void)
{
	int ret;
	int board_version;

	ret = cs42L52_write_array(cs42L52_settings,
		sizeof(cs42L52_settings)/2);
	if (ret) return(ret);

	// set audio jack speaker / headphone selection
	// try to support later board versions using highest value
	board_version = gpio_get_board_config();
	if (board_version > cs42L52_reg4_settings_MAX_VERSION)
		board_version = cs42L52_reg4_settings_MAX_VERSION;
	
	ret = cs42L52_write_byte(CS42L52_SPKCTL, cs42L52_reg4_settings[board_version]);

	if (ret < 0)
		return(ret);

	// set mixer selection
	// try to support later board versions using highest value
	board_version = gpio_get_board_config();
	if (board_version > cs42L52_reg26_settings_MAX_VERSION)
		board_version = cs42L52_reg26_settings_MAX_VERSION;
	dev->reg26_mixer_jack_low  = cs42L52_reg26_settings[board_version][0]; 
	dev->reg26_mixer_jack_high = cs42L52_reg26_settings[board_version][1]; 

	ret = cs42L52_set_mixer(dev->headphone_mode);

	// set K2 max volume
	if (gpio_have_gpio_k2()) {
		cs42L52_write_byte(0x20, CS42L52_MSTA_K2);
		cs42L52_write_byte(0x21, CS42L52_MSTB_K2);
	}

	return(ret);
}

/*
 * audio_set_volume() -- simulating ADC volume control with buttons
 */

void audio_set_volume(enum volume_setting volume)
{
	int reading = dev->ADC_reading;	// try to construct new reading
	if (volume == AUDIO_VOLUME_DOWN) reading -= DIDJ_BUTTON_VOLUME_STEP;
	if (volume == AUDIO_VOLUME_UP)   reading += DIDJ_BUTTON_VOLUME_STEP;

	/* ensure reading within valid range */
	if (reading < 0)		  reading= 0;
	if (reading > DIDJ_BUTTON_MAX_VOLUME) reading = DIDJ_BUTTON_MAX_VOLUME;

	dev->ADC_reading = reading;	// update reading
}
EXPORT_SYMBOL(audio_set_volume);

uint audio_get_volumeDownCount(void)
{
	return(dev->volumeDownCount);
}
EXPORT_SYMBOL(audio_get_volumeDownCount);

uint audio_get_volumeUpCount(void)
{
	return(dev->volumeUpCount);
}
EXPORT_SYMBOL(audio_get_volumeUpCount);

/**********************
 * Interrupt Handling *
 **********************/

static irqreturn_t audio_irq(int irq, void *dev_id)
{
	AUDIO_REGS *audioRegs = getAudioRegs();
	u16 tmp = ioread16(&audioRegs->AUDIO_IRQ_PEND);

	if(IS_SET(tmp, POUDR_PEND)) {	/* underrun */
		iowrite16(1<<POUDR_PEND, &audioRegs->AUDIO_IRQ_PEND);
		/*printk (KERN_WARNING "!!!!!! Audio Underrun !!!!!!\n");*/
#ifdef CONFIG_LF1000_AUDIO_STATS
		dev->underrun++;	
		if(lf1000_dma_busy(dev->dmachan))
			dev->underrun_busy++;
#endif
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/*******************
 * sysfs Interface *
 *******************/

static ssize_t show_buffer_count(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	int num = AUDIO_NUM_FRAGS - n_fooled_frags;
	return (sprintf (buf, "using %d out of max %d buffers\n", 
			 num, AUDIO_NUM_FRAGS));
}

static ssize_t set_buffer_count(struct device *pdev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	int num, reserved;
	unsigned long flags;
	struct list_head *ptr, *next;
	struct frag *f;
	if (sscanf(buf, "%d", &num) != 1)
		return -EINVAL;
	if (num < 2 || num > AUDIO_NUM_FRAGS)
		return -EINVAL;
	reserved = AUDIO_NUM_FRAGS - num;

	/* Put all the current fooled frags back on to filled list */
	spin_lock_irqsave(&dev->filled_lock, flags);
	n_fooled_frags = reserved;
	list_for_each_safe(ptr, next, &dev->fooled_frags) {
		f = list_entry(ptr, struct frag, list);
		memset((void *)f->buf, 0, AUDIO_FRAG_SIZE); /* Silence it */
		list_del(ptr);
		list_add_tail(&f->list, &dev->filled_frags);
	}
	/* Remove n filled frags and put on fooled list */
	list_for_each_safe(ptr, next, &dev->filled_frags) {
		if (reserved > 0)
		{
			reserved--;
			f = list_entry(ptr, struct frag, list);
			list_del(ptr);
			list_add_tail(&f->list, &dev->fooled_frags);
		}
	}
	spin_unlock_irqrestore(&dev->filled_lock, flags);
	/* If still need more, take from empty list */
	spin_lock_irqsave(&dev->empty_lock, flags);
	list_for_each_safe(ptr, next, &dev->empty_frags) {
		if (reserved > 0)
		{
			reserved--;
			f = list_entry(ptr, struct frag, list);
			list_del(ptr);
			list_add_tail(&f->list, &dev->fooled_frags);
		}
	}
	spin_unlock_irqrestore(&dev->empty_lock, flags);

	return(count);
}

static DEVICE_ATTR(buffer_count, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
		show_buffer_count, set_buffer_count);

#ifdef CONFIG_LF1000_AUDIO_STATS
static ssize_t show_stats(struct device *pdev, struct device_attribute *attr,
			  char *buf)
{
	int i;
	ssize_t len = 0;

	len += sprintf(buf, "Total underruns: %d\n", dev->underrun);
	len += sprintf(buf+len, "Underruns while DMA busy: %d\n", 
			dev->underrun_busy);

	/* for audio delay */
	for (i=0; i<AUDIO_NUM_FRAGS+1; i++) {
		len += sprintf(buf+len, "Frags %d: Filled: %5d Scheduled: %5d\n", 
			       i, dev->filled_frag[i], dev->scheduled_frag[i]);
	}
	return len;
}

static ssize_t set_stats(struct device *pdev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	/* No matter what you write, erase stats */
	lw_erase ();
	return(count);
}


static DEVICE_ATTR(stats, S_IRUGO | S_IWUGO, show_stats, set_stats);
#endif

#ifdef CONFIG_LF1000_AUDIO_DEBUG
static ssize_t show_registers(struct device *pdev, 
				struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	AUDIO_REGS *audioRegs = getAudioRegs();

	if(audioRegs == NULL) {
		printk(KERN_ERR "Error: Audio Register is NULL\n");
		return 0;
	}

	len += sprintf(buf+len,"\nAudio Registers\n");
	len += sprintf(buf+len,"AC97_CTRL         = 0x%04X\n",
		ioread16(&(audioRegs->AC97_CTRL)));
	len += sprintf(buf+len,"AC97_CONFIG       = 0x%04X\n",
		ioread16(&(audioRegs->AC97_CONFIG)));
	len += sprintf(buf+len,"I2S_CTRL          = 0x%04X\n",
		ioread16(&(audioRegs->I2S_CTRL)));
	len += sprintf(buf+len,"I2S_CONFIG        = 0x%04X\n",
		ioread16(&(audioRegs->I2S_CONFIG)));
	len += sprintf(buf+len,"AUDIO_BUFF_CTRL   = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_BUFF_CTRL)));
	len += sprintf(buf+len,"AUDIO_BUFF_CONFIG = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_BUFF_CONFIG)));
	len += sprintf(buf+len,"AUDIO_IRQ_ENA     = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_IRQ_ENA)));
	len += sprintf(buf+len,"AUDIO_IRQ_PEND    = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_IRQ_PEND)));
	len += sprintf(buf+len,"AC97_CODEC_ADDR   = 0x%04X\n",
		ioread16(&(audioRegs->AC97_CODEC_ADDR)));
	len += sprintf(buf+len,"AC97_CODEC_WDATA  = 0x%04X\n",
		ioread16(&(audioRegs->AC97_CODEC_WDATA)));
	len += sprintf(buf+len,"AC97_CODEC_RDATA  = 0x%04X\n",
		ioread16(&(audioRegs->AC97_CODEC_RDATA)));
	len += sprintf(buf+len,"AUDIO_STATUS0     = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_STATUS0)));
	len += sprintf(buf+len,"AUDIO_STATUS1     = 0x%04X\n",
		ioread16(&(audioRegs->AUDIO_STATUS1)));
	len += sprintf(buf+len,"CLKENB            = 0x%08X\n",
		ioread32(&(audioRegs->CLKENB)));
	len += sprintf(buf+len,"CLKGEN0           = 0x%04X\n",
		ioread16(&(audioRegs->CLKGEN0)));
	len += sprintf(buf+len,"CLKGEN1           = 0x%04X\n",
		ioread16(&(audioRegs->CLKGEN1)));

	return len ;
}
static DEVICE_ATTR(registers, S_IRUSR|S_IRGRP|S_IROTH, show_registers, NULL);
#endif

static ssize_t show_volume(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%d\n", dev->ADC_reading));
}

static ssize_t set_volume(struct device *pdev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	int newVolume;
	if (sscanf(buf, "%d", &newVolume) != 1)
		return -EINVAL;
	if (newVolume < 0 || newVolume > 1023)
		return -EINVAL;
	dev->ADC_reading = newVolume;
	return(count);
}

static DEVICE_ATTR(volume, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
		show_volume, set_volume);

static ssize_t show_headphone(struct device *pdev,
			struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%d\n", dev->headphone_volume));
}
static DEVICE_ATTR(headphone, S_IRUSR|S_IRGRP|S_IROTH, show_headphone, NULL);

static ssize_t show_speaker(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%d\n", dev->speaker_volume));
}
static DEVICE_ATTR(speaker, S_IRUSR|S_IRGRP|S_IROTH, show_speaker, NULL);

static ssize_t show_mute(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%s\n", dev->isMute ? "on" : "off"));
}
static DEVICE_ATTR(mute, S_IRUSR|S_IRGRP|S_IROTH, show_mute, NULL);

static ssize_t show_open(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%d\n", dev->isOpen));
}
static DEVICE_ATTR(open, S_IRUSR|S_IRGRP|S_IROTH, show_open, NULL);

static ssize_t show_volumeUpCount(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%u\n", dev->volumeUpCount));
}
static DEVICE_ATTR(volumeUpCount, S_IRUSR|S_IRGRP|S_IROTH, show_volumeUpCount, NULL);

static ssize_t show_volumeDownCount(struct device *pdev, struct device_attribute *attr,
			char *buf)
{
	return(sprintf(buf, "%u\n", dev->volumeDownCount));
}
static DEVICE_ATTR(volumeDownCount, S_IRUSR|S_IRGRP|S_IROTH, show_volumeDownCount, NULL);

static ssize_t show_output(struct device *pdev, 
		struct device_attribute *attr, char *buf)
{
	int isSpeaker;
	int board_version = gpio_get_board_config();
	if (board_version > MAX_detectSpeaker)
		board_version = MAX_detectSpeaker;
	isSpeaker = detectSpeaker[board_version];
	return sprintf(buf, "%s\n", 
		(dev->headphone_mode == isSpeaker) ?  "headphones" : "speaker");
}
static DEVICE_ATTR(output, S_IRUSR|S_IRGRP|S_IROTH, show_output, NULL);
			

static ssize_t show_codecid(struct device *pdev, struct device_attribute *attr,
			    char *buf)
{
	int id;
	cs42L52_read_byte(CS42L52_CHIP_ID);
	id = cs42L52_read_byte(CS42L52_CHIP_ID);

	if(id < 0) 
		return sprintf(buf, "%s", "error: chip ID not read.");
	return sprintf(buf, "0x%X\n", id);
}
static DEVICE_ATTR(codec_id, S_IRUSR|S_IRGRP|S_IROTH, show_codecid, NULL);

static ssize_t show_spkctl(struct device *pdev, struct device_attribute *attr,
		           char *buf)
{
	int spkctl;
	spkctl = cs42L52_read_byte(CS42L52_SPKCTL);
	if (spkctl < 0)
		return sprintf(buf, "%s", "error: chip SPKCTL reg not read.");
	return(sprintf(buf, "0x%2.2X\n", spkctl));
}

static ssize_t set_spkctl(struct device *pdev, struct device_attribute *attr,
		          const char *buf, size_t count)
{
	int spkctl;
	if(sscanf(buf, "%i", &spkctl) != 1)
		return -EINVAL;
	cs42L52_write_byte(CS42L52_SPKCTL, spkctl);
	return(count);
}

static DEVICE_ATTR(spk_ctl, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
		   show_spkctl, set_spkctl);

static ssize_t show_mixer_jack_low(struct device *pdev, struct device_attribute *attr,
		           char *buf)
{
	return(sprintf(buf, "0x%2.2X\n", dev->reg26_mixer_jack_low));
}

static ssize_t set_mixer_jack_low(struct device *pdev, struct device_attribute *attr,
		          const char *buf, size_t count)
{
	int value;
	if(sscanf(buf, "%i", &value) != 1)
		return -EINVAL;
	dev->reg26_mixer_jack_low = value;
	cs42L52_set_mixer(dev->headphone_mode);
	return(count);
}

static DEVICE_ATTR(jack_low, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
		   show_mixer_jack_low, set_mixer_jack_low);

static ssize_t show_mixer_jack_high(struct device *pdev, struct device_attribute *attr,
		           char *buf)
{
	return(sprintf(buf, "0x%2.2X\n", dev->reg26_mixer_jack_high));
}

static ssize_t set_mixer_jack_high(struct device *pdev, struct device_attribute *attr,
		          const char *buf, size_t count)
{
	int value;
	if(sscanf(buf, "%i", &value) != 1)
		return -EINVAL;
	dev->reg26_mixer_jack_high = value;
	cs42L52_set_mixer(dev->headphone_mode);
	return(count);
}

static DEVICE_ATTR(jack_high, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
		   show_mixer_jack_high, set_mixer_jack_high);

static struct attribute *audio_attributes[] = {
	&dev_attr_buffer_count.attr,
#ifdef CONFIG_LF1000_AUDIO_STATS
	&dev_attr_stats.attr,
#endif
#ifdef CONFIG_LF1000_AUDIO_DEBUG
	&dev_attr_registers.attr,
#endif
	&dev_attr_codec_id.attr,
	&dev_attr_volume.attr,
	&dev_attr_speaker.attr,
	&dev_attr_headphone.attr,
	&dev_attr_mute.attr,
	&dev_attr_open.attr,
	&dev_attr_volumeUpCount.attr,
	&dev_attr_volumeDownCount.attr,
	&dev_attr_spk_ctl.attr,
	&dev_attr_jack_low.attr,
	&dev_attr_jack_high.attr,
	&dev_attr_output.attr,
	NULL
};

static struct attribute_group audio_attr_group = {
	.attrs = audio_attributes
};


/********************************************************
 * headphone volume control				*
 *     input value is 10 bit ADC, from 0 to 0x3FF	*
 *     output value is 8 bits between VOLUME_MIN to	*
 *     VOLUME_MAX.  Translation table has 128 entries.  *
 *							*
 *******************************************************/

static u8 headphone_volume_convert(u16 reading)
{
	u32 tmp;

	/* keep most significant 7 bits, 0x80 range */
	tmp = (reading >> 3) & 0x7F;

	/* adjust for offset */
	return headphoneVolume[tmp];
}

 /*******************************************************
 * speaker volume control				*
 *     input value is 10 bit ADC, from 0 to 0x3FF	*
 *     output value is 8 bits between VOLUME_MIN to	*
 *     VOLUME_MAX.  Translation table has 128 entries.  *
 *							*
 *******************************************************/

static u8 speaker_volume_convert(u16 reading)
{
	u32 tmp;

	/* keep most significant 7 bits, 0x80 range */
	tmp = (reading >> 3) & 0x7F;

	/* adjust for offset */
	return speakerVolume[tmp];
}


static void mute_headphone(void)
{
	u8 reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (!(reg & 0xC0)) {		// if headphones unmuted, then mute
		reg = reg | 0xC0;	// enable mute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

static void unmute_headphone(void)
{
	u8 reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (reg & 0xC0) {		// if headphones muted, then unmute
		reg = reg & 0x3F;	// unmute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

static void set_headphone_volume(u8 vol)
{
	cs42L52_write_byte(CS42L52_HEADPHONE_A, vol);
	cs42L52_write_byte(CS42L52_HEADPHONE_B, vol);
}

static void mute_speaker(void)
{
	u8 reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (!(reg & 0x30)) {		// if speakers unmuted, then mute
		reg = reg | 0x30;	// enable mute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

static void unmute_speaker(void)
{
	u8 reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (reg & 0x30) {		// if speakers muted, then unmute
		reg = reg & 0xCF;	// unmute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

static void set_speaker_volume(u8 vol)
{
	cs42L52_write_byte(CS42L52_SPEAKER_A, vol);
	cs42L52_write_byte(CS42L52_SPEAKER_B, vol);
}


static void lf1000_set_volume(struct work_struct *work)
{
	s16 reading;
	u8 headphone_volume;
	u8 speaker_volume;

	if (gpio_have_volume_buttons()) { // have volume up/down buttons
		reading = dev->ADC_reading;
	} else {			  // have volume slider
		reading = adc_GetReading(LF1000_ADC_VOLUMESENSE);
		dev->ADC_reading = reading;	// save for sysfs
	}

	/*
 	 * filter out ADC measurement jitter.  Adjust volume
 	 * only if measurement is outside of capture window
 	 */
	if (VOLUME_CAPTURE_RANGE <= abs(dev->last_reading - reading)) {
		if (dev->last_reading < reading)
			dev->volumeUpCount++;	// increasing volume
		else
			dev->volumeDownCount++;	// decreasing volume
		dev->last_reading = reading;
	}
	headphone_volume = headphone_volume_convert(dev->last_reading);
	speaker_volume   = speaker_volume_convert(dev->last_reading);
 	
	/*
 	 * mute audio when device is closed
 	 * allow buffers to empty before muting
 	 */
	if (!dev->isOpen) {
		if (list_empty(&dev->scheduled_frags) &&
	    	    list_empty(&dev->filled_frags)    &&
		    !dev->isMute) {
			mute_speaker();
			mute_headphone();
			dev->isMute=1;
		}
	} else {
 		/* device open, mute/unmute as needed */
		if (dev->last_reading <= MUTE_HIGH) {	// mute
			if (!dev->isMute) {
				mute_speaker();
				mute_headphone();
				dev->isMute=1;
			}
		} else {			// unmute
			if (dev->isMute) {
				unmute_speaker();
				unmute_headphone();
				dev->isMute=0;
			}	
		}
	}

	/* adjust headphones if needed */	
	if(dev->headphone_volume != headphone_volume) { 
		set_headphone_volume(headphone_volume);
		dev->headphone_volume = headphone_volume;
	}

	/*  adjust speaker if needed */
	if(dev->speaker_volume != speaker_volume) {
		set_speaker_volume(speaker_volume);
		dev->speaker_volume = speaker_volume;
	}
}

static void lf1000_set_headphones(struct work_struct *work)
{
	int status;
       
	if(dev->headphone_mode < 0)
		return;

	status = gpio_get_val(lf1000_l2p_port(HEADPHONE_JACK),
			      lf1000_l2p_pin(HEADPHONE_JACK));

	if(status != dev->headphone_mode) { /* change in status */
		cs42L52_set_mixer(status);
		dev->headphone_mode = status;
	}
}

static void volume_monitor_task(unsigned long data)
{
	struct audio_device *a_dev = (struct audio_device *)data;

	queue_work(a_dev->codec_tasks, &a_dev->volume_work);
	a_dev->volume_timer.expires += VOLUME_SAMPLING_J;
	a_dev->volume_timer.function = volume_monitor_task;
	a_dev->volume_timer.data = data;
	add_timer(&a_dev->volume_timer);
}

static void headphone_monitor_task(unsigned long data)
{
	struct audio_device *a_dev = (struct audio_device *)data;

	queue_work(a_dev->codec_tasks, &a_dev->headphone_work);
	a_dev->headphone_timer.expires += HEADPHONE_SAMPLING_J;
	a_dev->headphone_timer.function = headphone_monitor_task;
	a_dev->headphone_timer.data = data;
	add_timer(&a_dev->headphone_timer);
}

/*******************************
 * fragment buffer functions
 *******************************/

/* Return true if there are enough empty frags to allow user to fill one */
static int empty_frag_available_no_lock(void)
{
	return list_empty(&dev->empty_frags) ? 0 : 1;
}

/* Return true if there are enough empty frags to allow user to fill one */
static int empty_frag_available(void)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&dev->empty_lock, flags);
	ret = empty_frag_available_no_lock();
	spin_unlock_irqrestore(&dev->empty_lock, flags);
	return ret;
}

/*******************************
 * character device operations *
 *******************************/

static void lw_erase (void)
{
	int i;
	dev->underrun = 0;
	dev->underrun_busy = 0;
	for (i=0; i<AUDIO_NUM_FRAGS+1; i++) {
		dev->filled_frag[i] = 0;
		dev->scheduled_frag[i] = 0;
	}
}

static ssize_t audio_write(struct file * file_p, const char *buf, size_t count,
				loff_t * ppos)
{
	unsigned long flags, flags2;
	struct frag *f;
	int size = count;
	int written = 0;
	int ret;

	do {
		/* sleep if no frags are available */
		spin_lock_irqsave(&dev->empty_lock, flags);
		while(!empty_frag_available_no_lock()) {
			spin_unlock_irqrestore(&dev->empty_lock, flags);
			if(file_p->f_flags & O_NONBLOCK) {
				return -EAGAIN;
			}
			if(wait_event_interruptible(dev->wqueue,
						    (empty_frag_available())))
				return -ERESTARTSYS;
			spin_lock_irqsave(&dev->empty_lock, flags);
		}

		/* Okay.  The empty_frags list is locked and has an available 
		 * fragment  Get the frag and remove it from the empty list */
		f = list_entry(dev->empty_frags.next, struct frag, list);
		list_del_init(&f->list);
		spin_unlock_irqrestore(&dev->empty_lock, flags);

		/* Now we have an empty fragment to copy our audio into */
		count = MIN(count, AUDIO_FRAG_SIZE);

		if(copy_from_user(f->buf, buf+written, count)) {
			/* whoops.  Put the frag back in the empty list */
			spin_lock_irqsave(&dev->empty_lock, flags);
			list_add_tail(&f->list, &dev->empty_frags);
			spin_unlock_irqrestore(&dev->empty_lock, flags);
			return -EFAULT;
		}

		lf1000_dma_initdesc(&f->desc);
		f->desc.src = (dma_addr_t)virt_to_phys((void *)(f->buf));
		f->desc.dst = dev->hardware_addr;
		f->desc.len = count;
		f->desc.flags = (DMA_DST_IO | DMA_DST_16BIT | DMA_DST_NOINC);
		f->desc.flags |= (DMA_SRC_MEM | DMA_SRC_8BIT);
		f->desc.id = DMA_PCMOUT;

		/* Now try to launch the dma transfer.  If you succeed, add the
		 * fragment to the scheduled list.  Otherwise, add it to the
		 * filled list and the dma handler will schedule it for us.
		 */
		spin_lock_irqsave(&dev->filled_lock, flags);
		spin_lock_irqsave(&dev->scheduled_lock, flags2);
		ret = lf1000_dma_launch(dev->dmachan, &f->desc);
		if(ret == 0) {
			list_add_tail(&f->list, &dev->scheduled_frags);
		} else {
			/* Assume EAGAIN */
			list_add_tail(&f->list, &dev->filled_frags);
		}
		spin_unlock_irqrestore(&dev->scheduled_lock, flags2);
		spin_unlock_irqrestore(&dev->filled_lock, flags);

		written += count;
	} while((file_p->f_flags & O_NONBLOCK) && written < size);

	return written;
}

static int audio_ioctl(struct inode *inode, struct file *filp, 
		unsigned int cmd, unsigned long arg)
{
	struct audio_buf_info info;

	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int ret = 0;
	int val;
	struct list_head *ptr, *next;
	unsigned long flags;

	switch(cmd) {
	/* get supported formats */
	case SNDCTL_DSP_GETFMTS:
	ret = put_user(AFMT_S16_LE, p);
	break;

	/* set format */
	case SNDCTL_DSP_SETFMT:
	if(get_user(val, p) != 0)
		return -EFAULT;
	/* (ignore requested format) */
	ret = put_user(AFMT_S16_LE, p); /* we left the format the same */
	break;

	/* report buffer information: recording
	 * (we do not support recording) */
	case SNDCTL_DSP_GETISPACE:
	memset((void *)&info, 0, sizeof(info));
	if(copy_to_user(p, &info, sizeof(info)) != 0)
		ret = -EFAULT;
	break;

	/* report buffer information: playback */
	case SNDCTL_DSP_GETOSPACE:
	/* count empty fragments */
	val = 0;
	spin_lock_irqsave(&dev->empty_lock, flags);
	list_for_each_safe(ptr, next, &dev->empty_frags) {
		val++;
	}
	spin_unlock_irqrestore(&dev->empty_lock, flags);
	info.fragments = val; /* (obsolete) */
	info.bytes = val*AUDIO_FRAG_SIZE; 
	info.fragstotal = AUDIO_NUM_FRAGS;
	info.fragsize = AUDIO_FRAG_SIZE;
	if(copy_to_user(p, &info, sizeof(info)) != 0) {
		ret = -EFAULT;
	}
	break;

	/* old way of asking for the fragment size */
	case SNDCTL_DSP_GETBLKSIZE:
	ret = put_user(AUDIO_FRAG_SIZE, p);
	break;

	/* set fragment size:
	 * (we don't support changing fragment sizes) */
	case SNDCTL_DSP_SETFRAGMENT:
	if(get_user(val, p) != 0)
		return -EFAULT;
	val = (AUDIO_NUM_FRAGS<<16)|9; /* TODO: generate selector from 
					  	AUDIO_FRAG_SIZE! */
	ret = put_user(val, p);
	break;

	/* we don't need to do anything for these: */
	case SNDCTL_DSP_SYNC: /* (deprecated) */
	case SNDCTL_DSP_GETTRIGGER: /* (deprecated) */
	case SNDCTL_DSP_SETTRIGGER: /* (don't have to support) */
	case SNDCTL_DSP_SETDUPLEX:
	case SNDCTL_DSP_STEREO:
	case SNDCTL_DSP_POST:
	case SNDCTL_DSP_GETODELAY:
	case SNDCTL_DSP_NONBLOCK:
	case SNDCTL_DSP_GETCAPS: /* (no capture support) */
	break;

	case SNDCTL_DSP_CHANNELS:
	if(get_user(val, p) != 0)
			return -EFAULT;
	/* TODO: set # channels */
	ret = put_user(2, p); /* we only allow 2 channels for now */
	break;

	case SNDCTL_DSP_SPEED:
	if(get_user(val, p) != 0)
			return -EFAULT;
	setAudioRate(val);
	ret = put_user(getAudioRate(), p);	/* return actual bit rate */
	break;

	/* Retrieve hardware playback volume.  We do not allow the volume to
	 * be set (only read) and we assume left volume = right volume. */
	case SOUND_MIXER_PCM:
	arg = 0xFF & (dev->last_reading >> 2); /* convert to 0-255 */
	ret = put_user((arg<<8)|(arg), p);
	break;

	default:
	//printk(KERN_INFO "audio: unknown ioctl #%d\n", _IOC_NR(cmd));
	ret = -ENOTTY;
	break;
	}

	return ret;
}

static unsigned int audio_poll(struct file *file, 
		struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	if(file->f_mode & FMODE_WRITE) {	
		poll_wait(file, &dev->wqueue, wait);
		if (empty_frag_available())
			mask |= POLLOUT | POLLWRNORM;
	}

	return mask;
}

static int audio_open(struct inode *inode, struct file *file)
{
	if (dev->isOpen) /* we have only one device */
		return -EBUSY;
	
	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		return -EINVAL;

	dev->isOpen++;
	cs42L52_write_byte(CS42L52_POWER_CTL_1, 0x9E);	/* power up audio */
	dev_info(&dev->pdev->dev, "isOpen=%d\n", dev->isOpen);
	return 0;
}

static int audio_release(struct inode *inode, struct file *file)
{
	cs42L52_write_byte(CS42L52_POWER_CTL_1, 0x9F); /* power down audio */
	dev->isOpen--;
	dev_info(&dev->pdev->dev, "isOpen=%d\n", dev->isOpen);
	return 0;
}

static struct file_operations audio_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= audio_ioctl,
	.write	= audio_write,
	.poll	= audio_poll,
	.open	= audio_open,
	.release = audio_release,
};

/****************************************
 * dma handling
 ***************************************/

/* DMA handers are called in IRQ context.  No sleeping! */
static void audio_dma_handler(int channel, void *priv, unsigned int pending)
{
	unsigned long flags, flags2, flags3;
	struct frag *current_frag;
	int scheduled;
	struct list_head *ptr;
	int ret;

#ifdef CONFIG_LF1000_AUDIO_STATS
	int filled_frag;
#endif

	/* we just finished writing out a scheduled frag.  Move it to the empty
	 * list and launch the DMA for one of the filled frags.
	 */
	if(unlikely(channel != dev->dmachan)) {
		/* should never get here! */
		printk(KERN_CRIT "dma handler called with bad channel!\n");
		return;
	}

	/* We need all the locks for this job */
	spin_lock_irqsave(&dev->empty_lock, flags);
	spin_lock_irqsave(&dev->filled_lock, flags2);
	spin_lock_irqsave(&dev->scheduled_lock, flags3);

	/* The DMA subsystem tells us how many DMA transfers are pending.  This
	 * number should be one less than the number of currently scheduled
	 * transfers (scheduled - 1).  If pending > (scheduled - 1), some rouge
	 * programmed a transfer without putting the fragment in the scheduled
	 * list.  Assume this never happens.  If pending < (scheduled - 1) it
	 * means we dropped an interrupt.  In this case, we free scheduled
	 * fragments until pending == (scheduled - 1).
	 */

	scheduled = 0;
	list_for_each(ptr, &dev->scheduled_frags) {
		scheduled++;
	}
#ifdef CONFIG_LF1000_AUDIO_STATS	
	filled_frag=pending;
	list_for_each(ptr, &dev->filled_frags) {
		filled_frag++;
	}
	dev->filled_frag[filled_frag]++;
	dev->scheduled_frag[scheduled]++;
#endif	
	if (pending != scheduled-1 || (scheduled != 2 && scheduled != 1))
		printk (KERN_ALERT "I2S: (pending=%d) != (scheduled=%d) - 1\n",
			pending, scheduled);
	do {
		/* first, put the scheduled frag back in the empty list.  There
		 * should always be at least one schedule frag if this dma
		 * handler was called!
		 */
		
		current_frag = list_entry(dev->scheduled_frags.next,
					  struct frag, list);
		list_del_init(&current_frag->list);
		scheduled--;
		list_add_tail(&current_frag->list, &dev->empty_frags);
		
		/* now schedule the next filled frag */
		if(!list_empty(&dev->filled_frags)) {
			current_frag = list_entry(dev->filled_frags.next,
						  struct frag, list);
			list_del_init(&current_frag->list);
			ret = lf1000_dma_launch(dev->dmachan, 
						&current_frag->desc);
			if(ret == 0) {
				list_add_tail(&current_frag->list,
					      &dev->scheduled_frags);
			} else if(ret == -EAGAIN) {
				/* this is pretty inconceivable */
				list_add_tail(&current_frag->list,
					      &dev->filled_frags);
			}
		}
	} while(pending < scheduled);

	lf1000_dma_int_clr(dev->dmachan);
	
	spin_unlock_irqrestore(&dev->scheduled_lock, flags3);
	spin_unlock_irqrestore(&dev->filled_lock, flags2);
	spin_unlock_irqrestore(&dev->empty_lock, flags);
	
	/* wake up writers */
	wake_up_interruptible(&dev->wqueue);	
}

static int lf1000_audio_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	struct frag *f;
	struct resource *res;
	unsigned long flags;
	struct list_head *ptr, *next;

	dev->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		dev_err(&pdev->dev, "failed to get resource\n");
		return -ENXIO;
	}

#ifdef CONFIG_LF1000_AUDIO_STATS
	lw_erase ();
#endif
	dev->headphone_mode = gpio_get_val(lf1000_l2p_port(HEADPHONE_JACK),
					   lf1000_l2p_pin(HEADPHONE_JACK));
	
	/* check i2c bus for our CS42L52 codec, try to initialize it */
	dev->i2c = i2c_get_adapter(0);
	if(dev->i2c == NULL)
		dev_err(&pdev->dev, "failed to connect to i2c bus\n");
	else { /* read chip ID from codec */
		dev->chip_id = cs42L52_read_byte(CS42L52_CHIP_ID);
		if(dev->chip_id < 0)
			dev_err(&pdev->dev, "failed to read chip ID\n");
		else {
			ret = cs42L52_init();
			if(ret < 0)
				dev_err(&pdev->dev, "codec init failed\n");
		}
	}

	mute_headphone();			// device closed, mute audio
	mute_speaker();
	dev->isMute = 1;			// audio muted
	dev->isOpen = 0;			// device not opened
	dev->volumeUpCount = 0;			/* number of times volume raised  */
	dev->volumeDownCount = 0;		/* number of times volume lowered */

	/* init volume settings */
	if (gpio_have_volume_buttons()) {	// have up/down volume buttons
		dev->last_reading = DIDJ_BUTTON_START_VOLUME;
		dev->ADC_reading = dev->last_reading;
	} else {				// have volume slider
		dev->last_reading = adc_GetReading(LF1000_ADC_VOLUMESENSE);
	}
	dev->headphone_volume = headphone_volume_convert(dev->last_reading);
	set_headphone_volume(dev->headphone_volume);
	dev->speaker_volume   = speaker_volume_convert(dev->last_reading);
	set_speaker_volume(dev->speaker_volume);

	/* allocate frag buffers */
	INIT_LIST_HEAD(&dev->empty_frags);
	INIT_LIST_HEAD(&dev->filled_frags);
	INIT_LIST_HEAD(&dev->fooled_frags);
	INIT_LIST_HEAD(&dev->scheduled_frags);
	dev->empty_lock = SPIN_LOCK_UNLOCKED;
	dev->filled_lock = SPIN_LOCK_UNLOCKED;
	dev->scheduled_lock = SPIN_LOCK_UNLOCKED;

	for(i=0; i<AUDIO_NUM_FRAGS; i++) {
		f = (struct frag *)kmalloc(sizeof(struct frag), GFP_KERNEL);
		if(!f) {
			ret = -ENOMEM;
			goto fail_frags;
		}
		INIT_LIST_HEAD(&f->list);
		f->buf =  (char *)kmalloc(AUDIO_FRAG_SIZE,
					  GFP_KERNEL|__GFP_DMA);
		if(!f->buf) {
			kfree(f);
			ret = -ENOMEM;
			goto fail_frags;
		}
		if (i < AUDIO_INIT_NUM_FRAGS)
			list_add_tail(&f->list, &dev->empty_frags);
		else
			list_add_tail(&f->list, &dev->fooled_frags);
	}

	/* Set up write buffer */
	init_waitqueue_head(&dev->wqueue);
	
	dev->dmachan = -1;
	dev->dmachan = lf1000_dma_request("i2s", DMA_PRIO_HIGH,
					  audio_dma_handler, NULL);
	if(dev->dmachan < 0) {
		ret = dev->dmachan;
		goto fail_dma;
	}

	lf1000_dma_int_en(dev->dmachan);

	ret = register_sound_dsp(&audio_fops, -1);
	if(ret < 0) {
		dev_err(&pdev->dev, "failed to register DSP\n");
		goto fail_register;
	}
	dev->minor = ret;

	/* configure I2S pins */
	for(i = 21; i <= 25; i++) {
		gpio_configure_pin(GPIO_PORT_A, i, GPIO_ALT1, 1, 0, 0);
	}

	dev->irq = platform_get_irq(pdev, 0);
	if(dev->irq < 0) {
		dev_err(&pdev->dev, "failed to get IRQ\n");
		ret = dev->irq;
		goto fail_irq;
	}
	ret = request_irq(dev->irq, audio_irq,
			IRQF_DISABLED|IRQF_SAMPLE_RANDOM, 
			"audio", NULL);
	if(ret) {
		dev_err(&pdev->dev, "requesting IRQ failed\n");
		goto fail_irq;
	}

	/* initialize devices */
	ret = initAudio(res);
 	if(ret < 0) {
		dev_err(&pdev->dev, "hardware initialization failed\n");
		goto fail_device_init;
	}
	
	/* set up I2S write address */
	dev->hardware_addr = res->start;

	/* set up volume monitoring if we found the codec chip */
	if(!(dev->chip_id < 0)) {
		/* set up work queue for using the codec */
		dev->codec_tasks = create_singlethread_workqueue("codec tasks");
		/* headphone control */
		INIT_WORK(&dev->headphone_work, lf1000_set_headphones);
		/* volume control */
		INIT_WORK(&dev->volume_work, lf1000_set_volume);

		/* grab initial volume setting */
		if (gpio_have_volume_buttons()) { // have up/down volume buttons
			ret = DIDJ_BUTTON_START_VOLUME;
		} else {			// have volume slider
			ret = adc_GetReading(LF1000_ADC_VOLUMESENSE);
		}
		if(ret < 0)
			dev_err(&pdev->dev, "initial volume read failed\n");
		else { /* set initial volume using current wheel setting */
			dev->headphone_volume =
				headphone_volume_convert((u16)ret);
			dev->speaker_volume = speaker_volume_convert((u16)ret);
		}

		/* set up periodic sampling */
		setup_timer(&dev->volume_timer, volume_monitor_task, 
			(unsigned long)dev);
		setup_timer(&dev->headphone_timer, headphone_monitor_task,
				(unsigned long)dev);
		dev->volume_timer.expires = get_jiffies_64() + 
						VOLUME_SAMPLING_J;
		dev->headphone_timer.expires = get_jiffies_64() +
						HEADPHONE_SAMPLING_J;

		add_timer(&dev->volume_timer); /* run */
		add_timer(&dev->headphone_timer);
	}

	sysfs_create_group(&pdev->dev.kobj, &audio_attr_group);
	return 0;

fail_device_init:
	free_irq(dev->irq, NULL);
	unregister_sound_dsp(dev->minor);
fail_irq:
fail_register:
	if(dev->dmachan >= 0) {
		lf1000_dma_int_dis(dev->dmachan);
		lf1000_dma_free(dev->dmachan);
	}
fail_dma:
fail_frags: 
	spin_lock_irqsave(&dev->empty_lock, flags);
	list_for_each_safe(ptr, next, &dev->empty_frags) {
		f = list_entry(ptr, struct frag, list);
		list_del(ptr);
		kfree(f->buf);
		kfree(f);
	}
	spin_unlock_irqrestore(&dev->empty_lock, flags);

	i2c_put_adapter(dev->i2c);
	return ret;
}

static int lf1000_audio_remove(struct platform_device *pdev)
{
	struct frag *f;
	struct list_head *ptr, *next;
	unsigned long flags;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	free_irq(dev->irq, NULL);

	setAudioPcmOutBuffer(0);	// disable PCM audio out
	deinitAudio(res);

	unregister_sound_dsp(dev->minor);
	i2c_put_adapter(dev->i2c);

	if(dev->dmachan >= 0) {
		lf1000_dma_int_dis(dev->dmachan);
		lf1000_dma_free(dev->dmachan);
	}

	spin_lock_irqsave(&dev->empty_lock, flags);
	list_for_each_safe(ptr, next, &dev->empty_frags) {
		f = list_entry(ptr, struct frag, list);
		list_del(ptr);
		kfree(f->buf);
		kfree(f);
	}
	spin_unlock_irqrestore(&dev->empty_lock, flags);
	
	spin_lock_irqsave(&dev->filled_lock, flags);
	list_for_each_safe(ptr, next, &dev->filled_frags) {
		f = list_entry(ptr, struct frag, list);
		list_del(ptr);
		kfree(f->buf);
		kfree(f);
	}
	list_for_each_safe(ptr, next, &dev->fooled_frags) {
		f = list_entry(ptr, struct frag, list);
		list_del(ptr);
		kfree(f->buf);
		kfree(f);
	}
	spin_unlock_irqrestore(&dev->filled_lock, flags);

	while(lf1000_dma_busy(dev->dmachan))
		msleep(10);
	lf1000_dma_int_dis(dev->dmachan);

	spin_lock_irqsave(&dev->scheduled_lock, flags);
	list_for_each_safe(ptr, next, &dev->scheduled_frags) {
		f = list_entry(ptr, struct frag, list);
		list_del(ptr);
		kfree(f->buf);
		kfree(f);
	}
	spin_unlock_irqrestore(&dev->scheduled_lock, flags);

	if(!(dev->chip_id < 0)) /* stop codec tasks */
		destroy_workqueue(dev->codec_tasks);

	sysfs_remove_group(&pdev->dev.kobj, &audio_attr_group);

	return 0;
}

static struct platform_driver lf1000_audio_driver = {
	.probe		= lf1000_audio_probe,
	.remove		= lf1000_audio_remove,
	.driver		= {
		.name	= "lf1000-audio",
		.owner	= THIS_MODULE,
	},
};

/****************************************
 *  module functions and initialization *
 ****************************************/
static void __exit audio_cleanup(void)
{
	platform_driver_unregister(&lf1000_audio_driver);
}

static int __init audio_init(void)
{
	return platform_driver_register(&lf1000_audio_driver);
}

module_init(audio_init);
module_exit(audio_cleanup);

MODULE_AUTHOR("Scott Esters, Andrey Yurovsky, Brian Cavagnolo");
MODULE_VERSION("1:1.0");
MODULE_LICENSE("GPL");
