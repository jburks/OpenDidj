/*
 * spi_hal.h
 * Hardware specific defines for spi driver
 *
 * Scott Esters
 * LeapFrog Enterprises
 */

#ifndef SPI_HAL_H
#define SPI_HAL_H

/* SPI registers as offsets */
#define SSPSPICONT0	0x00
#define SSPSPICONT1	0x02
#define SSPSPIDATA	0x04
#define SSPSPISTATE	0x06
#define SSPSPICLKENB	0x40
#define SSPSPICLKGEN	0x44

/******************
 * CONT0 Register *
 *****************/
#define ZENB		   14
#define RXONLY		   13
#define DMAENB		   12
#define ENB		   11
#define FFCLR		   10
#define	EXTCLKSEL	    9
#define NUMBIT		    5
#define NUMBIT_MASK	0x1E0
#define DIVCNT		    0
#define DIVCNT_MASK      0x1F


/******************
 * CONT1 Register *
 *****************/
#define SLAV_SEL	    4
#define SCLKPOL		    3
#define SCLKSH		    2
#define TYPE		    0
#define TYPE_MASK	  0x3

/******************
 * STATE Register *
 *****************/
#define IRQEENB		   15
#define IRQWENB		   14
#define IRQRENB		   13
#define IRQE		    6
#define IRQW		    5
#define IRQR		    4
#define WFFFULL		    3
#define WFFEMPTY	    2
#define RFFFULL		    1
#define RFFEMPTY	    0

/*******************
 * CLKENB Register *
 ******************/
#define SPI_PCLKMODE	    3
#define SPI_CLKGENENB	    2

/*******************
 * CLKGEN Register *
 ******************/
#define SPI_CLKDIV	     4
#define SPI_CLKDIV_MASK	 0x3F0
#define SPI_CLKSRCSEL	     1
#define SPI_CLKSRCSEL_MASK 0xE

#endif
