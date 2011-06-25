/*
 *  arch/arm/include/asm/serial.h
 *
 *  Copyright (C) 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Changelog:
 *   15-10-1996	RMK	Created
 */

#ifndef __ASM_SERIAL_H
#define __ASM_SERIAL_H

#ifndef	CONFIG_ARCH_LF1000
#define BASE_BAUD	(1843200 / 16)
#else
#define BASE_BAUD	LF1000_SYS_UART_BR

#define STD_COM_FLAGS	( UPF_BOOT_AUTOCONF | UPF_BUGGY_UART | UPF_SKIP_TEST | UPF_AUTO_IRQ | UART_CONFIG_TYPE)

#define SERIAL_PORT_DFNS									\
        { 0, LF1000_SYS_UART_BR, 0, LF1000_UART0_IRQ, STD_COM_FLAGS,				\
	  0, UPIO_MEM, (unsigned char *)IO_ADDRESS( LF1000_UART0_BASE), 0 },     /* ttyS0 */	\
        { 1, LF1000_SYS_UART_BR, 0, LF1000_UART1_IRQ, STD_COM_FLAGS,				\
	  0, UPIO_MEM, (unsigned char *)IO_ADDRESS( LF1000_UART1_BASE), 0 },     /* ttyS1 */	\
        { 2, LF1000_SYS_UART_BR, 0, LF1000_UART2_IRQ, STD_COM_FLAGS,				\
	  0, UPIO_MEM, (unsigned char *)IO_ADDRESS( LF1000_UART2_BASE), 0 },     /* ttyS2 */	\
        { 3, LF1000_SYS_UART_BR, 0, LF1000_UART3_IRQ, STD_COM_FLAGS,				\
	  0, UPIO_MEM, (unsigned char *)IO_ADDRESS( LF1000_UART3_BASE), 0 },     /* ttyS3 */

#endif
#endif
