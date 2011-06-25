/*
 *  LF1000 touchscreen driver
 *
 *  Author:	Scott Esters, <sesters@leapfrog.com>
 *  Created:	May 27, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This code is heavily based on ucb1x00-*.c copyrighted by Russell King
 */

#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <mach/gpio.h>
#include <mach/adc.h>

#include <linux/sysfs.h>

#define TOUCHSCREEN_SAMPLING_J	HZ / 100  // sample touchscreen every 10 ms

#define TOUCHSCREEN_MAJOR	247

#define TS_DEBOUNCE_DOWN 4	// default debounce down, min samples for down
#define TS_DEBOUNCE_UP	 1	// default debounce up, min samples for up
#define TS_HISTORY_MAX	50	// max length of sample history queue

// input event touch defines. X=0 and Y=0 is reserved for no touch.
#define TS_MIN_X	1	// min X resolution value
#define TS_MAX_X	1023	// max X resolution value
#define	TS_FUZZ_X	4	// gaussian filter window size, 0=none

#define TS_MIN_Y	1	// min Y resolution value
#define TS_MAX_Y	1023	// max Y resolution value
#define	TS_FUZZ_Y	4	// gaussian filter window size, 0=none

#define TS_DELAY_IN_US	150	// settling time before sample

// default stylus touch rectangle.  These are conservative values for
// use during device driver load and need to be adjusted for each device.
#define TS_TOUCH_MIN_X	100	// min valid X touch value
#define TS_TOUCH_MAX_X	930	// max valid X touch value
#define TS_TOUCH_MIN_Y	100	// min valid Y touch value
#define TS_TOUCH_MAX_Y	930	// max valid Y touch value

// Default rotation, translation, and shear values.  System should set these
// at startup.  Note order in /etc/pointercal is A1 A2 A0 A4 A5 A3 A6
#define TS_A0	25786444
#define TS_A1	  -29676
#define TS_A2	    -170
#define TS_A3	21441222	
#define TS_A4	    -632 
#define TS_A5	  -26927
#define TS_A6	   65536


static int abs_x[3] = {TS_MIN_X, TS_MAX_X, TS_FUZZ_X};
module_param_array(abs_x, int, NULL, 0);
MODULE_PARM_DESC(abs_x, "Touchscreen absolute X min, max, fuzz");

static int abs_y[3] = {TS_MIN_Y, TS_MAX_Y, TS_FUZZ_Y};
module_param_array(abs_y, int, NULL, 0);
MODULE_PARM_DESC(abs_y, "Touchscreen absolute Y min, max, fuzz");

struct touch {
	struct input_dev	*i_dev;

	struct task_struct	*ts_task;

	struct	timer_list touchscreen_timer;
	struct	workqueue_struct *touchscreen_tasks;
	struct	work_struct touchscreen_work;	// check touchscreen
	int	stop_timer;		// non-zero = stop timer reload

	int	sample_rate_in_jiffies;	  // screen sample rate
	int	debounce_in_samples_down; // samples before stylus down declared
	int	debounce_in_samples_up;   // samples before stylus up declared
	int	history_queue[TS_HISTORY_MAX][2]; // history queue
	int	queue_head;		  // write X,Y data here
	int	queue_tail;		  // read X,Y data here

	/*
	 * report_events, controls sending data up to Linux
	 * as well as debugging messages.  Also has bitfield definitions
	 *   bit 0 ==0 no events reported to Linux
	 *         ==1 report events to Linux
	 *   bit 1 ==1 show ADC debugging messages
	 *   bit 2 ==0 use read_xy() to read x ADC, then y ADC value
	 *         ==1 use read_yx() to read y ADC, then x ADC value
	 */
	int	report_events;

	int	delay_in_us;		// delay between GPIO setup and reading

	int	min_x;			// min screen touched
	int	min_y;			// min screen touched

	int	max_x;			// max screen touched
	int	max_y;			// max screen touched

	int	first_adc;		// 0=didj-ts, 4=Leapster 3

	int	adc_x;			// raw adc for x measurements
	int	adc_x2;			// raw adc for x measurements

