/* Nand Scrubber, ala u-boot's nand scrubber
 * R Dowling 4/8/2008
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

/* From #include <linux/mtd/nand.h> */
#define NAND_SMALL_BADBLOCK_POS		5
#define NAND_LARGE_BADBLOCK_POS		0

#define PROGRAM "nandscrub"
#define VERSION "$Revision: 1.0 $"

static const char *exe_name;
static const char *mtd_device;
/* Some args */
static int quiet;		/* true -- don't output progress */
static int jffs2;		// format for jffs2 usage
static long arg_start_block = 0;
static long arg_n_blocks = 0;
static int pages_too = 0;
static int do_erase = 0;
static int do_mark = 0;
static int do_view = 0;
static int narrow_view = 0;
static int just_bad_blocks = 0;

static void process_options (int argc, char *argv[]);
static void display_help (void);
static void display_version (void);
static struct jffs2_unknown_node cleanmarker;
int target_endian = __BYTE_ORDER;

int nand_bad_block_scrub(mtd_info_t *a, loff_t b, int c)
{
	return 0;
}

static int save_bbt_protects_erase;
static const char *sysfs = "/sys/devices/platform/lf1000-nand/bbt_protects_erase";

#if 0
void setup_sysfs ()
{
	int fd;
	/* Read and save old value */
	if ((fd = open(sysfs, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	char c;
	if (read (fd, &c, 1) != 1) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	save_bbt_protects_erase = (c == '1') ? 1 : 0;
	if (close (fd)) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	/* Write a 0 */
	if ((fd = open(sysfs, O_WRONLY)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	c = '0';
	if (write (fd, &c, 1) != 1) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	if (close (fd)) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
}

void takedown_sysfs ()
{
	int fd;
	/* Restore value */
	if ((fd = open(sysfs, O_WRONLY)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	char c = save_bbt_protects_erase ? '1' : '0';
	if (write (fd, &c, 1) != 1) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	if (close (fd)) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
}
#endif

/* This is the central problem: Must ignore bad blocks during OOB erase */

/* This from "u-boot" "scrub" command: */

/* scrub option allows to erase badblock. To prevent internal
 * check from erase() method, set block check method to dummy
 * and disable bad block table while erasing.
 */
 /*
         struct nand_chip *priv_nand = meminfo->priv;

		nand_block_bad_old = priv_nand->block_bad;
		priv_nand->block_bad = nand_block_bad_scrub;

		/* we don't need the bad block table anymore...
		 * after scrub, there are no bad blocks left!
		 *
		if (priv_nand->bbt) {
			kfree(priv_nand->bbt);
		}
		priv_nand->bbt = NULL;
	}
*/

int main (int argc, char *argv[])
{
	mtd_info_t meminfo;
	int fd, clmpos = 0, clmlen = 8;
	erase_info_t erase;
	int isNAND, bbtest = 1;
	unsigned char oobbuf[64];
	unsigned char badmarkbuf[64];
	struct mtd_oob_buf oob = {0, 16, oobbuf};

	process_options(argc, argv);

	/* If not marking or not erasing, then turn on viewing */
	if (!do_mark && !do_erase)
		do_view = 1;

	/* Get info on device */
	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}
	

	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		fprintf(stderr, "%s: %s: unable to get MTD device info\n", exe_name, mtd_device);
		exit(1);
	}

	/* Prepare the OOB image of a factory marked bad block */
	int i;
	for (i=0; i<64; i++)
		badmarkbuf[i] = 0xff;
	badmarkbuf[NAND_LARGE_BADBLOCK_POS] = 0; /* 0 */
	badmarkbuf[NAND_SMALL_BADBLOCK_POS] = 0; /* 5 */

	/* If erasing, set up sysfs in mtd nand driver to allow it */
	/* void setup_sysfs () */

	/* Get mtd_info_t */
	/*
struct mtd_info_user {
	uint8_t type;
	uint32_t flags;
	uint32_t size;	 // Total size of the MTD
	uint32_t erasesize;
	uint32_t writesize;
	uint32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	uint32_t ecctype;
	uint32_t eccsize;
};
	printf ("meminfo.type=%d=%x\n", meminfo.type, meminfo.type);
	printf ("meminfo.flags=%d=%x\n", meminfo.flags, meminfo.flags);
	printf ("meminfo.size=%d=%x\n", meminfo.size, meminfo.size);
	printf ("meminfo.erasesize=%d=%x\n", meminfo.erasesize, meminfo.erasesize);
	printf ("meminfo.writesize=%d=%x\n", meminfo.writesize, meminfo.writesize);
	printf ("meminfo.oobsize=%d=%x\n", meminfo.oobsize, meminfo.oobsize);
	printf ("meminfo.ecctype=%d=%x\n", meminfo.ecctype, meminfo.ecctype);
	printf ("meminfo.eccsize=%d=%x\n", meminfo.eccsize, meminfo.eccsize);
	*/

	/* Get oobinfo */
	/* struct nand_oobinfo oobinfo; */
	/*
struct nand_oobinfo {
	uint32_t useecc;
	uint32_t eccbytes;
	uint32_t oobfree[8][2];
	uint32_t eccpos[32];
};
	if (ioctl(fd, MEMGETOOBSEL, &oobinfo) != 0) {
		fprintf(stderr, "%s: %s: unable to get NAND oobinfo\n", exe_name, mtd_device);
		exit(1);
	}
	printf ("oobinfo.useecc=%d=%x (0x%x)\n", oobinfo.useecc, oobinfo.useecc, MTD_NANDECC_AUTOPLACE);
	printf ("oobinfo.eccbytes=%d=%x\n", oobinfo.eccbytes, oobinfo.eccbytes);
	for (i=0; i<8; i++)
	{
		int j;
		for (j=0; j<2; j++)
		{
			printf ("oobfree[%d][%d]=%d=%x\n", i, j, oobinfo.oobfree[i][j], oobinfo.oobfree[i][j]);
		}
	}
	for (i=0; i<meminfo.eccsize; i++)
	{
		printf ("eccpos[%d]=%d=%x\n", i, oobinfo.eccpos[i], oobinfo.eccpos[i]);
	}
	*/

	/* Set up unchanging data for ioctls */
	erase.length = meminfo.erasesize;
	oob.length = meminfo.oobsize;

	int end_block = arg_n_blocks ? 
		arg_n_blocks + arg_start_block : 
		meminfo.size / meminfo.erasesize - arg_start_block;
	for (i=arg_start_block; i<end_block; i++)
	{
		/* This uses the Bad Block Table that mtd assembled */
		loff_t ll_start = meminfo.erasesize * i; /* loff_t is long long */
		long start = ll_start;
		int is_bad = ioctl(fd, MEMGETBADBLOCK, &ll_start);
		if (is_bad)
			printf ("MTD scan at boot shows block %5d start=%08x bad (ret=%d).\n", i, start, is_bad);
		if (just_bad_blocks && !is_bad)
			continue;

		int npages = meminfo.erasesize / meminfo.writesize;
		int k;
		for (k=0; k<npages; k++)
		{
			if (do_erase || do_mark)
			{
				erase.start = ll_start;
				if (ioctl(fd, MEMERASE, &erase) != 0) {
					fprintf(stderr, "\n%s: %s: MTD Erase failure: %s\n", exe_name, mtd_device, strerror(errno));
				}
				else
				{
					printf ("Erase     %5d page %2d %08x=%10d\n", i, k, start, start);
				}
			}
			if (do_mark)
			{
				oob.ptr = (unsigned char *) &badmarkbuf;
				oob.start = ll_start;
				if (ioctl (fd, MEMWRITEOOB, &oob) != 0) {
					fprintf(stderr, "\n%s: %s: MTD Write failure: %s\n", exe_name, mtd_device, strerror(errno));
				}
				else
				{
					printf ("Mark      %5d page %2d %08x=%10d\n", i, k, start, start);
				}
			}

			if (do_view)
			{
				/* Read OOB data */
				oob.ptr = (unsigned char *) &oobbuf;
				oob.start = ll_start;
				if (ioctl(fd, MEMREADOOB, &oob) != 0) {
					printf ("error reading OOB at %08x\n", start);
				}
				else
				{
					int j;
					printf ("OOB block %5d page %2d %08x=%10d: ", i, k, start, start);
					int end = meminfo.oobsize;
					if (narrow_view && end > 20)
						end = 20;
					for (j=0; j<end; j++)
					{
						printf (" %02x", oobbuf[j]);
					}
					printf ("\n");
				}
			}

			if (!pages_too)
				break; /* Don't show/operate on pages other than first in erase block */

			ll_start += meminfo.writesize;
			start += meminfo.writesize;
		}
	}
	return 0;
}

void process_options (int argc, char *argv[])
{
	int error = 0;

	exe_name = argv[0];

	if (argc==1)
	{
		display_help ();
		exit (0);
	}

	for (;;) {
		int option_index = 0;
		static const char *short_options = "hejml:pqs:vw";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"version", no_argument, 0, 0},
			{"justbad", no_argument, 0, 'j'},
			{"quiet", no_argument, 0, 'q'},
			{"silent", no_argument, 0, 'q'},
			{"start", required_argument, 0, 's'},
			{"length", required_argument, 0, 'l'},
			{"pages", no_argument, 0, 'p'},
			{"erase", no_argument, 0, 'e'},
			{"mark", no_argument, 0, 'm'},
			{"view", no_argument, 0, 'v'},
			{"narrow", no_argument, 0, 'w'},
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
			case 'v':
				do_view = 1;
				break;
			case 'w':
				narrow_view = 1;
				break;
			case 'm':
				do_mark = 1;
				printf ("** Marking blocks! **\n");
				break;
			case 'e':
				do_erase = 1;
				printf ("** Erasing blocks enabled! **\n");
				break;
			case 'p':
				pages_too = 1;	/* Show/operate on pages other than first within erase block */
				break;
			case 's':
				arg_start_block = atoi (optarg);
				printf ("start=%d=%x\n", arg_start_block, arg_start_block);
				break;
			case 'l':
				arg_n_blocks = atoi (optarg);
				printf ("arg_n_blocks=%d=%x\n", arg_n_blocks, arg_n_blocks);
				break;
			case 'q':
				quiet = 1;
				break;
			case 'j':
				just_bad_blocks = 1;
				break;
			case '?':
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
	       "Manipulate bad block markers on the specified MTD device.\n"
	       "\n"
	       "  -s, --start BLK   starting block number (not address)\n"
	       "  -l, --length LEN  length, in blocks\n"
	       "  -j, --justbad     limit operation just to blocks already found bad during\n"
	       "                      previous scan\n"
	       "  -p, --pages       show/operate on all pages, not just 1st or 1st+2nd in\n"
	       "                      erase block\n"
	       "  -e, --erase       erase OOB data (including any bad-block marker)\n"
	       "  -m, --mark        mark block bad in OOB\n"
	       "  -v, --view        show OOB data\n"
	       "  -w, --narrow      only show first few bytes of OOB for large-block devices\n" 
      /*       "  -q, --quiet       don't display progress messages\n"
	       "      --silent      same as --quiet\n" */
	       "      --help        display this help and exit\n"
	       "      --version     output version information and exit\n",
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
