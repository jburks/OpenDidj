/* Multi-Layer Controller (MLC) API examples 
 *
 * Andrey Yurovsky <andrey@cozybit.com> */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/lf1000/mlc_ioctl.h>

int main(int argc, char **argv)
{
	int mlc;
	int req;
	int arg = 0;
	int response;
	union mlc_cmd c;

	if(argc < 4) {
		printf( "usage: mlc-control <device> <action> <command> [args]\n\n"
				"commands:\n"
				"\t<set>     enable     <on|off>\n"
				"\t<get>     screensize\n"
				"\t<get|set> background <color>\n"
			 	"\t<get|set> priority   <value>\n"
				"\t<get|set> dirty\n" );
		return 0;
	}

	mlc = open(argv[1], O_WRONLY);
	if(mlc < 0) {
		perror("failed to open device\n");
		return 1;
	}

	if(argv[2][0] == 's') { /* SET */
		if(!strcmp(argv[3],"background")) {
			if(argc < 5) {
				printf("invalid number of arguments\n");
				close(mlc);
				return 1;
			}
			if(sscanf(argv[4], "%06X", &arg) <= 0) {
				printf("invalid argument\n");
				close(mlc);
				return 1;
			}
			req = MLC_IOCTBACKGND;
		}
		else if(!strcmp(argv[3],"priority")) {
			if(argc < 5) {
				printf("invalid number of arguments\n");
				close(mlc);
				return 1;
			}
			arg = atoi(argv[4]);
			req = MLC_IOCTPRIORITY;
		}
		else if(!strcmp(argv[3],"enable")) {
			if(argc < 5) {
				printf("invalid number of arguments\n");
				close(mlc);
				return 1;
			}
			if(!strcmp(argv[4],"on"))
				arg = 1;
			else if(!strcmp(argv[4],"off"))
				arg = 0;
			else {
				printf("error: invalid argument, use \"on\" or \"off\"\n");
				close(mlc);
				return 1;
			}
			req = MLC_IOCTENABLE;
		}
		else if(!strcmp(argv[3],"priority")) {
			if(argc < 5) {
				printf("invalid number of arguments\n");
				close(mlc);
				return 1;
			}
			arg = atoi(argv[4]);
			req = MLC_IOCTPRIORITY;
		}
		else if(!strcmp(argv[3],"dirty")) {
			arg = 0;
			req = MLC_IOCTTOPDIRTY;
		}
		else {
			printf("invalid command\n");
			close(mlc);
			return 1;
		}

		response = ioctl(mlc, req, arg);
		if(response < 0)
			perror("error: ioctl failed\n");
	}
	else if(argv[2][0] == 'g') { /* GET */
		if(!strcmp(argv[3],"background")) {
			response = ioctl(mlc, MLC_IOCQBACKGND, 0);
			printf("background color: %06X\n", response);
		}
		else if(!strcmp(argv[3],"screensize")) {
			response = ioctl(mlc, MLC_IOCGSCREENSIZE, &c);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(mlc);
				return 1;
			}
			printf("width:  %d\nheight: %d\n", 
				   c.screensize.width,
				   c.screensize.height);
		}
		else if(!strcmp(argv[3],"priority")) {
			response = ioctl(mlc, MLC_IOCQPRIORITY, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(mlc);
				return 1;
			}
			printf("priority: %d\n", response);
		}
		else {
			printf("invalid command\n");
			close(mlc);
			return 1;
		}
	}
	else {
		printf("error: invalid action, use \"get\" or \"set\"\n");
		close(mlc);
		return 1;
	}

	close(mlc);
	return 0;
}
