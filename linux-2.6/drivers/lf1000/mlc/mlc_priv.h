/* 
 * drivers/lf1000/mlc/mlc_priv.h
 *
 * LF1000 Multi-Layer Controller Driver
 * 
 * Andrey Yurovsky <andrey@cozybit.com> */

#ifndef MLC_PRIV_H
#define MLC_PRIV_H

#include <linux/cdev.h>
#include <linux/semaphore.h>

struct mlc_layer {
	u8 id;
	struct cdev dev;
	void *fb;
	struct semaphore sem;
};

struct mlc_device {
	void *mem;				/* memory-mapped registers */
	struct cdev dev;
	struct mlc_layer layer[MLC_NUM_LAYERS];
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc;
	struct proc_dir_entry *proc_layers;
#endif
	int lcd_id;
};

#endif

