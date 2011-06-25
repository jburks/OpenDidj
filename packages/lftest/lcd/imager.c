/* Faster example for writing raw image file to the frame buffer */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>

#include <linux/lf1000/mlc_ioctl.h>

/* Added for png support */
#include "readpng.h"
#include "formats.h"

int main(int argc, char **argv)
{
	int layer = -1;
	int img = -1;
	int fb = -1;
	unsigned char* imgbuf = NULL;
	FILE *pngfile = NULL;
	int base, fbsize, bufsize;
	struct stat statbuf;
	struct position_cmd pos;
	unsigned long width, height, bpr;
	int bpp;
	char *png_data = NULL;
	int fmt;

	if(argc < 3) {
		printf("Writes raw RGB B8G8R8 data to a frame buffer.\n"
			   "\tusage: imager <layer> <rgb_file>\n"
			   "\t(example: imager /dev/layer0 tux.rgb)\n");
		return 0;
	}

	layer = open(argv[1], O_RDWR|O_SYNC);
	if(layer < 0) {
		perror("failed to open layer device");
		return 1;
	}

	/* get the base address for this layer's frame buffer */
	base = ioctl(layer, MLC_IOCQADDRESS, 0);
	if(base < 0) {
		perror("get_address ioctl failed");
		goto cleanup;
	}

	/* figure out the size of the frame buffer */
	fbsize = ioctl(layer, MLC_IOCQFBSIZE, 0);
	if(fbsize < 0) {
		perror("get_fbsize ioctl failed");
		goto cleanup;
	}

	fb = (int)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, layer, 
				   base);
	if(fb < 0) {
		perror("mmap() failed");
		goto cleanup;
	}

	/* The file is either a raw rgb or png. */
	img = open(argv[2], O_RDONLY&~(O_CREAT));
	if(img < 0) {
		perror("failed to open image file");
		close(layer);
		return 1;
	}

	if (!(pngfile = fopen(argv[2], "rb"))) {
		perror("can't open intput file.");
		return 1;
	}

	if (readpng_init(pngfile, &width, &height) == 0) {
		/* We have a png. Decode it. */
		png_data = readpng_get_image(2.2, &bpp, &bpr);
		if (!png_data) {
			fprintf(stderr, "unable to decode PNG image\n");
			goto cleanup;
		}	

		/* If bpp is 3, assume B8G8R8 which is 0xC653.  If bpp is 4,
		 * assume A8B8G8R8 which is 0x8653 */
		if (bpp == 3) {
			fmt = 0xC653;
		} else if (bpp == 4) {
			fmt = 0x8653;
		} else {
			fprintf(stderr, "Unsupported bytes per pixel: %d\n", bpp);
			goto cleanup;
		}
		
		/* Get the screen dimensions */
		if(ioctl(layer, MLC_IOCGPOSITION, &pos) < 0) {
			perror("failed to get layer dimensions.");
			goto cleanup;
		}
		
		/* Check image format */
		if(pos.bottom - pos.top + 1 != height ||
		   pos.right - pos.left + 1 != width ||
		   bpr/bpp != pos.right - pos.left + 1) {
			fprintf(stderr,
				"Image dimensions don't match screen\n");
			goto cleanup;
		}
		
		/* Change screen layer format prior to framebuffer copy */
		if( (ioctl(layer, MLC_IOCTFORMAT, fmt) < 0) ||
		    (ioctl(layer, MLC_IOCTHSTRIDE, bpp) < 0) ||
		    (ioctl(layer, MLC_IOCTVSTRIDE, bpr) < 0) ||
		    (ioctl(layer, MLC_IOCTDIRTY, 1) < 0) ) {
			perror("failed to set format.");
			goto cleanup;
		}

		/* Sync screen layer update prior to framebuffer copy */
		while (ioctl(layer, MLC_IOCQDIRTY, 0) == 1)
			;
		memcpy((void *)fb, png_data, height*width*bpp);
		readpng_cleanup(FALSE);

	} else {

		/* read the image file to local buffer */
		if (fstat(img, &statbuf) < 0) {
			perror("fstat() failed");
			goto cleanup;
		}
		bufsize = (statbuf.st_size < fbsize) ? statbuf.st_size : fbsize;
		imgbuf = (unsigned char*)malloc(bufsize);
		if (imgbuf == NULL) {
			perror("malloc() failed");
			goto cleanup;
		}
		if (read(img, imgbuf, bufsize) < 0) {
			perror("read() failed");
			goto cleanup;
		}
		/* write the image to the framebuffer */
		memcpy((void*)fb, imgbuf, bufsize);
	}

cleanup:
	if (imgbuf != NULL) free(imgbuf);
	if (fb >= 0) close(fb);
	if (img >= 0) close(img);
	if (layer >= 0) close(layer);
	if (png_data) free(png_data);
	if (pngfile) fclose(pngfile);
	return 0;
}
