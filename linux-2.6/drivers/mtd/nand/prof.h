/*
 *  drivers/mtd/nand/prof.h
 *
 * Copyright 2008 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/* Nand profiling support */

/* Define the types of nand operations we can accumulate data on */
enum prof_type { NS_READ, NS_WRITE, NS_ERASE, NS_LOCK, NS_MAX };

/* The collector function */
extern void nand_stats_accum (enum prof_type type, int in);