	int	adc_y;			// raw adc for y measurements
	int	adc_y2;			// raw adc for y measurements

	int	touch_x, touch_y;	// raw ADC touchscreen x and y values
	int	touch_button;		// screen touched
	int	screen_x, screen_y;	// transformed screen x and y values

	int	screen_last_x, screen_last_y;  // debugging values

	int	stylus_down_count;	// count consecutive stylus down samples
	int	a[7];			// /etc/pointercal values

	struct cdev *cdev;		// char device
	int	dev;
	int	major;
} touch_dev;

struct touch * t_dev = &touch_dev;

/*
 * sysfs Interface
 */

static ssize_t show_name(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "Name is %s\n", __FILE__);
}
static DEVICE_ATTR(name, S_IRUSR|S_IRGRP|S_IROTH, show_name, NULL);

/* Return /etc/pointercal values.  pointercal order is A1 A2 A0 A4 A5 A3 A6 */
static ssize_t show_pointercal(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d %d %d %d %d %d\n",
		t_dev->a[1], t_dev->a[2], t_dev->a[0],
		t_dev->a[4], t_dev->a[5], t_dev->a[3], t_dev->a[6]);
}
static DEVICE_ATTR(pointercal, S_IRUSR|S_IRGRP|S_IROTH, show_pointercal, NULL);


/* report current x1 coordinate, 'x0' is used by calibration programs */
static ssize_t show_x0(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n",lf1000_touch_dev->adc_x);
}
static DEVICE_ATTR(x0, S_IRUSR|S_IRGRP|S_IROTH, show_x0, NULL);


/* report current y coordinate, 'y1' is used by calibration programs */
static ssize_t show_y1(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->adc_y);
}
static DEVICE_ATTR(y1, S_IRUSR|S_IRGRP|S_IROTH, show_y1, NULL);

static ssize_t show_touch_x(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->touch_x);
}
static DEVICE_ATTR(touch_x, S_IRUSR|S_IRGRP|S_IROTH, show_touch_x, NULL);

static ssize_t show_touch_y(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->touch_y);
}
static DEVICE_ATTR(touch_y, S_IRUSR|S_IRGRP|S_IROTH, show_touch_y, NULL);

static ssize_t show_touch_button(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->touch_button);
}
static DEVICE_ATTR(touch_button, S_IRUSR|S_IRGRP|S_IROTH, show_touch_button, NULL);

static ssize_t show_screen_x(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->screen_x);
}
static DEVICE_ATTR(screen_x, S_IRUSR|S_IRGRP|S_IROTH, show_screen_x, NULL);

static ssize_t show_screen_y(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->screen_y);
}
static DEVICE_ATTR(screen_y, S_IRUSR|S_IRGRP|S_IROTH, show_screen_y, NULL);

