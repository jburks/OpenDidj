/* Nand BBT Show-er
 * R Dowling 4/17/2008
 */

/* Lifted from... */
/* eraseall.c -- erase the whole of a MTD device

   Copyright (C) 2000 Arcom Control System Ltd

   Renamed to flash_eraseall.c

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include "crc32.h"

#include <mtd/mtd-user.h>
#include <mtd/jffs2-user.h>

#define PROGRAM "nandshowbbt"
#define VERSION "$Revision: 1.0 $"

static const char *exe_name;
static const char *mtd_device;

static void process_options (int argc, char *argv[]);
static void display_help (void);
static void display_version (void);

int show_ram = 0;
int do_scan = 0;

mtd_info_t meminfo;

void show_ram_bbt (int fd, int start_block, int end_block)
{
	int i;
	int any = 0;
	printf ("RAM BBT:\n");
	for (i=0; i<end_block; i++)
	{
		/* This uses the Bad Block Table that mtd assembled */
		loff_t ll_start = meminfo.erasesize * i; /* loff_t is long long */
		long start = ll_start;
		int is_bad = ioctl(fd, MEMGETBADBLOCK, &ll_start);
		if (is_bad)
		{
			any ++;
			printf ("%7d %08x", i, start);
			if (!(any % 5))
				printf ("\n");
		}
	}
	if (!any)
		printf ("None\n");
	printf ("\n");
}

void scan_oob (int fd)
{
	printf ("Scanning OOB:\n");
	if (ioctl(fd, BBTSCAN) != 0) {
		fprintf(stderr, "%s: %s: unable to BBTSCAN MTD device\n", exe_name, mtd_device);
		exit(1);
	}
}

int main (int argc, char *argv[])
{
	int fd;

	process_options(argc, argv);

	/* Get info on device */
	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}

	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		fprintf(stderr, "%s: %s: unable to get MTD device info\n", exe_name, mtd_device);
		exit(1);
	}

	int end_block = meminfo.size / meminfo.erasesize;
	if (show_ram)
		show_ram_bbt (fd, 0, end_block);
	if (do_scan)
		scan_oob (fd);

	close (fd);

	return 0;
}

void process_options (int argc, char *argv[])
{
	int error = 0;

	exe_name = argv[0];

	if (argc==1)
	{
		display_help();
		exit (0);
	}
	for (;;) {
		int option_index = 0;
		static const char *short_options = "rs";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"ram", no_argument, 0, 0},
			{"scan", no_argument, 0, 0},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
			case 0:
				switch (option_index) {
					case 0:
						display_help();
						break;
					case 1:
						display_version();
						break;
				}
				break;
			case 'h':
				display_help ();
				exit (0);
			case 'r':
				show_ram = 1;
				break;
			case 's':
				do_scan = 1;
				break;
			default:
				error = 1;
				break;
		}
	}
	if (optind == argc) {
		fprintf(stderr, "%s: no MTD device specified\n", exe_name);
		error = 1;
	}
	if (error) {
		fprintf(stderr, "Try `%s --help' for more information.\n",
				exe_name);
		exit(1);
	}

	mtd_device = argv[optind];
}


void display_help (void)
{
	printf("Usage: %s [OPTION] MTD_DEVICE\n"
	       "Display or recreate RAM version of BBT from OOB of specified MTD device.\n"
	       "\n"
	       "  -r, --ram      display the RAM version of the BBT\n"
	       "  -s, --scan     scan OOB of nand and construct RAM BBT\n"
	       "      --help     display this help and exit\n"
	       "      --version  output version information and exit\n",
	       exe_name);
	exit(0);
}


void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
			"\n"
			"Copyright (C) 2008 LeapFrog Enterprise\n"
			"\n"
			PROGRAM " comes with NO WARRANTY\n"
			"to the extent permitted by law.\n"
			"\n"
			"You may redistribute copies of " PROGRAM "\n"
			"under the terms of the GNU General Public Licence.\n"
			"See the file `COPYING' for more information.\n");
	exit(0);
}
