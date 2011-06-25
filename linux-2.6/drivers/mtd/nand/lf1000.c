/*
 * drivers/mtd/nand/lf1000.c
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 * Scott Esters <sesters@leapfrog.com>
 * Robert T. Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/nand.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include "../ubi/ubi-media.h"

static int lf1000_init_cart(u32 nand_base);
static int lf1000_cart_remove(void);

#if defined CONFIG_MTD_NAND_LF1000_HWECC
void lf1000_enable_hwecc(struct mtd_info *mtd, int mode);
int lf1000_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat,
				uint8_t *ecc_code);
int lf1000_correct_ecc(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc,
				uint8_t *calc_ecc);
#endif

/* control registers */
#define MCU_Y_BASE	IO_ADDRESS(LF1000_MCU_Y_BASE)
#define NAND_BASE	IO_ADDRESS(LF1000_MCU_S_BASE)

/*
 * Private device structures
 *
 *  mtd_onboard points to info about the base NAND flash
 *  mtd_cart    points to info about the cartridge NAND flash
 */

struct lf1000_nand_device {
	void __iomem	*mem;
	struct mtd_info *mtd_onboard;
	struct mtd_info *mtd_cart;
	struct platform_device *pdev;

	struct nand_hw_control controller;
	int cart_ready;
	int cart_ubi;
	struct semaphore sem_hotswap;
};

static struct lf1000_nand_device nand = {
	.mem = NULL,
	.mtd_onboard = NULL,
	.mtd_cart = NULL,
	.cart_ready = 0,
	.cart_ubi = 0
};

/*
 * Define partitions for flash devices
 */

#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

/* All other boards have big erase blocks */
/* TODO: FIXME: when MLC NAND is used, this ought to be 0x40000 */
// Reggie Changed
/* For didj the erase block size is 0x20000*/
#define LF_ERASE_BLK 0x20000


//NIRV START

/* Just shortening the names for clearer code */
#define LF_P0 (CONFIG_NAND_LF1000_P0_SIZE)
#define LF_P1 (CONFIG_NAND_LF1000_P1_SIZE)
#define LF_P2 (CONFIG_NAND_LF1000_P2_SIZE)
#define LF_P3 (CONFIG_NAND_LF1000_P3_SIZE)
#define LF_P4 (CONFIG_NAND_LF1000_P4_SIZE)
#define LF_P5 (CONFIG_NAND_LF1000_P5_SIZE)
#define LF_P6 (CONFIG_NAND_LF1000_P6_SIZE)
#define LF_P7 (CONFIG_NAND_LF1000_P7_SIZE)

#if ((LF_P0 % LF_ERASE_BLK) || (LF_P1 % LF_ERASE_BLK) || \
	(LF_P2 % LF_ERASE_BLK) || (LF_P3 % LF_ERASE_BLK) || \
	(LF_P4 % LF_ERASE_BLK) || (LF_P5 % LF_ERASE_BLK) || \
	(LF_P6 % LF_ERASE_BLK) || (LF_P7 % LF_ERASE_BLK))
#error "NAND partitions must be multiple of erase block."
#endif

// we should really just add a section for reading in either didj or explorer
// nand partition tables.
static struct mtd_partition partition_info[] = {
  	{ .name		= "LF1000_uniboot",
  	  .offset	= 0,
 	  .size		= LF_P0},
  	{ .name		= "Atomic_Boot_Flags",
 	  .offset	= LF_P0,
 	  .size		= LF_P1 },
  	{ .name		= "Manufacturing_Data",
 	  .offset	= LF_P0 + LF_P1,
 	  .size		= LF_P2},
  	{ .name		= "Kernel0",
 	  .offset	= LF_P0 + LF_P1 + LF_P2,
 	  .size		= LF_P3 },
  	{ .name		= "Linux_RFS0",
 	  .offset	= LF_P0 + LF_P1 + LF_P2 + LF_P3,
 	  .size		= LF_P4 },
  	{ .name		= "Kernel1",
 	  .offset	= LF_P0 + LF_P1 + LF_P2 + LF_P3 + LF_P4,
 	  .size		= LF_P5 },
  	{ .name		= "Linux_RFS1",
 	  .offset	= LF_P0 + LF_P1 + LF_P2 + LF_P3 + LF_P4 + LF_P5,
 	  .size		= LF_P6 },
  	{ .name		= "Brio",
 	  .offset	= LF_P0 + LF_P1 + LF_P2 + LF_P3 + LF_P4 + LF_P5 + LF_P6,
 	  .size		= LF_P7 },
};


