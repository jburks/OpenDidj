/* keyboard-example.c
 *
 * Simple user-space example for reading from /dev/input/event0 to test the
 * buttons found on the ME_LF1000 development board.  See evtest.c for more
 * input events programming examples.  This program asks the user to press
 * (and release) each button in the button map, in order.  It reads one event
 * at a time from /dev/input/event0.  
 *
 * You can actually read more than one event at a time (see evtest.c) if 
 * needed.
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

#define KEYBOARD_NAME 	"LF1000 Keyboard"
#define MAX_DEVNODES	8

/*
 * button mapping: names followed by expected key codes. 
 */

#define NUM_BUTTONS	12

char *button_name[NUM_BUTTONS] = {
	"up",
	"down",
	"right",
	"left",
	"A",
	"B",
	"left shoulder",
	"right shoulder",
	"start", /* 'menu' */
	"hint",
	"pause",
	"brightness",
};

char button_code[NUM_BUTTONS] = {
	KEY_UP,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_A,
	KEY_B,
	KEY_L,
	KEY_R,
	KEY_M,
	KEY_H,
	KEY_P,
	KEY_X,
};

int main(void) {
	int fd, rd, i, j, version;
	char pressed, released;
	struct input_event ev[1];
	char dev[20];
	char name[32];

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

		if(!strcmp(name, KEYBOARD_NAME)) {
			printf("found \"%s\" on %s\n", KEYBOARD_NAME, dev);
			break;
		}
		else { /* not what we want, check another */
			close(fd);
			fd = -1;
		}
	}

	if(fd < 0) {
		printf("failed to find the keyboard\n");
		return 1;
	}

	for(i = 0; i < NUM_BUTTONS; i++) {
		printf("Press the \"%s\" button\n", button_name[i]);
		pressed = 0;
		released = 0;
		while(!(pressed && released)) {
			rd = read(fd, ev, sizeof(struct input_event));
			if(rd < (int) sizeof(struct input_event)) {
				perror("read error\n");
				close(fd);
				return 1;
			}

			if(ev[0].code == button_code[i]) {
			       	if(ev[0].value == 1) {
					printf("pressed\n");
					pressed = 1;
				}
				else if(ev[0].value == 0) {
					printf("released\n");
					released = 1;
				}
			}
		}
	}

	close(fd);
	return 0;
}
