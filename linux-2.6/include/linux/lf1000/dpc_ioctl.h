/* LF1000 Display Controller (DPC) Driver
 *
 * dpc_ioctl.h -- Supported ioctl commands.
 *
 * Andrey Yurovsky <andrey@cozybit.com> */

#ifndef DPC_IOCTL_H
#define DPC_IOCTL_H

#define DPC_IOC_MAGIC	'd'

struct hsync_cmd {
	unsigned int avwidth;
	unsigned int hsw, hfp, hbp;
	unsigned char inv_hsync;
};

struct vsync_cmd {
	unsigned int avheight;
	unsigned int vsw, vfp, vbp;
	unsigned char inv_vsync;
	unsigned int eavheight;
    unsigned int evsw, evfp, evbp;
};

struct mode_cmd {
	unsigned char format;
    unsigned char interlace;
    unsigned char invert_field;
    unsigned char rgb_mode;
    unsigned char swap_rb;
    unsigned char ycorder;
    unsigned char clip_yc;
    unsigned char embedded_sync;
    unsigned char clock;
    unsigned char inverted_clock;
};

struct clock0_cmd {
	unsigned char source;
	unsigned char div;
	unsigned char delay;
	unsigned char out_inv;
	unsigned char out_en;
};

struct clock1_cmd {
	unsigned char source;
	unsigned char div;
	unsigned char delay;
	unsigned char out_inv;
};

union dpc_cmd {
	struct hsync_cmd hsync;
	struct vsync_cmd vsync;
	struct clock0_cmd clock0; 
	struct clock1_cmd clock1; 
	struct mode_cmd mode;
};


/* supported ioctls */

#define DPC_IOCTINTENB	_IOW(DPC_IOC_MAGIC, 0, unsigned int)
#define DPC_IOCSHSYNC	_IOW(DPC_IOC_MAGIC, 1, struct hsync_cmd)
#define DPC_IOCSVSYNC	_IOW(DPC_IOC_MAGIC, 2, struct vsync_cmd *)
#define DPC_IOCSCLOCK0  _IOW(DPC_IOC_MAGIC, 3, struct clock0_cmd)
#define DPC_IOCSCLOCK1  _IOW(DPC_IOC_MAGIC, 4, struct clock1_cmd *)
#define DPC_IOCSMODE	_IOW(DPC_IOC_MAGIC, 5, struct mode_cmd *)
#define DPC_IOCTSWAPRB	_IOW(DPC_IOC_MAGIC, 6, unsigned int)
#define DPC_IOCTCONTRAST _IOW(DPC_IOC_MAGIC, 7, unsigned int)
#define DPC_IOCTBRIGHTNESS _IOW(DPC_IOC_MAGIC, 8, unsigned int)
#define DPC_IOCTBACKLIGHT _IOW(DPC_IOC_MAGIC, 10, unsigned int)
#define DPC_IOCQBACKLIGHT _IOR(DPC_IOC_MAGIC, 11, unsigned int)
#define DPC_IOCQBRIGHTNESS _IOR(DPC_IOC_MAGIC, 12, unsigned int)
#define DPC_IOCQCONTRAST  _IOR(DPC_IOC_MAGIC, 13, unsigned int)
#define DPC_IOCTBACKLIGHTVIRT _IOW(DPC_IOC_MAGIC, 14, int)
#define DPC_IOCQBACKLIGHTVIRT _IOR(DPC_IOC_MAGIC, 15, int)

#endif