//static struct mtd_partition partition_info_recovery[] = {
//  	{ .name		= "Base",
//  	  .offset	= 0,
// 	  .size		= MTDPART_SIZ_FULL },
//};

static struct mtd_partition partition_info_cart[] = {
	{ .name		= "Cartridge",
	  .offset	= 0,
 	  .size		= MTDPART_SIZ_FULL },
};

#undef LF_P0
#undef LF_P1
#undef LF_P2
#undef LF_P3
#undef LF_P4
#undef LF_P5
#undef LF_P6
#undef LF_P7
#undef LF_ERASE_BLK

/*******************
 * sysfs Interface *
 *******************/

// Create a new MTD device as a subset, a slice of another device
static ssize_t set_new_mtd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int old_mtd_num;
	u32 offset, size;
	char tag[64];

	struct mtd_info *mtd;
	struct nand_chip *chip;

	static struct mtd_partition new_part[1] = { { 0 } };

	if (sscanf (buf, "%d %x %x %63s", &old_mtd_num, &offset, &size, tag) != 4)
		return -EINVAL;

	/* Nice if we could convert old_mtd_num to "1=cart, 0=base" type number */
	mtd = old_mtd_num ? nand.mtd_cart : nand.mtd_onboard;
	chip = mtd->priv;

	new_part[0].offset = offset;
	new_part[0].size = size;
	tag[63] = 0;
	new_part[0].name = kmalloc(1+strlen(tag), GFP_KERNEL);
	strcpy (new_part[0].name, tag);
	if (chip) {
		spin_lock(&chip->controller->lock);
		add_mtd_partitions(mtd, new_part, 1);
		spin_unlock(&chip->controller->lock);
		dev_alert(dev, "New mtd device '%s' offset=0x%x size=0x%x created on %s.\n",
			tag, offset, size, old_mtd_num ? "cart" : "onboard");
	}
	else {
		dev_alert(dev, "No MTD device found\n");
	}

	return count;
}

static DEVICE_ATTR(new_mtd, S_IWUSR|S_IWGRP|S_IWOTH, NULL, set_new_mtd);

static ssize_t show_ramsize(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	uint size;

	size = (readw(MCU_Y_BASE + MEMCFG) >> SDRCAP) & 3;
	switch(size) {
		case SDRCAP_64MBIT:	return(sprintf(buf, "%s\n",  "8MB"));
		case SDRCAP_128MBIT:	return(sprintf(buf, "%s\n", "16MB"));
		case SDRCAP_256MBIT:	return(sprintf(buf, "%s\n", "32MB"));
		case SDRCAP_512MBIT:	return(sprintf(buf, "%s\n", "64MB"));
		default:		return sprintf(buf, "unknown\n");
	}
}
static DEVICE_ATTR(ramsize, S_IRUSR|S_IRGRP|S_IROTH, show_ramsize, NULL);

#if defined CONFIG_MTD_NAND_LF1000_DEBUG
static ssize_t show_nand_timing(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	u32 tmp;

	tmp = readl(NAND_BASE+MEMTIMEACS);
	len += sprintf(buf+len, "0: ACS = %d\n", ((tmp>>22) & 0x3));
	tmp = readl(NAND_BASE+MEMTIMECOS);
	len += sprintf(buf+len, "1: COS = %d\n", ((tmp>>22) & 0x3));
	tmp = readl(NAND_BASE+MEMTIMEACCH);
	len += sprintf(buf+len, "2: ACC = %d\n", ((tmp>>12) & 0xF));
	tmp = readl(NAND_BASE+MEMTIMECOH);
	len += sprintf(buf+len, "3: COH = %d\n", ((tmp>>21) & 0x3));
	tmp = readl(NAND_BASE+MEMTIMECAH);
	len += sprintf(buf+len, "4: CAH = %d\n", ((tmp>>22) & 0x3));

	return len;
}

