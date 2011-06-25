#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <linux/input.h>
extern char cPower;

int open_input_device(char *input_name)
{
	char dev[20];
	char name[32];
	int fd, i;

	for(i = 0; i < 8; i++) {
		sprintf(dev, "/dev/input/event%d", i);
		fd = open(dev, O_RDONLY|O_NONBLOCK);
		if(fd < 0) {
			return 1;
		}

		if(ioctl(fd, EVIOCGNAME(32), name) < 0) {
			close(fd);
			return 1;
		}
		printf("%d = %s\n", i, name);
		if(!strcmp(name, input_name)) {
			return fd;
		}
		else { /* not what we want, check another */
			close(fd);
			fd = -1;
		}
	}
	
	return -1;
}

int GrabPowerButton(void)
{
	int fd_pwr;
	
	/* 
	 * grab the Power Button input device 
	 */
	fd_pwr = open_input_device("Power Button");
	if(fd_pwr <= 0) 
		return -1;
	return fd_pwr;
}

/*
  Return 1 if power was pressed
  Return 0 if nothing happened
*/

int MonitorPower(int fd)
{
	int ret, size;
	struct pollfd sfd;

	sfd.fd = fd;
	sfd.events = POLLIN;
	/* monitor our file descriptor*/
	ret = poll(&sfd, 1, -1);
	close(fd);
	printf("Power button detected\n");
	cPower = 0;
	return 1;
}

