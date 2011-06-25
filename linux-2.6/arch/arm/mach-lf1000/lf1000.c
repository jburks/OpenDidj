/*
 *  arch/arm/mach-lf1000/lf1000.c
 *
 *      Copyright (C) 2007 Kosta Demirev <kdemirev@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/amba/bus.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <mach/core.h>

MACHINE_START(DIDJ, "ARM-LF1000")
	.phys_io	= LF1000_SYS_IO,
	.io_pg_offst	= (IO_ADDRESS(LF1000_SYS_IO) >> 18) & 0xfffc,
	.boot_params	= CONFIG_LF1000_BOOT_PARAMS_ADDR,
	.map_io		= lf1000_map_io,
	.init_irq	= lf1000_init_irq,
	.timer		= &lf1000_timer,
	.init_machine	= lf1000_init,
MACHINE_END