static ssize_t set_nand_timing(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int index, value;
	u32 tmp;

	if(sscanf(buf, "%u,%u", &index, &value) != 2)
		return -EINVAL;

	switch(index) {
		case 0: /* ACS */
		tmp = readl(NAND_BASE+MEMTIMEACS) & ~(0x3<<22);
		tmp |= ((0x3 & value)<<22);
		writel(tmp, NAND_BASE+MEMTIMEACS);
		break;
		case 1: /* COS */
		tmp =  readl(NAND_BASE+MEMTIMECOS) & ~(0x3<<22);
		tmp |= ((0x3 & value)<<22);
		writel(tmp, NAND_BASE+MEMTIMECOS);
		break;
		case 2: /* ACC */
		tmp = readl(NAND_BASE+MEMTIMEACCH) & ~(0xF<<12);
		tmp |= ((0xF & value)<<12);
		writel(tmp, NAND_BASE+MEMTIMEACCH);
		break;
		case 3: /* COH */
		tmp = readl(NAND_BASE+MEMTIMECOH) & ~(0x3<<21);
		tmp |= ((0x3 & value)<<21);
		writel(tmp, NAND_BASE+MEMTIMECOH);
		break;
		case 4: /* CAH */
		tmp = readl(NAND_BASE+MEMTIMECAH) & ~(0x3<<22);
		tmp |= ((0x3 & value)<<22);
		writel(tmp, NAND_BASE+MEMTIMECAH);
		break;
		default:
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(timing, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		show_nand_timing, set_nand_timing);
#endif


#ifdef CONFIG_MTD_NAND_LF1000_PROF
#include "prof.h"
static unsigned long ws_n[NS_MAX], ws_sum[NS_MAX], ws_min[NS_MAX], ws_max[NS_MAX];
static long ws_start[NS_MAX];
static int ws_any = 0;

/* From the timer module; 15 minutes worth of ticks at 4.593MHz fits in 32 bits */
extern int read_current_timer(unsigned long *timer_value);

void nand_stats_erase (void)
{
	int i;
	for (i=0; i<NS_MAX; i++)
		ws_n[i] = ws_sum[i] = 0;
}

/* Accumulate a start (in=1) or stop (in=0) time for a given type of access */
void nand_stats_accum (enum prof_type type, int in)
{
	long stop, delta;
	if (type >= NS_MAX)
	{
		dev_alert(&nand.pdev->dev,
				"nand_stats_accum: type=%d > NS_MAX", type);
		return;
	}
	if (!ws_any)
	{
		/* First time through, erase stats  */
		nand_stats_erase ();
		ws_any = 1;
	}
	read_current_timer ((unsigned long *)&stop);
	if (in)
	{
		ws_start[type] = stop;
	}
	else
	{
		delta = stop - ws_start[type];
		ws_sum[type] += delta;
		if (!ws_n[type])
		{
			/* First data point, set min and max */
			ws_min[type] = ws_max[type] = delta;
		}
		else
		{
			if (ws_min[type] > delta)
				ws_min[type] = delta;
			if (ws_max[type] < delta)
				ws_max[type] = delta;
		}
		ws_n[type]++;
	}
}

static ssize_t show_write_stats(struct device *dev, struct device_attribute *attr, char *buf)
{
	int x=0, i;
	static char *title[] = {"Read ", "Write", "Erase", "Lock "};
	for (i=0; i<NS_MAX; i++)
	{
		if (ws_n[i])
		{
			x += sprintf (buf+x, "%s N=%ld %ld/%ld/%ld\n",
				      title[i], ws_n[i], ws_min[i], ws_sum[i]/ws_n[i], ws_max[i]);
		}
		else
		{
			x += sprintf (buf+x, "%s N=%ld %ld/%ld/%ld\n", title[i], 0L,0L,0L,0L);
		}
	}
	return x;
}

static ssize_t clear_write_stats(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	nand_stats_erase ();
	return count;
}

/* Write to this sysfs device: clear stats.  Read: dump out stats.  See profnand.c */
static DEVICE_ATTR(write, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH,
		   show_write_stats, clear_write_stats);

#endif
/* End #ifdef CONFIG_MTD_NAND_LF1000_PROF */

#ifdef CONFIG_MTD_NAND_LF1000_READ_DELAY
volatile int read_delay = 0;

static ssize_t show_read_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf (buf, "%d\n", read_delay);
}

static ssize_t set_read_delay(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(sscanf(buf, "%d", &read_delay) != 1)
		return -EINVAL;
	return count;
}

static DEVICE_ATTR(read_delay, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, show_read_delay, set_read_delay);
#endif /* ifdef CONFIG_MTD_NAND_LF1000_READ_DELAY */

#ifdef CONFIG_MTD_NAND_LF1000_STRESS_TEST
volatile int nand_stress = 0;

static ssize_t show_nand_stress(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf (buf, "%d\n", nand_stress);
}

static ssize_t set_nand_stress(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, value;

	ret = get_option(&buf, &value);

	if(ret != 1)
		return -EINVAL;

	nand_stress = value;

	/*
	 * Restore any severed connections
	 */
	if(IS_CLR(nand_stress, CART_READ))
	{
		stress_cut_cart(0);
	}

	return count;
}

static DEVICE_ATTR(stress, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, show_nand_stress, set_nand_stress);

#endif /* ifdef CONFIG_MTD_NAND_LF1000_STRESS_TEST */

static ssize_t show_cart_ecc_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	int cart_ecc_mode = -1;

	if (nand.mtd_cart)
	{
		struct nand_chip *cart;
		cart = (struct nand_chip *) (&nand.mtd_cart[1]);
		if (cart) {
			switch (cart->ecc.mode)
			{
			case NAND_ECC_NONE: cart_ecc_mode = 0; break;
			case NAND_ECC_SOFT: cart_ecc_mode = 1; break;
			case NAND_ECC_HW_SYNDROME: cart_ecc_mode = 4; break;
			default: cart_ecc_mode = -1; break;
			}
		}
		else {
			dev_alert(dev, "Funky cartridge device\n");
		}
	}
	else {
		dev_alert(dev, "No cartridge device\n");
	}
	return sprintf (buf, "%d\n", cart_ecc_mode);
}