static ssize_t show_fuzz_x(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->i_dev->absfuzz[ABS_X]);
}
static ssize_t set_fuzz_x(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->i_dev->absfuzz[ABS_X] = temp;
	dev_dbg(dev, "%s.%s:%d fuzz_x=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->i_dev->absfuzz[ABS_X]);
	return(count);
}

static DEVICE_ATTR(fuzz_x, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_fuzz_x, set_fuzz_x);

static ssize_t show_fuzz_y(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;

	return sprintf(buf, "%d\n", lf1000_touch_dev->i_dev->absfuzz[ABS_Y]);
}
static ssize_t set_fuzz_y(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->i_dev->absfuzz[ABS_Y] = temp;
	dev_dbg(dev, "%s.%s:%d fuzz_y=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->i_dev->absfuzz[ABS_Y]);
	return(count);
}

static DEVICE_ATTR(fuzz_y, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_fuzz_y, set_fuzz_y);

static ssize_t show_delay_in_us(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->delay_in_us);
}
static ssize_t set_delay_in_us(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%u", &temp) != 1)
		return -EINVAL;
	if (temp < 0)
		return -EINVAL;
	lf1000_touch_dev->delay_in_us = temp;
	dev_dbg(dev, "%s.%s:%d delay_in_us=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->delay_in_us);
	return(count);
}

static DEVICE_ATTR(delay_in_us, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_delay_in_us, set_delay_in_us);

static ssize_t show_debounce_in_samples_down(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->debounce_in_samples_down);
}

static ssize_t set_debounce_in_samples_down(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%u", &temp) != 1)
		return -EINVAL;

	/* if value out of range then ignore it */
	if (temp < 0 || (TS_HISTORY_MAX / 2) < temp) {
		dev_err(dev, "%s: Invalid value %s\n",
			__FUNCTION__, buf);
		return (count);
	}

	lf1000_touch_dev->debounce_in_samples_down = temp;
	lf1000_touch_dev->stylus_down_count = 0;	/* reset counter */

	dev_dbg(dev, "%s.%s:%d debounce_in_samples_down=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->debounce_in_samples_down);
	return(count);
}

static DEVICE_ATTR(debounce_in_samples_down, \
	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, \
	show_debounce_in_samples_down, set_debounce_in_samples_down);

static ssize_t show_debounce_in_samples_up(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->debounce_in_samples_up);
}

static ssize_t set_debounce_in_samples_up(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%u", &temp) != 1)
		return -EINVAL;

	/* if value out of range then ignore it */
	if (temp < 0 || (TS_HISTORY_MAX / 2) < temp) {
		dev_err(dev, "%s: Invalid value %s\n",
			__FUNCTION__, buf);
		return (count);
	}

	lf1000_touch_dev->debounce_in_samples_up = temp;
	lf1000_touch_dev->stylus_down_count = 0;	/* reset counter */

	dev_dbg(dev, "%s.%s:%d debounce_in_samples_up=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->debounce_in_samples_up);
	return(count);
}

static DEVICE_ATTR(debounce_in_samples_up, \
	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, \
	show_debounce_in_samples_up, set_debounce_in_samples_up);


static ssize_t show_stylus_down_count(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->stylus_down_count);
}

static DEVICE_ATTR(stylus_down_count, \
	S_IRUSR|S_IRGRP|S_IROTH,
	show_stylus_down_count, NULL);

static ssize_t show_min_x(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->min_x);
}
static ssize_t set_min_x(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->min_x = temp;
	dev_dbg(dev, "%s.%s:%d min_x=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->min_x);
	return(count);
}

static DEVICE_ATTR(min_x, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_min_x, set_min_x);

static ssize_t show_max_x(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->max_x);
}
static ssize_t set_max_x(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->max_x = temp;
	dev_dbg(dev, "%s.%s:%d max_x=%d\n", __FILE__, __FUNCTION__, 
			__LINE__, lf1000_touch_dev->max_x);
	return(count);
}

static DEVICE_ATTR(max_x, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_max_x, set_max_x);

static ssize_t show_min_y(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->min_y);
}
static ssize_t set_min_y(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->min_y = temp;
	dev_dbg(dev, "%s.%s:%d min_y=%d\n", __FILE__, __FUNCTION__, 
			__LINE__, lf1000_touch_dev->min_y);
	return(count);
}

static DEVICE_ATTR(min_y, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_min_y, set_min_y);

static ssize_t show_max_y(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->max_y);
}
static ssize_t set_max_y(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->max_y = temp;
	dev_dbg(dev, "%s.%s:%d max_y=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->max_y);
	return(count);
}

static DEVICE_ATTR(max_y, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_max_y, set_max_y);

static ssize_t show_a0(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[0]);
}
static ssize_t set_a0(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[0] = temp;
	dev_dbg(dev, "%s.%s:%d a[0]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[0]);
	return(count);
}
static DEVICE_ATTR(a0, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a0, set_a0);

static ssize_t show_a1(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[1]);
}

static ssize_t set_a1(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[1] = temp;
	dev_dbg(dev, "%s.%s:%d a[1]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[1]);
	return(count);
}

static DEVICE_ATTR(a1, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a1, set_a1);

static ssize_t show_a2(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[2]);
}

