/* switches-example.c
 *
 * Simple user-space example for reading from /dev/input/event0 to test the
 * switches found on the ME_LF1000 development board.  See evtest.c for more
 * input events programming examples.  
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_DEVNODES	8

int open_input_device(char *input_name)
{
	char dev[20];
	char name[32];
	int fd, i;

	for(i = 0; i < MAX_DEVNODES; i++) {
		sprintf(dev, "/dev/input/event%d", i);
		fd = open(dev, O_RDONLY);
		if(fd < 0) {
			printf("can't open %s\n", dev);
			return 1;
		}

		if(ioctl(fd, EVIOCGNAME(32), name) < 0) {
			perror("can't get name\n");
			close(fd);
			return 1;
		}

		if(!strcmp(name, input_name)) {
			printf("found \"%s\" on %s\n", input_name, dev);
			return fd;
		}
		else { /* not what we want, check another */
			close(fd);
			fd = -1;
		}
	}
	
	return -1;
}

int main(void) {
	int fd;
	int sw = 0;

	/*
	 * check the keyboard
	 */

	fd = open_input_device("LF1000 Keyboard");
	if(fd >= 0) {
		if(ioctl(fd, EVIOCGSW(sizeof(int)), &sw) < 0) {
			perror("can't get state of switches\n");
			close(fd);
			return 1;
		}
		printf("headphones inserted: %s\ncartridge inserted: %s\n",
				sw & (1<<SW_HEADPHONE_INSERT) ? "yes" : "no",
				sw & (1<<SW_TABLET_MODE) ? "yes" : "no");
		close(fd);
	}

	/*
	 * check the USB
	 */

	fd = open_input_device("LF1000 USB");
	if(fd >= 0) {
		if(ioctl(fd, EVIOCGSW(sizeof(int)), &sw) < 0) {
			perror("can't get state of switches\n");
			close(fd);
			return 1;
		}
		printf("USB inserted: %s\n", sw & (1<<SW_LID) ? "yes" : "no");
		close(fd);
	}

	return 0;
}