static ssize_t set_cart_ecc_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int cart_ecc_mode;
	int ecc_mode = -1;
	if(sscanf(buf, "%d", &cart_ecc_mode) != 1)
		return -EINVAL;
	if (nand.mtd_cart) {
		struct mtd_info *mtd = nand.mtd_cart;
		struct nand_chip *cart;
		cart = (struct nand_chip *) (&nand.mtd_cart[1]);
		if (cart) {
			switch (cart_ecc_mode)
			{
			case 4: ecc_mode = NAND_ECC_HW_SYNDROME; break;
			case 1: ecc_mode = NAND_ECC_SOFT; break;
			case 0: ecc_mode = NAND_ECC_NONE; break;
			default:
				dev_alert(dev, "Bad ecc_mode %d\n",
						cart_ecc_mode);
				break;
			}
			if (ecc_mode != -1)
			{
				// Delete cart parts and reinstate them
				struct nand_chip *chip = mtd->priv;
				int cart_parts_nb = 0;
				struct mtd_partition *cart_parts = 0;
				extern int nand_switch_ecc_mode(struct mtd_info *mtd, int ecc_mode);


				cart_parts = partition_info_cart;
				cart_parts_nb = ARRAY_SIZE(partition_info_cart);
				spin_lock(&chip->controller->lock);
				del_mtd_partitions(mtd);
				nand_switch_ecc_mode(mtd, ecc_mode);
				chip->scan_bbt(mtd);
				add_mtd_partitions(nand.mtd_cart, cart_parts, cart_parts_nb);
				spin_unlock(&chip->controller->lock);
			}
		}
		else {
			dev_alert(dev, "Funky cartridge device\n");
		}
	}
	else {
		dev_alert(dev, "No cartridge device\n");
	}

	return count;
}

static DEVICE_ATTR(cart_ecc_mode, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, show_cart_ecc_mode, set_cart_ecc_mode);

/******************************************************************************/

/* Protect NOR from accidental erasure:
 * Create an address threshold; addresses LOWER than this address are
 *   blocked from erase or write commands to the nor (will return -EPERM).
 * By default, set the threshold up high, to protect everything in NOR
 */
u32 nor_write_addr_threshold = 0x7fffffff;

static ssize_t show_nor_write_addr_threshold(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf (buf, "0x%08x\n", nor_write_addr_threshold);
}

static ssize_t set_nor_write_addr_threshold(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(sscanf(buf, "0x%x", &nor_write_addr_threshold) != 1)
		if(sscanf(buf, "%d", &nor_write_addr_threshold) != 1)
			return -EINVAL;
	return count;
}