static ssize_t set_a2(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[2] = temp;
	dev_dbg(dev, "%s.%s:%d a[2]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[2]);
	return(count);
}

static DEVICE_ATTR(a2, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a2, set_a2);


static ssize_t show_a3(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[3]);
}
static ssize_t set_a3(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[3] = temp;
	dev_dbg(dev, "%s.%s:%d a[3]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[3]);
	return(count);
}

static DEVICE_ATTR(a3, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a3, set_a3);

static ssize_t show_a4(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[4]);
}
static ssize_t set_a4(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[4] = temp;
	dev_dbg(dev, "%s.%s:%d a[4]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[4]);
	return(count);
}

static DEVICE_ATTR(a4, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a4, set_a4);

static ssize_t show_a5(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[5]);
}
static ssize_t set_a5(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[5] = temp;
	dev_dbg(dev, "%s.%s:%d a[5]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[5]);
	return(count);
}

static DEVICE_ATTR(a5, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a5, set_a5);

static ssize_t show_a6(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->a[6]);
}
static ssize_t set_a6(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	lf1000_touch_dev->a[6] = temp;
	dev_dbg(dev, "%s.%s:%d a[6]=%d\n",
		__FILE__, __FUNCTION__, __LINE__, lf1000_touch_dev->a[6]);
	return(count);
}

static DEVICE_ATTR(a6, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_a6, set_a6);

static ssize_t show_sample_rate_in_hz(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rate;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	rate = lf1000_touch_dev->sample_rate_in_jiffies;
	rate = (0 < rate) ? (HZ / rate) : 0 ;	// expect positive value
	return sprintf(buf, "%d\n", rate);
}
static ssize_t set_sample_rate_in_hz(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%u", &temp) != 1)
		return -EINVAL;
	temp = (0 < temp )   ? temp : 1;  // set min rate to 1
	temp = (temp <= HZ ) ? temp : HZ; // set max rate to HZ
	lf1000_touch_dev->sample_rate_in_jiffies = (HZ / temp); 
	dev_dbg(dev,  "%s.%s:%d sample_rate_in_jiffies=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->sample_rate_in_jiffies);
	return(count);
}

static DEVICE_ATTR(sample_rate_in_hz, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_sample_rate_in_hz, set_sample_rate_in_hz);

static ssize_t show_report_events(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	return sprintf(buf, "%d\n", lf1000_touch_dev->report_events);
}
static ssize_t set_report_events(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int temp;
	struct touch *lf1000_touch_dev = (struct touch *)dev->driver_data;
	if(sscanf(buf, "%i", &temp) != 1)
		return -EINVAL;
	if (temp < 0 || temp > 7)	// ensure value between 0 and 7
		return -EINVAL;
	lf1000_touch_dev->report_events = temp;
	dev_dbg(dev, "%s.%s:%d report_events=%d\n",
		__FILE__, __FUNCTION__, __LINE__,
		lf1000_touch_dev->report_events);
	return(count);
}

static DEVICE_ATTR(report_events, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, show_report_events, set_report_events);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_name.attr,
	&dev_attr_pointercal.attr,
	&dev_attr_x0.attr,
	&dev_attr_y1.attr,
	&dev_attr_touch_x.attr,
	&dev_attr_touch_y.attr,
	&dev_attr_touch_button.attr,
	&dev_attr_screen_x.attr,
	&dev_attr_screen_y.attr,
	&dev_attr_fuzz_x.attr,
	&dev_attr_fuzz_y.attr,
	&dev_attr_delay_in_us.attr,
	&dev_attr_debounce_in_samples_down.attr,
	&dev_attr_debounce_in_samples_up.attr,
	&dev_attr_stylus_down_count.attr,
	&dev_attr_min_x.attr,
	&dev_attr_max_x.attr,
	&dev_attr_min_y.attr,
	&dev_attr_max_y.attr,
	&dev_attr_a0.attr,
	&dev_attr_a1.attr,
	&dev_attr_a2.attr,
	&dev_attr_a3.attr,
	&dev_attr_a4.attr,
	&dev_attr_a5.attr,
	&dev_attr_a6.attr,
	&dev_attr_sample_rate_in_hz.attr,
	&dev_attr_report_events.attr,
	NULL
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes
};

