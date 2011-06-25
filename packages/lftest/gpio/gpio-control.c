/* gpio-control.c
 *
 * Simple test/example code for using LF1000 GPIO driver ioctl calls, also a
 * basic user space interface for manipulating and testing IO pins.  See
 * linux/lf1000/gpio_ioctl.h for more documentation.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/lf1000/gpio_ioctl.h>

char *pin_functions[] = {"GPIO", "ALT1", "ALT2"};

int main(int argc, char **argv)
{
	int gpio;
	int req;
	int arg = 0;
	int response = 0;
	union gpio_cmd c;

	if(argc < 4) {
		printf( "usage: %s <device> <command> <arguments>\n\n"
				"commands:\n"
				"\toutvalue  <port> <pin> <value>\n"
				"\toutenable <port> <pin> <value>\n"
				"\tinvalue <port> <pin>\n"
				"\tfunc <port> <pin> <func>\n"
				"\tgetfunc <port> <pin>\n", argv[0]);
		return 1;
	}

	gpio = open(argv[1], O_RDWR);
	if(gpio < 0) {
		printf("error: failed to open %s\n", argv[1]);
		return 1;
	}

	if(!strcmp(argv[2],"outvalue")) {
		if(argc < 6) {
			printf("error: invalid number of arguments.\n");
			close(gpio);
			return 1;
		}
		c.outvalue.port 	= atoi(argv[3]);
		c.outvalue.pin 		= atoi(argv[4]);
		c.outvalue.value	= atoi(argv[5]);
		req = GPIO_IOCSOUTVAL;

		response = ioctl(gpio, req, &c);
	}
	else if(!strcmp(argv[2],"outenable")) {
		if(argc < 6) {
			printf("error: invalid number of arguments.\n");
			close(gpio);
			return 1;
		}
		c.outenb.port 	= atoi(argv[3]);
		c.outenb.pin 	= atoi(argv[4]);
		c.outenb.value	= atoi(argv[5]);
		req = GPIO_IOCSOUTENB;
		response = ioctl(gpio, req, &c);
	}
	else if(!strcmp(argv[2],"invalue")) {
		if(argc < 5) {
			printf("error: invalid number of arguments.\n");
			close(gpio);
			return 1;
		}
		c.invalue.port 	= atoi(argv[3]);
		c.invalue.pin 	= atoi(argv[4]);
		req = GPIO_IOCXINVAL;
		response = ioctl(gpio, req, &c);
		if(response >= 0) {
			response = c.invalue.value;
			if(c.invalue.value >= 0)
				printf("\nvalue=%d\n", c.invalue.value);
		}
	}
	else if(!strcmp(argv[2],"func")) {
		if(argc < 6) {
			printf("error: invalid number of arguments.\n");
			close(gpio);
			return 1;
		}
		c.func.port 	= atoi(argv[3]);
		c.func.pin 	= atoi(argv[4]);
		c.func.func	= atoi(argv[5]);
		req = GPIO_IOCSFUNC;
		response = ioctl(gpio, req, &c);
	}
	else if(!strcmp(argv[2], "getfunc")) {
		if(argc < 5) {
			printf("error: invalid number of arguments.\n");
			close(gpio);
			return 1;
		}
		c.func.port = atoi(argv[3]);
		c.func.pin  = atoi(argv[4]);
		req = GPIO_IOCXFUNC;
		response = ioctl(gpio, req, &c);
		if(response >= 0) {
			response = c.func.func;
			if(response > 2)
				printf("\nunknown func (%d)\n", response);
			else
				printf("\nfunc = %d (%s)\n", response,
						pin_functions[response]);
		}
	}
	else {
		printf("unknown command\n");
		close(gpio);
		return 1;
	}

	close(gpio);
	if(response < 0) {
		fprintf(stderr, "ioctl failed\n");
	}
	return response;
}