static DEVICE_ATTR(nor_write_addr_threshold, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, show_nor_write_addr_threshold, set_nor_write_addr_threshold);

/******************************************************************************/

#ifdef CONFIG_MTD_NAND_LF1000_HOTSWAP
static ssize_t get_cart_hotswap_state(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ready = 0;

	/* frist make sure mtd_cart is allocated */
	if(nand.mtd_cart != NULL) {
		ready = nand.cart_ready;
	}

	return sprintf (buf, "%d\t%d\n", ready, nand.cart_ubi);
}

static ssize_t set_cart_hotswap_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct resource *res;
	int cart_parts_nb = 0;
	int hotswap_state;
	struct platform_device *pdev__ ;
	struct mtd_partition *cart_parts = 0;
	size_t nread;
	uint32_t magic;
	ssize_t ret = count;

	if(sscanf(buf, "%d", &hotswap_state) != 1)
		return -EINVAL;

	pdev__= to_platform_device(dev);
	res = platform_get_resource(pdev__, IORESOURCE_MEM, 0);
	if(!res) {
		dev_err(dev, "nand: failed to get resource!\n");
		return -ENXIO;
	}

	if (down_interruptible(&nand.sem_hotswap))
		return -ERESTARTSYS;

	/* cart is inserted */
	if(hotswap_state) {

		/* check if a cartridge is inserted */
		gpio_configure_pin(NAND_CART_DETECT_PORT, NAND_CART_DETECT_PIN,
			GPIO_GPIOFN, 0, 1, 0);
		if(gpio_get_val(NAND_CART_DETECT_PORT, NAND_CART_DETECT_PIN) != NAND_CART_DETECT_LEVEL) {
			dev_err(dev, "cartridge insertion can't be confirmed by driver\n");
			ret = -EAGAIN;
			goto out;
		} else {
			int i = 0;
			int scan;

			dev_info(dev, "cartridge inserted\n");

			if(nand.cart_ready == 1){
				dev_err(dev, "cartridge driver was ready\n");
				goto out;
			}

			if(lf1000_init_cart(res->start)) {
				nand.cart_ready = -1;
				if(nand.mtd_cart)
					kfree(nand.mtd_cart);
				ret = -EPERM;
				goto out;
			}

			do {
				scan = nand_scan(nand.mtd_cart, 1);
			} while (scan && ++i < 4);

			if (i > 1)
				dev_info(dev, "tried to scan cartridge %d times\n", i);

			if (scan) {
				nand.cart_ready = -1;
				dev_err(dev, "cartridge inserted, but NAND not detected !\n");
				nand_release(nand.mtd_cart);
				kfree(nand.mtd_cart);
				nand.mtd_cart = NULL;
				ret = -EPERM;
				goto out;
			}

#ifdef CONFIG_MTD_PARTITIONS
			nand.mtd_cart->name = "lf1000-cart";
			cart_parts_nb = parse_mtd_partitions(nand.mtd_cart,
							     part_probes,
							     &cart_parts, 0);
#endif
			if (cart_parts_nb == 0) {
				cart_parts = partition_info_cart;
				cart_parts_nb = ARRAY_SIZE(partition_info_cart);
			}

			/* Register the cartridge partitions, if it exists */
			add_mtd_partitions(nand.mtd_cart, cart_parts, cart_parts_nb);

			nand.mtd_cart->read(nand.mtd_cart, 0, sizeof(uint32_t), &nread, (void *)&magic);
			magic = be32_to_cpu(magic);
			if (magic == UBI_EC_HDR_MAGIC) {
				nand.cart_ubi=1;
				dev_info(dev, "cartridge has UBI layer, nread=%d\n", nread);
			} else {
				nand.cart_ubi=0;
				dev_info(dev, "cartridge has no UBI, nread=%d\n", nread);
			}

			// Error checking
			if((gpio_get_val(NAND_CART_TYPE_PORT, NAND_CART_TYPE_EMERALD) == 0) && (nand.cart_ubi==1)){
				dev_err(dev, "cart has UBI layer, but is usig Emerald Cart ID\n");
			} else if ((gpio_get_val(NAND_CART_TYPE_PORT, NAND_CART_TYPE_EMERALD) == 1) && (nand.cart_ubi==0)){
				dev_err(dev, "cart ID is Didj, but no UBI layer found !!\n");
			}

			nand.cart_ready = 1;
			dev_info(dev, "cart driver ready !\n");
		}
	} else {  /* cart is removed */
		nand.cart_ready = 0;

		lf1000_cart_remove();

		dev_info(dev, "cartridge removed !\n");
	}

