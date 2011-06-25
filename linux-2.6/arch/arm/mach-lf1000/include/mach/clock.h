/*
 *  mach-lf1000/include/mach/clock.h
 *
 *      Copyright (C) 2007 Kosta Demirev <kdemirev@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef CLK_LF1000_H
#define CLK_LF1000_H

struct module;
struct lf1000_params;

struct clk {
	struct list_head		node;
	unsigned long			rate;
	unsigned long			number;
	struct module			*owner;
	const char			*name;
	const struct lf1000_params	*params;
	void				*data;
	void				(*setvco)(struct clk *);
};

int clk_register(struct clk *clk);
void clk_unregister(struct clk *clk);
#endif
