/* mach-lf1000/include/mach/mem_controller.h -- LF1000 Memory Controller HAL 
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#ifndef _LF1000_MEM_CONTROLLER_H
#define _LF1000_MEM_CONTROLLER_H

#include <mach/platform.h>

/* static memory bus */
#define LF1000_STATIC_BASE_NOSHADOW	0x00000000 /* shadow mode disabled */
#define LF1000_STATIC_BASE_SHADOW	0x80000000 /* shadow mode */
#define LF1000_STATIC_BASE	LF1000_STATIC_BASE_NOSHADOW /* not shadow */
#define LF1000_STATIC0_BASE	0x00000000
#define LF1000_STATIC1_BASE	0x04000000
#define LF1000_STATIC2_BASE	0x08000000
#define LF1000_STATIC3_BASE	0x0C000000
#define LF1000_STATIC4_BASE	0x10000000
#define LF1000_STATIC5_BASE	0x14000000
#define LF1000_STATIC6_BASE	0x18000000
#define LF1000_STATIC7_BASE	0x1C000000
#ifdef CPU_LF1000
#define LF1000_STATIC8_BASE	0x20000000
#define LF1000_STATIC9_BASE	0x24000000
/* (reserved) */
#define LF1000_STATIC11_BASE_LOW	0x2C000000
#define LF1000_STATIC11_BASE_HIGH	0xAC000000
#endif

/* MCU-Y Memory Controller Registers (as offsets from LF1000_MCU_Y_BASE) */
#define MEMCFG		0x00
#define MEMTIME0	0x02
#define MEMTIME1	0x04
//RESERVED		0x06
#define MEMREFRESH	0x08
#define MEMCONRTROL	0x0A
#define MEMCLKDELAY	0x0C
#define MEMDQSOUTDELAY	0x0E
#define MEMDQSINDELAY	0x10

/* MEMCFG register SDRCAP */
#define	SDRCAP		0
#define SDRCAP_64MBIT	0
#define SDRCAP_128MBIT	1
#define SDRCAP_256MBIT  2
#define SDRCAP_512MBIT  3


/* MCU-S Memory Controller Registers (as offsets from LF1000_MCU_S_BASE) */
#define MEMBW		0x00
#define MEMTIMEACS	0x04
#define MEMTIMECOS	0x08
#define MEMTIMEACCL	0x0C
#define MEMTIMEACCH	0x10
#define MEMTIMESACCL	0x14
#define MEMTIMESACCH	0x18
#ifdef CPU_LF1000
#define MEMTIMESACCWL	0x1C /* (reserved) */
#define MEMTIMESACCWH	0x20 /* (reserved) */
#endif
#define MEMTIMECOH	0x24
#define MEMTIMECAH	0x28
#define MEMBURSTL	0x2C
#define MEMBURSTH	0x30
#define MEMWAIT		0x34

#endif