out:
	up(&nand.sem_hotswap);

	return ret;
}
static DEVICE_ATTR(cart_hotswap, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, get_cart_hotswap_state, set_cart_hotswap_state);
#endif

/******************************************************************************/

static struct attribute *nand_attributes[] = {
	&dev_attr_new_mtd.attr,
	&dev_attr_ramsize.attr,
#if defined CONFIG_MTD_NAND_LF1000_DEBUG
	&dev_attr_timing.attr,
#endif
#ifdef CONFIG_MTD_NAND_LF1000_PROF
	&dev_attr_write.attr,
#endif
#ifdef CONFIG_MTD_NAND_LF1000_READ_DELAY
	&dev_attr_read_delay.attr,
#endif
#ifdef CONFIG_MTD_NAND_LF1000_STRESS_TEST
	&dev_attr_stress.attr,
#endif
	&dev_attr_cart_ecc_mode.attr,
	&dev_attr_nor_write_addr_threshold.attr,

#ifdef CONFIG_MTD_NAND_LF1000_HOTSWAP
	&dev_attr_cart_hotswap.attr,
#endif
	NULL
};

static struct attribute_group nand_attr_group = {
	.attrs = nand_attributes
};

/*
 * There are 3 ways to test ready/busy bit:
 * 1) test the RnB bit in NFCONTROL (used here)
 * 2) test the IRQPEND bit in NFCONTROL and then set it to clear the interrupt
 * 3) send a NAND_CMD_STATUS to then NAND chip, test the response against
 *    the mask 0x40
 */
static int lf1000_nand_ready(struct mtd_info *mtd)
{
	u32 ctl = readl(NAND_BASE+NFCONTROL);

	if(IS_SET(ctl,RnB))
		return 1;	/* ready */
	return 0;		/* busy */
}

/*
 * hardware-specific access to control and address lines:
 * The LF1000's NAND controller handles the CLE and ALE signals automatically,
 * data must simply be written to the appropriate register: NFCMD or NFADDR
 * respectively.
 */
static void lf1000_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;

	if(cmd == NAND_CMD_NONE)
		return;

	if(ctrl & NAND_CLE) /* command */ {
		writeb(cmd, chip->IO_ADDR_W + NFCMD);
	}
	else if(ctrl & NAND_ALE) /* address */ {
		writeb(cmd, chip->IO_ADDR_W + NFADDR);
	}
}

static void lf1000_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;
	u32 tmp;

	switch(chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:
		tmp = readl(NAND_BASE+NFCONTROL);
		BIT_CLR(tmp, NFBANK);
		writel(tmp, NAND_BASE+NFCONTROL);
		break;
	default:
		BUG();
	}
}

void lf1000_select_cart(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;
	u32 tmp;

	switch(chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:
		tmp = readl(NAND_BASE+NFCONTROL);
		BIT_SET(tmp, NFBANK);
		writel(tmp, NAND_BASE+NFCONTROL);
		break;
	default:
		BUG();
	}
}

/* Our OTP parts are made by NAND_MFR_TOSHIBA */
#define	OTP_DEVID_128MBIT	0x73	/* 16MByte */
#define	OTP_DEVID_256MBIT	0x75	/* 32MByte */
#define	OTP_DEVID_512MBIT	0x76	/* 64MByte */
#define	OTP_DEVID_1GBIT		0x79	/* 128MByte */