void set_read_x(void)
{
	// float X1 and X2 first, then drive Y1 high and Y2 low
	// gpio_configure_pin(PORT, PIN, FUNCTION, OUT=1, PULLUP=1, VALUE)
	// Set X1 as input with pullup
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 0, 1, 0);
	// Set X2 as input with pullup
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2), GPIO_GPIOFN, 0, 1, 0);
	// Set Y1 as output high
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 1, 0, 1);
	// Set Y2 as output low
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2), GPIO_GPIOFN, 1, 0, 0);
}

void set_read_y(void)
{
	// float Y1 and Y2 first, then drive X1 high and X2 low
	// gpio_configure_pin(PORT, PIN, FUNCTION, OUT=1, PULLUP=1, VALUE)
	// Set Y1 as input with pullup
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_GPIOFN, 0, 1, 0);
	// Set Y2 as input with pullup
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2), GPIO_GPIOFN, 0, 1, 0);
	// Set X1 as output high
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_GPIOFN, 1, 0, 1);
	// Set X2 as output low
	gpio_configure_pin(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2), GPIO_GPIOFN, 1, 0, 0);
}

/*
 * int read_xy(void) -- returns 0==Stylus UP, 1==Stylus DOWN, have ADC values.
 * Read X and Y touchscreen values, returning Stylus UP / Stylus DOWN results.
 * Assume any ADC value out of Stylus DOWN range means the Stylus is UP.
 */
static int read_xy(void)
{
	/*
	 * read first set of points
	 */
	set_read_x();			// set GPIO pins to read X value
	udelay(t_dev->delay_in_us);	// Let the screen settle and charge up
	t_dev->adc_x  = adc_GetReading(t_dev->first_adc);     // read X ADC
	t_dev->adc_x2 = adc_GetReading(t_dev->first_adc + 2); // read X ADC
	set_read_y();			// set GPIO pins to read Y value
	udelay(t_dev->delay_in_us);	// Let the screen settle and charge up
	t_dev->adc_y  = adc_GetReading(t_dev->first_adc + 1); // read Y ADC
	t_dev->adc_y2 = adc_GetReading(t_dev->first_adc + 3); // read Y ADC

	/* check for valid point */
	if (t_dev->adc_x  < t_dev->min_x || t_dev->max_x < t_dev->adc_x  ||
	    t_dev->adc_x2 < t_dev->min_x || t_dev->max_x < t_dev->adc_x2 ||
	    t_dev->adc_y  < t_dev->min_y || t_dev->max_y < t_dev->adc_y  ||
	    t_dev->adc_y2 < t_dev->min_y || t_dev->max_y < t_dev->adc_y2)
		return 0;		// stylus UP

	return 1;			// stylus DOWN
}

/*
 * int read_yx(void) -- returns 0==Stylus UP, 1==Stylus DOWN, have ADC values.
 * Read Y and X touchscreen values, returning Stylus UP / Stylus DOWN results.
 * Assume any ADC value out of Stylus DOWN range means the Stylus is UP.
 */
static int read_yx(void)
{
	/*
	 * read first set of points
	 */
	set_read_y();			// set GPIO pins to read Y value
	udelay(t_dev->delay_in_us);	// Let the screen settle and charge up
	t_dev->adc_y  = adc_GetReading(t_dev->first_adc + 1); // read Y ADC
	t_dev->adc_y2 = adc_GetReading(t_dev->first_adc + 3); // read Y ADC
	set_read_x();			// set GPIO pins to read X value
	udelay(t_dev->delay_in_us);	// Let the screen settle and charge up
	t_dev->adc_x  = adc_GetReading(t_dev->first_adc);     // read X ADC
	t_dev->adc_x2 = adc_GetReading(t_dev->first_adc + 2); // read X ADC

	/* check for valid point */
	if (t_dev->adc_x  < t_dev->min_x || t_dev->max_x < t_dev->adc_x  ||
	    t_dev->adc_x2 < t_dev->min_x || t_dev->max_x < t_dev->adc_x2 ||
	    t_dev->adc_y  < t_dev->min_y || t_dev->max_y < t_dev->adc_y  ||
	    t_dev->adc_y2 < t_dev->min_y || t_dev->max_y < t_dev->adc_y2)
		return 0;		// stylus UP

	return 1;			// stylus DOWN
}

