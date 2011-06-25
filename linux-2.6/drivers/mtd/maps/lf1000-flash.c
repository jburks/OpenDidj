/*
 * Map driver for LeapFrog LF1000 platform.
 * Based on pxa2xx-flash.c driver
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>

#include <asm/mach/flash.h>

static void lf1000_map_inval_cache(struct map_info *map, unsigned long from,
				      ssize_t len)
{
	flush_ioremap_region(map->phys, map->cached, from, len);
}

struct lf1000_flash_info {
	struct mtd_partition	*parts;
	struct mtd_info		*mtd;
	unsigned int		nr_parts;
	struct map_info		map;
};


static const char *probes[] = { "cmdlinepart", NULL };

int lf1000_mtd_probe_flash(struct lf1000_flash_info* info,
			   struct flash_platform_data* flash)
{
        info->map.virt = ioremap(info->map.phys, info->map.size);
        if (!info->map.virt) {
                printk(KERN_WARNING "Failed to ioremap %s\n",
                       info->map.name);
                return -ENOMEM;
        }
        info->map.cached =
                ioremap_cached(info->map.phys, info->map.size);
        if (!info->map.cached)
                printk(KERN_WARNING "Failed to ioremap cached %s\n",
                       info->map.name);
        info->map.inval_cache = lf1000_map_inval_cache;
        simple_map_init(&info->map);

        printk(KERN_NOTICE
               "Probing %s at physical address 0x%08x"
               " (%d-bit bankwidth)\n",
               info->map.name, info->map.phys,
               info->map.bankwidth * 8);

        info->mtd = do_map_probe(flash->map_name, &info->map);
	return(0);
}


static int __init lf1000_flash_probe(struct platform_device *pdev)
{
	struct flash_platform_data *flash = pdev->dev.platform_data;
	struct lf1000_flash_info *info;
	struct mtd_partition *parts;
	struct resource *res;
	int ret = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	info = kmalloc(sizeof(struct lf1000_flash_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, sizeof(struct lf1000_flash_info));
	info->map.name = (char *) flash->name;
	info->map.bankwidth = flash->width;
	info->map.phys = res->start;
	info->map.size = res->end - res->start + 1;
	info->parts = flash->parts;
	info->nr_parts = flash->nr_parts;

	ret = lf1000_mtd_probe_flash(info, flash);
	printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
	if (ret < 0) {	// error mapping flash
		return(ret);
	}

	/*
 	 * If NOR not found, maybe try a second HIGH address.
 	 * We can't tell if system booted from internal NAND
 	 * or external ATAP cartridge.
 	 */
	if (!info->mtd) {
		printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
		if (info->map.virt)
			iounmap((void *)info->map.virt);
		if (info->map.cached) {
			printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
			iounmap(info->map.cached);
		}
		/* NOR not found, maybe try a second HIGH address */
		if (info->map.phys != LF1000_NOR_FLASH_BASE_HIGH0)
			return -ENXIO;

		printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
		/* Look for NOR at second address */
		res->start = LF1000_NOR_FLASH_BASE_HIGH1;
		res->end   = res->start + LF1000_NOR_FLASH_SIZE - 1;
		info->map.phys = res->start;
		ret = lf1000_mtd_probe_flash(info, flash);

		if (ret < 0) {	// error mapping flash
			printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
			return(ret);
		}

		if (!info->mtd) {  /* NOR not found at second address */
			printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
			if (info->map.virt)
				iounmap((void *)info->map.virt);
			if(info->map.cached)
				iounmap(info->map.cached);
				release_resource(res);
			printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
			return -ENXIO;
		}
		printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);
	}
	info->mtd->owner = THIS_MODULE;

#ifdef CONFIG_MTD_PARTITIONS
	ret = parse_mtd_partitions(info->mtd, probes, &parts, 0);

	printk(KERN_INFO "%s().%d\n", __FUNCTION__, __LINE__);

	if (ret > 0) {
		info->nr_parts = ret;
		info->parts = parts;
	}
#endif

	if (info->nr_parts) {
		add_mtd_partitions(info->mtd, info->parts,
				   info->nr_parts);
	} else {
		printk("Registering %s as whole device\n",
		       info->map.name);
		add_mtd_device(info->mtd);
	}

	platform_set_drvdata(pdev, info);
	return 0;
}

static int __exit lf1000_flash_remove(struct platform_device *pdev)
{
	struct lf1000_flash_info *info = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_MTD_PARTITIONS
	if (info->nr_parts)
		del_mtd_partitions(info->mtd);
	else
#endif
		del_mtd_device(info->mtd);

	map_destroy(info->mtd);
	iounmap(info->map.virt);
	if (info->map.cached)
		iounmap(info->map.cached);
	kfree(info->parts);
	kfree(info);
	return 0;
}

#ifdef CONFIG_PM
static int lf1000_flash_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct lf1000_flash_info *info = platform_get_drvdata(pdev);
	int ret = 0;

	if (info)
		ret = info->mtd->suspend(info->mtd);

	return ret;
}

static int lf1000_flash_resume(struct platform_device *pdev)
{
	struct lf1000_flash_info *info = platform_get_drvdata(pdev);

	if (info->mtd && info->mtd->resume)
		info->mtd->resume(info->mtd);
	return 0;
}
static void lf1000_flash_shutdown(struct platform_device *pdev)
{
	struct lf1000_flash_info *info = platform_get_drvdata(pdev);

	if (info && info->mtd->suspend(info->mtd) == 0)
		info->mtd->resume(info->mtd);
}
#else
#define lf1000_flash_suspend NULL
#define lf1000_flash_resume NULL
#define lf1000_flash_shutdown NULL
#endif

static struct platform_driver lf1000_flash_driver = {
	.probe		= lf1000_flash_probe,
	.remove		= __exit_p(lf1000_flash_remove),
	.suspend	= lf1000_flash_suspend,
	.resume		= lf1000_flash_resume,
	.shutdown	= lf1000_flash_shutdown,
	
	.driver 	= {
		.name	= "lf1000-flash",
		.owner	= THIS_MODULE,
	},
};

static int __init init_lf1000_flash(void)
{
	return platform_driver_register(&lf1000_flash_driver);
}

static void __exit cleanup_lf1000_flash(void)
{
	platform_driver_unregister(&lf1000_flash_driver);
}

module_init(init_lf1000_flash);
module_exit(cleanup_lf1000_flash);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Scott Esters <sesters@leapfrog.com>");
MODULE_DESCRIPTION("MTD map driver for LeapFrog LF1000");