static int lf1000_init_cart(u32 nand_base)
{
	struct nand_chip *cart;
	const char *s;

	nand.mtd_cart = kmalloc(sizeof(struct mtd_info) +
			sizeof(struct nand_chip), GFP_KERNEL);
	if(!nand.mtd_cart) {
		dev_err(&nand.pdev->dev, "Unable to allocate cart device\n");
		return -ENOMEM;
	}

	nand.mtd_cart->owner = THIS_MODULE;

	/* Get pointer to private data */
	cart = (struct nand_chip *) (&nand.mtd_cart[1]);

	memset(nand.mtd_cart, 0, sizeof(struct mtd_info)+
			sizeof(struct nand_chip));

	cart->controller = &nand.controller;

	/* Link the private data with the MTD structure */
	nand.mtd_cart->priv = cart;

	/* Set address of NAND IO lines */
	cart->IO_ADDR_R = (void __iomem *)(IO_ADDRESS(nand_base+NFDATA));
	cart->IO_ADDR_W = (void __iomem *)(IO_ADDRESS(nand_base+NFDATA));

	cart->dev_ready = lf1000_nand_ready;
	cart->options = 0; /* 8 bit bus width */
	cart->cmd_ctrl = lf1000_hwcontrol; /* hardware access for cmd, addr */
	cart->select_chip = lf1000_select_cart;

	/* 25 us command delay time */
	cart->chip_delay = 25;

	gpio_configure_pin(NAND_CART_TYPE_PORT, NAND_CART_TYPE_EMERALD,
			GPIO_GPIOFN, 0, 1, 0);
	gpio_configure_pin(NAND_CART_TYPE_PORT, NAND_CART_TYPE_HIGH,
			GPIO_GPIOFN, 0, 1, 0);
	if(gpio_get_val(NAND_CART_TYPE_PORT, NAND_CART_TYPE_HIGH) == 0) {
		dev_info(&nand.pdev->dev, "cartridge type: OTP\n");
		cart->ecc.mode = NAND_ECC_NONE; /* NONE;  */
	} else {
		dev_info(&nand.pdev->dev, "cartridge type: NAND\n");
		cart->ecc.mode = NAND_ECC_SOFT;
	}

	/* report the ECC mode that we wound up with for the cartridge */
	switch(cart->ecc.mode) {
	case NAND_ECC_NONE:		s="none"; break;
	case NAND_ECC_SOFT:		s="software"; break;
	case NAND_ECC_HW_SYNDROME:	s="hardware syndrome"; break;
	default:			s="unknown"; break;
	}
	dev_info(&nand.pdev->dev, "cartridge ECC mode: %s\n", s);

	return 0;
}

static int lf1000_cart_remove(void)
{
	/* Release resources, unregister device */
	if(nand.mtd_cart)
		nand_release(nand.mtd_cart);

	if(nand.mtd_cart)
		kfree(nand.mtd_cart);

	nand.mtd_cart = NULL;

	return 0;
}

/*
 * Platform Device
 */