/*
 * void get_touch(struct work_struct work)
 * read touchscreen, debounce stylus up/down, and report screen activity.
 */

static void get_touch(struct work_struct *work)
{
	int report_events = t_dev->report_events; // keep, as value may change

	t_dev->touch_button = (report_events & 1<<2) ? read_yx() : read_xy();

	/* increment stylus up / down capture counters */
	if (t_dev->touch_button) {
		/* stylus down */
		t_dev->stylus_down_count = t_dev->stylus_down_count + 1;
		t_dev->stylus_down_count = min ( t_dev->stylus_down_count,
	   t_dev->debounce_in_samples_down + t_dev->debounce_in_samples_up );
	   
	} else { /* stylus up */
		t_dev->stylus_down_count = 0;
		t_dev->queue_head = 0;		/* reset queue	*/
		t_dev->queue_tail = 0;		/* reset queue	*/
	}

	if (0 == t_dev->stylus_down_count) {
		if (report_events & 1<<0)	/* report stylus up */
			input_report_key(t_dev->i_dev, BTN_TOUCH, 0);

	/* stylus is down */
	} else if (t_dev->debounce_in_samples_down < t_dev->stylus_down_count) {
		/* save touchscreen data in queue */
		t_dev->history_queue[t_dev->queue_head][0]=t_dev->adc_x;
		t_dev->history_queue[t_dev->queue_head][1]=t_dev->adc_y;

		t_dev->queue_head = t_dev->queue_head + 1;	/* inc */
		if (t_dev->debounce_in_samples_up <= t_dev->queue_head)
			t_dev->queue_head = 0;			/* wrap around*/

		/* report stylus up or down */
		if (t_dev->stylus_down_count < (t_dev->debounce_in_samples_down +
					  t_dev->debounce_in_samples_up)) {
			/* collect data, report stylus up */
			if (report_events & 1<<0)	/* report stylus up */
				input_report_key(t_dev->i_dev, BTN_TOUCH, 0);

		} else { /* data valid, report stylus down */
			if (report_events & 1<<0) /* say stylus down */
			    input_report_key(t_dev->i_dev, BTN_TOUCH, 1);

			/* get prior data */
			t_dev->touch_x =
				t_dev->history_queue[t_dev->queue_tail][0];
			t_dev->touch_y =
				t_dev->history_queue[t_dev->queue_tail][1];
			t_dev->queue_tail = t_dev->queue_tail + 1; /* inc */
			if (t_dev->debounce_in_samples_up <= t_dev->queue_tail)
				t_dev->queue_tail = 0;
		
			/* convert position */
			t_dev->screen_x =
				(t_dev->a[1] * t_dev->touch_x +
				 t_dev->a[2] * t_dev->touch_y +
				 t_dev->a[0]) / t_dev->a[6];
			t_dev->screen_y =
				(t_dev->a[4] * t_dev->touch_x +
				 t_dev->a[5] * t_dev->touch_y +
				 t_dev->a[3]) / t_dev->a[6];

			/* debugging data; showing output as data changes */
			if ((report_events & 1<<1) &&
		   		(t_dev->screen_x != t_dev->screen_last_x ||
		    		 t_dev->screen_y != t_dev->screen_last_y)) {
				dev_dbg(&t_dev->i_dev->dev,
			"t_x:%d t_x2:%d t_y:%d t_y2:%d s_x:%d s_y:%d\n",
					t_dev->adc_x, t_dev->adc_x2,
					t_dev->adc_y, t_dev->adc_y2,
					t_dev->screen_x, t_dev->screen_y);
				t_dev->screen_last_x = t_dev->screen_x;
				t_dev->screen_last_y = t_dev->screen_y;
			}

			if (report_events & 1<<0) {	/* report prior XY */
				input_report_abs(
					t_dev->i_dev, ABS_X, t_dev->screen_x);
				input_report_abs(
					t_dev->i_dev, ABS_Y, t_dev->screen_y);
			}
		}
	}

	if (report_events & 1<<0)	/* report sync */
		input_sync(t_dev->i_dev);
	
}

