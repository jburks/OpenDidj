/* LF1000 SPI Driver
 *
 * spi.h -- SPI control.
 *
 * Scott Esters
 * LeapFrog Enterprises
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#ifndef _LF1000_SPI_H
#define _LF1000_SPI_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <asm/io.h>

/* clock settings */
#define SPI_SRC_HZ      10000000
#define SPI_CLK_SRC     PLL1

#endif
