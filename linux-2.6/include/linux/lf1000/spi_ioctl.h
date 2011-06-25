/*
 * spi_ioctl.h
 * SPI supported IOCTL calls
 *
 * Scott Esters
 * LeapFrog Enterprises
 */

#ifndef SPI_IOCTL_H
#define SPI_IOCTL_H

#define SPI_IOC_MAGIC   'S'

/* supported ioctls */
#define SPI_IOCTQ_GETWORD	_IO(SPI_IOC_MAGIC,  1)	// get word

#define SPI_IOC_MAXNR		1			// last IOCTL command

#endif