void touchscreen_monitor_task(unsigned long data)
{
	struct touch *t_dev = (struct touch *)data;

	if (!t_dev->stop_timer) {	// not stopping, reload timer
		queue_work(t_dev->touchscreen_tasks, &t_dev->touchscreen_work);
		t_dev->touchscreen_timer.expires += t_dev->sample_rate_in_jiffies;
		t_dev->touchscreen_timer.function = touchscreen_monitor_task;
		t_dev->touchscreen_timer.data = data;
		add_timer(&t_dev->touchscreen_timer);
	}
}

int touchscreen_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
                                          unsigned long arg)
{
	return 0;
}
	
struct file_operations touchscreen_fops = {
	.owner = THIS_MODULE,
	.ioctl = touchscreen_ioctl,
};

static int lf1000_ts_probe(struct platform_device *pdev)
{
	struct input_dev *i_dev;
	int error;

	i_dev = input_allocate_device();

	if (!i_dev)  {
		error = -ENOMEM;
		goto err_free_devs;
	}

	i_dev->name		= "LF1000 touchscreen interface";
	i_dev->phys		= "lf1000/touchscreen";
	i_dev->id.bustype	= BUS_HOST;
	i_dev->id.vendor	= 0x0001;
	i_dev->id.product	= 0x0001;
	i_dev->id.version	= 0x0001;
	t_dev->i_dev		= i_dev;

	/* initial X and Y clipping values */
	t_dev->min_x		= TS_TOUCH_MIN_X;
	t_dev->max_x		= TS_TOUCH_MAX_X;

	t_dev->min_y		= TS_TOUCH_MIN_Y;
	t_dev->max_y		= TS_TOUCH_MAX_Y;

	t_dev->delay_in_us	= TS_DELAY_IN_US;

	/* set GPIOs to max output current */
	gpio_set_cur(lf1000_l2p_port(TOUCHSCREEN_X1),
		lf1000_l2p_pin(TOUCHSCREEN_X1), GPIO_CURRENT_8MA);
	gpio_set_cur(lf1000_l2p_port(TOUCHSCREEN_Y1),
		lf1000_l2p_pin(TOUCHSCREEN_Y1), GPIO_CURRENT_8MA);
	gpio_set_cur(lf1000_l2p_port(TOUCHSCREEN_X2),
		lf1000_l2p_pin(TOUCHSCREEN_X2), GPIO_CURRENT_8MA);
	gpio_set_cur(lf1000_l2p_port(TOUCHSCREEN_Y2),
		lf1000_l2p_pin(TOUCHSCREEN_Y2), GPIO_CURRENT_8MA);

	/*
 	 * set first ADC channel hooked to touchscreen
 	 * Didj TS uses ADC[0] to ADC[3]
 	 * Leapster 3 uses ADC[4] to ADC[7]
 	 */

	if (gpio_have_gpio_dev() || gpio_have_gpio_didj())
		t_dev->first_adc = 0;
	else if (gpio_have_gpio_acorn() || gpio_have_gpio_emerald())
		t_dev->first_adc = 4;
	else {
		t_dev->first_adc = 4;
		dev_dbg(&t_dev->i_dev->dev,
				"%s.%s:%d unknown touchscreen interface",
				__FILE__, __FUNCTION__, __LINE__);
	}

	/* event types that we support */	
	i_dev->evbit[0]		= BIT(EV_KEY) | BIT(EV_ABS);
	i_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	/* initial screen coordination      min=[0]   max=[1]  fuzz=[2]  */
	input_set_abs_params(i_dev, ABS_X, abs_x[0], abs_x[1], abs_x[2], 0);
	input_set_abs_params(i_dev, ABS_Y, abs_y[0], abs_y[1], abs_y[2], 0);
	platform_set_drvdata(pdev, t_dev);

	/* set range of X and Y coordinates */
	
	error = input_register_device(i_dev);
	if(error)
		goto err_free_devs;

	/* setup work queue */
	t_dev->touchscreen_tasks = create_singlethread_workqueue("touchscreen tasks");
	INIT_WORK(&t_dev->touchscreen_work, get_touch);

	/* grab initial touchscreen settings */
	read_xy();

	/* set default /etc/pointercal values */
	t_dev->a[0] = TS_A0;
	t_dev->a[1] = TS_A1;
	t_dev->a[2] = TS_A2;
	t_dev->a[3] = TS_A3;
	t_dev->a[4] = TS_A4;
	t_dev->a[5] = TS_A5;
	t_dev->a[6] = TS_A6;

	/* setup periodic sampling */
	t_dev->sample_rate_in_jiffies   = TOUCHSCREEN_SAMPLING_J;
	t_dev->debounce_in_samples_down = TS_DEBOUNCE_DOWN; /* min down */
	t_dev->debounce_in_samples_up   = TS_DEBOUNCE_UP;  /* min up    */
	t_dev->queue_head = 0;		/* write X,Y here	*/
	t_dev->queue_tail = 0;		/* read X,Y here	*/
	t_dev->report_events = 1;	/* send input_event data */
	t_dev->stylus_down_count = 0;	/* # consecutive stylus down samples */
	setup_timer(&t_dev->touchscreen_timer, touchscreen_monitor_task,
		(unsigned long)t_dev);
	t_dev->touchscreen_timer.expires = get_jiffies_64() +
		t_dev->sample_rate_in_jiffies;

	add_timer(&t_dev->touchscreen_timer);	/*run*/

	sysfs_create_group(&pdev->dev.kobj, &touchscreen_attr_group);
	return 0;

err_free_devs:
	if (&t_dev->touchscreen_timer != NULL) {
		t_dev->stop_timer = 1;		// don't reload timer
		del_timer_sync(&t_dev->touchscreen_timer);
	}
	sysfs_remove_group(&pdev->dev.kobj, &touchscreen_attr_group);
	input_free_device(t_dev->i_dev);
	return error;
}

