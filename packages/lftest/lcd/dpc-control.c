/* Display Controller (DPC) API examples 
 *
 * Andrey Yurovsky <andrey@cozybit.com> */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/lf1000/dpc_ioctl.h>

int main(int argc, char **argv)
{
	int dpc;
	int req;
	int arg = 0;
	void *ptr = NULL;
	union dpc_cmd c;
	int response;
	char type = 0;

	if(argc < 4) {
		printf( "usage: %s <device> <command> <value>\n\n"
				"commands:\n"
				"\tinterrupt <en|dis>\n" 
				"\thsync <avwidth> <hsw> <hfp> <hbp> <invhsync>\n"
				"\tvsync <avheigh> <vsw> <vfp> <vbp> <invvsync> <eavh> <evsw> <evfp> <evbp>\n"
				"\tclock0 <source> <div> <delay> <outinv> <outen>\n"
				"\tclock1 <source> <div> <delay> <outinv>\n"
				"\tmode <fmt> <intlace> <inv> <rgb> <swap> <yc> <clip> <sync> <clk> <invclk>\n"
				"\tswaprb <en|dis>\n"
				"\tcontrast <0-15>      set contrast\n"
				"\tbrightness <0-255>   set brightness\n"
				"\tbacklight <0-512>    set backlight\n"
				"\n"
				"\tget all              get current settings\n"
				, argv[0] );
		return 0;
	}

	dpc = open(argv[1], O_WRONLY);
	if(dpc < 0) {
		printf("error: failed to open %s\n", argv[1]);
		return 1;
	}

	if(!strcmp(argv[2],"interrupt")) {
		req = DPC_IOCTINTENB;
		type = 0;
		if(!strcmp(argv[3],"en"))
			arg = 1;
		else if(!strcmp(argv[3],"dis"))
			arg = 0;
		else {
			printf("invalid argument\n");
			close(dpc);
			return 1;
		}
	}
	else if(!strcmp(argv[2],"swaprb")) {
		req = DPC_IOCTSWAPRB;
		if(argc < 4) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		if(!strcmp(argv[3],"en"))
			arg = 1;
		else if(!strcmp(argv[3],"dis"))
			arg = 0;
		else {
			printf("invalid argument\n");
			close(dpc);
			return 1;
		}
	}
	else if(!strcmp(argv[2],"hsync")) {
		req = DPC_IOCSHSYNC;
		type = 1;
		if(argc < 7) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		c.hsync.avwidth 	= atoi(argv[3]);
		c.hsync.hsw		= atoi(argv[4]);
		c.hsync.hfp 		= atoi(argv[5]);
		c.hsync.hbp 		= atoi(argv[6]);
		c.hsync.inv_hsync	= atoi(argv[7]);
		ptr = &c.hsync;
	}
	else if(!strcmp(argv[2],"vsync")) {
		req = DPC_IOCSVSYNC;
		type = 1;
		if(argc < 12) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		c.vsync.avheight	= atoi(argv[3]);
		c.vsync.vsw 		= atoi(argv[4]);
		c.vsync.vfp 		= atoi(argv[5]);
		c.vsync.vbp 		= atoi(argv[6]);
		c.vsync.inv_vsync 	= atoi(argv[7]);
		c.vsync.eavheight 	= atoi(argv[8]);
		c.vsync.evsw 		= atoi(argv[9]);
		c.vsync.evfp 		= atoi(argv[10]);
		c.vsync.evbp 		= atoi(argv[11]);
		ptr = &c.vsync;
	}
	else if(!strcmp(argv[2],"clock0")) {
		req = DPC_IOCSCLOCK0;
		type = 1;
		if(argc < 8) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		c.clock0.source 	= atoi(argv[3]);
		c.clock0.div		= atoi(argv[4]);
		c.clock0.delay 		= atoi(argv[5]);
		c.clock0.out_inv	= atoi(argv[6]);
		c.clock0.out_en 	= atoi(argv[7]);
		ptr = &c.clock0;
	}
	else if(!strcmp(argv[2],"clock1")) {
		req = DPC_IOCSCLOCK1;
		type = 1;
		if(argc < 7) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		c.clock1.source 	= atoi(argv[3]);
		c.clock1.div		= atoi(argv[4]);
		c.clock1.delay 		= atoi(argv[5]);
		c.clock1.out_inv	= atoi(argv[6]);
		ptr = &c.clock1;
	}
	else if(!strcmp(argv[2],"mode")) {
		req = DPC_IOCSMODE;
		type = 1;
		if(argc < 13) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		c.mode.format 		= atoi(argv[3]);
		c.mode.interlace	= atoi(argv[4]);
		c.mode.invert_field	= atoi(argv[5]);
		c.mode.rgb_mode		= atoi(argv[6]);
		c.mode.swap_rb		= atoi(argv[7]);
		c.mode.ycorder		= atoi(argv[8]);
		c.mode.clip_yc		= atoi(argv[9]);
		c.mode.embedded_sync	= atoi(argv[10]);
		c.mode.clock		= atoi(argv[11]);
		c.mode.inverted_clock	= atoi(argv[12]);
		ptr = &c.mode;
	}
	else if(!strcmp(argv[2],"contrast")) {
		req = DPC_IOCTCONTRAST;
		type = 0;
		if(argc < 4) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		arg = strtol(argv[3],NULL,0);
	}
	else if(!strcmp(argv[2],"brightness")) {
		req = DPC_IOCTBRIGHTNESS;
		type = 0;
		if(argc < 4) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		arg = strtol(argv[3],NULL,0);
	}
	else if(!strcmp(argv[2],"backlight")) {
		req = DPC_IOCTBACKLIGHT;
		type = 0;
		if(argc < 4) {
			printf("invalid number of arguments\n");
			close(dpc);
			return 1;
		}
		arg = strtol(argv[3],NULL,0);
	}
	// FIXME: just test retrieving values
	else if(!strcmp(argv[2],"get")) {
		req = DPC_IOCQBACKLIGHT;
		response = ioctl(dpc, req, &arg);
		if (response < 0) {
			printf("ioctl error = %d\n", response);
			close(dpc);
			return(1);
		}
		printf("backlight =%d\n", response);

		req = DPC_IOCQBRIGHTNESS;
		response = ioctl(dpc, req, &arg);
		if (response < 0) {
			printf("ioctl error = %d\n", response);
			close(dpc);
			return(1);
		}
		printf("brightness=%d\n", response);
		req = DPC_IOCQCONTRAST;
		response = ioctl(dpc, req, &arg);
		if (response < 0) {
			printf("ioctl error = %d\n", response);
			close(dpc);
			return(1);
		}
		printf("contrast  =%d\n", response);
		close(dpc);
		return(0);	// FIXME: restructure later
	}
	else {
		printf("unknown command\n");
		close(dpc);
		return 1;
	}

	/* issue the request */
	if(type == 0 || type == 2)
		response = ioctl(dpc, req, arg);
	else 
		response = ioctl(dpc, req, ptr);

	if(response < 0)
		perror("ioctl failed\n");
	else if (type == 2)
		printf("SPI[0x%02X] = 0x%02X\n", strtol(argv[3],NULL,0), (response & 0xFF));
	close(dpc);
	return 0;
}