static int lf1000_nand_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	struct nand_chip *this;
	const char *part_type = NULL;
	int base_parts_nb = 0;
	int cart_parts_nb = 0;
	struct mtd_partition *base_parts = NULL;
	struct mtd_partition *cart_parts = NULL;

	nand.pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		dev_err(&pdev->dev, "failed to get resource!\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1,
				"lf1000-nand")) {
		dev_err(&pdev->dev, "failed to map memory region.");
		return -EBUSY;
	}

	nand.mem = ioremap(res->start, (res->end - res->start)+1);
	if(nand.mem == NULL) {
		dev_err(&pdev->dev, "failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	/* initialize the controller */
	spin_lock_init(&nand.controller.lock);
	init_waitqueue_head(&nand.controller.wq);

	/* Allocate memory for MTD device structure and private data */
	nand.mtd_onboard = kmalloc(sizeof(struct mtd_info) +
			sizeof(struct nand_chip), GFP_KERNEL);
	if(!nand.mtd_onboard) {
		dev_err(&pdev->dev, "Unable to allocate MTD device\n");
		ret = -ENOMEM;
		goto fail_mem_onboard;
	}

	nand.mtd_onboard->owner = THIS_MODULE;

	/* Get pointer to private data */
	this = (struct nand_chip *)(&nand.mtd_onboard[1]);

	/* Initialize structures */
	memset(nand.mtd_onboard, 0, sizeof(struct mtd_info)+
			sizeof(struct nand_chip));

	this->controller = &nand.controller;

	/* Link the private data with the MTD structure */
	nand.mtd_onboard->priv = this;

	/* Set address of NAND IO lines */
	this->IO_ADDR_R = (void __iomem *)(IO_ADDRESS(res->start+NFDATA));
	this->IO_ADDR_W = (void __iomem *)(IO_ADDRESS(res->start+NFDATA));

	this->dev_ready = lf1000_nand_ready;
	this->options = 0; /* 8 bit bus width */
	this->cmd_ctrl = lf1000_hwcontrol; /* hardware access for cmd, addr */
	this->select_chip = lf1000_select_chip;

	/* 25 us command delay time */
	this->chip_delay = 25;

	this->ecc.mode = NAND_ECC_SOFT;

	/* find the onboard NAND Flash (must exist) */
	if(nand_scan(nand.mtd_onboard, 1)) {
		ret = -ENXIO;
		goto fail_find_onboard;
	}

	/* check if a cartridge is inserted */
	gpio_configure_pin(NAND_CART_DETECT_PORT, NAND_CART_DETECT_PIN,
			GPIO_GPIOFN, 0, 1, 0);

	init_MUTEX(&nand.sem_hotswap);

	/* Add the base partitions */
#ifdef CONFIG_MTD_PARTITIONS
	nand.mtd_onboard->name = "lf1000-base";
	base_parts_nb = parse_mtd_partitions(nand.mtd_onboard,
			part_probes, &base_parts, 0);
	if (base_parts_nb > 0)
		part_type = "command line";
	else
		base_parts_nb = 0;

	if(nand.mtd_cart != NULL) {
		nand.mtd_cart->name = "lf1000-cart";
		cart_parts_nb = parse_mtd_partitions(nand.mtd_cart,
						     part_probes,
						     &cart_parts, 0);
		if (cart_parts_nb > 0)
			part_type = "command line";
		else
			cart_parts_nb = 0;
	}
#endif
	if (base_parts_nb == 0) {
		if (gpio_get_boot_source_config() == SCRATCH_BOOT_SOURCE_USB)
		{
//			base_parts = partition_info_recovery;
//			base_parts_nb = ARRAY_SIZE(partition_info_recovery);
			base_parts = partition_info;
			base_parts_nb = ARRAY_SIZE(partition_info);
		}
		else
		{
			base_parts = partition_info;
			base_parts_nb = ARRAY_SIZE(partition_info);
		}
		part_type = "static";
	}

	if (cart_parts_nb == 0) {
		cart_parts = partition_info_cart;
		cart_parts_nb = ARRAY_SIZE(partition_info_cart);
		part_type = "static";
	}

	/* Register the onboard partitions */
	add_mtd_partitions(nand.mtd_onboard, base_parts, base_parts_nb);

	/* Register the cartridge partitions, if it exists */
	if (nand.mtd_cart != NULL) {
		nand.cart_ready = 1;
		add_mtd_partitions(nand.mtd_cart, cart_parts, cart_parts_nb);
	}

	/* enable NAND_WP pin as an output, enable write & erase */
	gpio_configure_pin(NAND_WP_PORT, NAND_WP_PIN, GPIO_GPIOFN, 1, 0, 1);

	sysfs_create_group(&pdev->dev.kobj, &nand_attr_group);
	return 0;

fail_find_onboard:
	kfree(nand.mtd_onboard);
	if(nand.mtd_cart)
		kfree(nand.mtd_cart);
fail_mem_onboard:
	iounmap(nand.mem);
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);
	return ret;
}

static int lf1000_nand_remove(struct platform_device *pdev)
{
	struct resource *res  = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	sysfs_remove_group(&pdev->dev.kobj, &nand_attr_group);

	/* Release resources, unregister device */
	nand_release(nand.mtd_onboard);
	if(nand.mtd_cart)
		nand_release(nand.mtd_cart);

	/* Free the MTD device structure */
	kfree(nand.mtd_onboard);
	if(nand.mtd_cart)
		kfree(nand.mtd_cart);

	if(nand.mem != NULL)
		iounmap(nand.mem);

	release_mem_region(res->start, (res->end - res->start) + 1);
	return 0;
}

static struct platform_driver lf1000_nand_driver = {
	.probe		= lf1000_nand_probe,
	.remove		= lf1000_nand_remove,
	.driver		= {
		.name	= "lf1000-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init lf1000_init(void)
{
	return platform_driver_register(&lf1000_nand_driver);
}
module_init(lf1000_init);

static void __exit lf1000_cleanup(void)
{
	platform_driver_unregister(&lf1000_nand_driver);
}
module_exit(lf1000_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Yurovsky <ayurovsky@leapfrog.com>");
MODULE_DESCRIPTION("NAND Flash Controller for the LF1000");
