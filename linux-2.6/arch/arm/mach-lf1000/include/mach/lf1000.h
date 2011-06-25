/*
 *  mach-lf1000/include/mach/lf1000.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef PLL_HARDWARE_LF1000_H
#define PLL_HARDWARE_LF1000_H

struct lf1000_params {
	unsigned long	ref;
	unsigned long	vco_max;	/* inclusive */
	unsigned short	md_min;		/* inclusive */
	unsigned short	md_max;		/* inclusive */
	unsigned char	pd_min;		/* inclusive */
	unsigned char	pd_max;		/* inclusive */
};

struct lf1000_vco {
	unsigned short	m;
	unsigned char	p;
	unsigned char	s;
};

#endif