static int lf1000_ts_remove(struct platform_device *pdev)
{
	struct touch *t_dev = platform_get_drvdata(pdev);

	if (&t_dev->touchscreen_timer != NULL) {
		t_dev->stop_timer = 1;		// don't reload timer
		del_timer_sync(&t_dev->touchscreen_timer);
	}

	destroy_workqueue(t_dev->touchscreen_tasks);

	sysfs_remove_group(&pdev->dev.kobj, &touchscreen_attr_group);

	input_unregister_device(t_dev->i_dev);
	return 0;
}

/* fake release function to quiet "does not have a release()" warning */

void lf1000_ts_release(struct device *dev)
{
}

static struct platform_device lf1000_ts_device = {
	.name		= "lf1000-touchscreen",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.release	= lf1000_ts_release,
	}
};

static struct platform_driver lf1000_ts_driver = {
	.probe		= lf1000_ts_probe,
	.remove		= lf1000_ts_remove,
	.driver		= {
		.name	= "lf1000-touchscreen",
		.owner	= THIS_MODULE,
		},
};

static int __init lf1000_ts_init(void)
{
	int ret;
	ret = platform_device_register(&lf1000_ts_device);
	ret = platform_driver_register(&lf1000_ts_driver);
	return(ret);
}

static void __exit lf1000_ts_exit(void)
{
	platform_driver_unregister(&lf1000_ts_driver);
	platform_device_unregister(&lf1000_ts_device);
}

module_init(lf1000_ts_init);
module_exit(lf1000_ts_exit);

MODULE_AUTHOR("Scott Esters <sesters@leapfrog.com>");
MODULE_DESCRIPTION("LF1000 Touchscreen driver");
MODULE_LICENSE("GPL");

