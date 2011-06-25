/*
 * drives/mtd/ubi/stress.c
 *
 * Support routines for ubi stress testing under LF1000 platform.
 *
 * Copyright 2009 LeapFrog Enterprises, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include "ubi.h"
#include "stress.h"

/*
 * mtd_part copied from mtdpart.c
 * Is it the same as mtd_partition?
 */

/* Our partition node structure */
struct mtd_part {
        struct mtd_info mtd;
        struct mtd_info *master;
        uint64_t offset;
        int index;
        struct list_head list;
        int registered;
};

/*
 * Given a pointer to the MTD object in the mtd_part structure, we can retrieve
 * the pointer to that structure with this macro.
 */
#define PART(x)  ((struct mtd_part *)(x))

/*
 * Print block numbers of PEBs (raw physical erase blocks) in list 'which'
 * 'which' should be, e.g., &si->erase for the dirty block list.
 */
void print_pebs(struct ubi_device *ubi, struct ubi_scan_info *si, struct list_head *which)
{
	struct ubi_scan_leb *seb = NULL, *tmp = NULL;
	struct mtd_part *part = PART(ubi->mtd);
	uint64_t page_addr, block_addr, div;

	list_for_each_entry_safe ( seb, tmp, which,  u.list)
	{
		/* Note: the physical blocks here correspond to an
 		 * mtd partition (mtd_part), which do not match the
 		 * actual device blocks erased in nand_erase_nand().
 		 * Align them with mtd_part->offset.
 		 */
		page_addr = seb->pnum * ubi->peb_size;
		page_addr += part->offset;

		/* block_addr = page_addr / ubi->peb_size;
		 * fails because: drivers/built-in.o:
		 * 	undefined reference to `__aeabi_uldivmod'
		 * So, loop-shift to implement divide.
		 */
		div = ubi->peb_size;
		block_addr = page_addr;
		while( div >>=1 )
		{
			block_addr >>= 1;
		}

		printk ("Block %lld, EC = %d\n" , block_addr, seb->ec );
	}
	printk( "\n" );
}
