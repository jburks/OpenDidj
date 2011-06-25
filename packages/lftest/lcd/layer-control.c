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
#include "formats.h"

int on_or_off(char *arg)
{
	if(!strcmp(arg,"on"))
		return 1;
	if(!strcmp(arg,"off"))
		return 0;
	printf("error: invalid argument, use \"on\" or \"off\"\n");
	return -1;
}

int main(int argc, char **argv)
{
	int layer;
	int sw;
	int req, i;
	void *arg = 0;
	int val;
	int response;
	union mlc_cmd c;

	if(argc < 4) {
		printf( "usage: layer-control <device> <action> <command> [args]\n\n"
				"commands:\n"
				"<set>     enable      <on|off>\n"
				"<get|set> 3D          <on|off>\n"
				"<get|set> blend       <on|off>\n"
				"<get|set> transparent <on|off>\n"
				"<get|set> invert      <on|off>\n"
				"<get|set> invisible   <on|off>\n"
				"<set>     dirty\n"
				"<get|set> address     <address>\n"
				"<get>     fbsize\n"
				"<get|set> locksize    <locksize>\n"
				"<get|set> hstride     <hstride>\n"
				"<get|set> vstride     <vstride>\n"
				"<get|set> format      <format>\n"
				"<get|set> position    <top> <left> <right> <bottom>\n"
				"<get|set> invisarea  <top> <left> <right> <bottom>\n"
				"<get|set> tpcolor     <color>\n"
				"<get|set> alpha       <alpha>\n"
				"<get|set> invcolor    <color>\n" );
		return 0;
	}

	/* open the layer device */
	layer = open(argv[1], O_WRONLY);
	if(layer < 0) {
		printf("error: failed to open device\n");
		return 1;
	}

	if(argv[2][0] == 'g') {
		if(!strcmp(argv[3],"position")) {
			response = ioctl(layer, MLC_IOCGPOSITION, &c);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\ttop    = %d\n"
				   "\tleft   = %d\n"
				   "\tright  = %d\n"
				   "\tbottom = %d\n", c.position.top, 
				   		      c.position.left, 
				  		      c.position.right, 
						      c.position.bottom); 
		}
		if(!strcmp(argv[3],"invisarea")) {
			response = ioctl(layer, MLC_IOCGINVISIBLEAREA, &c);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\ttop    = %d\n"
				   "\tleft   = %d\n"
				   "\tright  = %d\n"
				   "\tbottom = %d\n", c.position.top, 
				   		      c.position.left, 
				  		      c.position.right, 
						      c.position.bottom); 
		}
		else if(!strcmp(argv[3],"format")) {
			response = ioctl(layer, MLC_IOCQFORMAT, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\tformat = 0x%04X\n", response);
		}
		else if(!strcmp(argv[3],"address")) {
			response = ioctl(layer, MLC_IOCQADDRESS, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\taddress = 0x%08X\n", response);
		}
		else if(!strcmp(argv[3],"fbsize")) {
			response = ioctl(layer, MLC_IOCQFBSIZE, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\tfbsize = 0x%08X\n", response);
		}
		else if(!strcmp(argv[3],"hstride")) {
			response = ioctl(layer, MLC_IOCQHSTRIDE, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\tHStride = %d\n", response);
		}
		else if(!strcmp(argv[3],"vstride")) {
			response = ioctl(layer, MLC_IOCQVSTRIDE, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\tVStride = %d\n", response);
		}
		else if(!strcmp(argv[3],"alpha")) {
			response = ioctl(layer, MLC_IOCQALPHA, 0);
			if(response < 0) {
				perror("error: ioctl failed\n");
				close(layer);
				return 1;
			}
			printf("\talpha = %d\n", response);
		}
		else {
			printf("unknown command\n");
			close(layer);
			return 1;
		}
	}
	else if(argv[2][0] == 's') {
		if(!strcmp(argv[3],"enable")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCTLAYEREN;
			arg = (void*)sw;
		}
		else if(!strcmp(argv[3],"3D")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCT3DENB;
			arg = (void *)sw;
		}
		else if(!strcmp(argv[3],"blend")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCTBLEND;
			arg = (void *)sw;
		}
		else if(!strcmp(argv[3],"transparent")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCTTRANSP;
			arg = (void *)sw;
		}
		else if(!strcmp(argv[3],"invert")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCTINVERT;
			arg = (void *)sw;
		}
		else if(!strcmp(argv[3],"invisible")) {
			sw = on_or_off(argv[4]);
			if(sw < 0) {
				close(layer);
				return 1;
			}
			req = MLC_IOCTINVISIBLE;
			arg = (void *)sw;
		}
		else if(!strcmp(argv[3],"dirty")) {
			req = MLC_IOCTDIRTY;
		}
		else if(!strcmp(argv[3],"address")) {
			req = MLC_IOCTADDRESS;
			if(sscanf(argv[4], "%08X", &arg) <= 0) {
				printf("error: invalid argument\n");
				close(layer);
				return 1;
			}
		}
		else if(!strcmp(argv[3],"alpha")) {
			req = MLC_IOCTALPHA;
			arg = (void *)atoi(argv[4]);
		}
		else if(!strcmp(argv[3],"tpcolor")) {
			req = MLC_IOCTTPCOLOR;
			if(sscanf(argv[4], "%06X", &arg) <= 0) {
				printf("error: invalid argument\n");
				close(layer);
				return 1;
			}
		}
		else if(!strcmp(argv[3],"locksize")) {
			req = MLC_IOCTLOCKSIZE;
			arg = (void *)atoi(argv[4]);
		}
		else if(!strcmp(argv[3],"hstride")) {
			req = MLC_IOCTHSTRIDE;
			arg = (void *)atoi(argv[4]);
		}
		else if(!strcmp(argv[3],"vstride")) {
			req = MLC_IOCTVSTRIDE;
			arg = (void *)atoi(argv[4]);
		}
		else if(!strcmp(argv[3],"position")) {
			if(argc < 8) {
				printf("invalid number of arguments\n");
				close(layer);
				return 1;
			}
			req = MLC_IOCSPOSITION;
			c.position.top    = atoi(argv[4]);
			c.position.left   = atoi(argv[5]);
			c.position.right  = atoi(argv[6]);
			c.position.bottom = atoi(argv[7]);
			arg = (void *)&c;
		}
		else if(!strcmp(argv[3],"invisarea")) {
			if(argc < 8) {
				printf("invalid number of arguments\n");
				close(layer);
				return 1;
			}
			req = MLC_IOCSINVISIBLEAREA;
			c.position.top    = atoi(argv[4]);
			c.position.left   = atoi(argv[5]);
			c.position.right  = atoi(argv[6]);
			c.position.bottom = atoi(argv[7]);
			arg = (void *)&c;
		}
		else if(!strcmp(argv[3],"format")) {
			req = MLC_IOCTFORMAT;
			arg = (void *)-1;
			for(i = 0; i < NUM_FORMATS; i++) 
				if(!strcmp(argv[4],format_name[i]))
					arg = (void *)format_value[i];
			if(arg == (void *)-1) {
				printf("invalid format\n");
				close(layer);
				return 1;
			}
		}
		else {
			printf("unknown command\n");
			close(layer);
			return 1;
		}

		/* issue the request */
		response = ioctl(layer, req, arg);
		if(response < 0)
			perror("ioctl failed\n");
	}
	else {
		printf("error: invalid action, use \"get\" or \"set\"\n");
		close(layer);
		return 1;
	}

	close(layer);
	return 0;
}
