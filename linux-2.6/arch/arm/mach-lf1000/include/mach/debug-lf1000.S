/*
 * mach-lf1000/include/mach/debug-lf1000.S
 *
 *  Copyright (C) 1994-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <mach/uart.h>

	.macro  senduart,rd,rx
	strb    \rd, [\rx, #THB]		@ Write Transmitter Holding Register
	.endm

        .macro  busyuart,rd,rx
1001:   ldr	\rd, [\rx, #TRSTATUS]		@ get TRANSMIT STATUS reg
        tst     \rd, #(1<<TRANSMITTER_EMPTY)	@ = 1 when tx complete
        beq	1001b
        .endm

	.macro  waituart,rd,rx
#ifdef FLOW_CONTROL
1001:	ldr     \rd, [\rx, #FSTATUS]		@ Read Status Register
        tst     \rd, #(1<<TX_FIFO_FULL)		@ TX_FIFO_FULL = 1 when ready to transmit
        beq     1001b
#endif
        .endm
