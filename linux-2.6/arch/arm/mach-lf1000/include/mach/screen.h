/*
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * Describe platform screen modules and provide information on the system's
 * screen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef __LF1000_SCREEN_H__
#define __LF1000_SCREEN_H__

struct lf1000_screen_info {
	u16 xres;
	u16 yres;

	u32 clk_hz;

	/* horizonal sync */
	u16 hsw;	/* sync width */
	u16 hfp;	/* front porch */
	u16 hbp;	/* back porch */

	/* vertical sync */
	u16 vsw;	/* sync width */
	u16 vfp;	/* front porch */
	u16 vbp;	/* back porch */
};

struct lf1000_screen_info *lf1000_get_screen_info(void);

#endif /* __LF1000_SCREEN_H__ */
