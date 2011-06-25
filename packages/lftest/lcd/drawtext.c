/* Example for writing mono-spaced text glyphs to the frame buffer */
/* Each text glyph is 8x16 RGB pixels mapped to ASCII char codes 0x20..0x80 in rows of 0x10 */

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

int main(int argc, char **argv)
{
	int layer = -1;
	int img = -1;
	int fb = -1;
	unsigned char* imgbuf = NULL;
	int base, fbsize, bufsize;
	struct stat statbuf;
	int i,j,ds,dd;
	unsigned char* s;
	unsigned char* d;
	char * text = "Hello World";
	
	if(argc < 4) {
		printf("Draw mono-spaced text glyphs to a frame buffer.\n"
			   "\tusage: drawtext <layer> <rgb_file> <text>\n"
			   "\t(example: drawtext /dev/layer0 monotext8x16.rgb ""quoted text string"")\n");
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
	s = (unsigned char*)imgbuf;
	d = (unsigned char*)fb;
	ds = 128 * 3;
	dd = 320 * 3;
	/*
	for (i = 0; i < 6; i++)
		for (j = 0; j < 8; j++) {
			memcpy(d, s, ds);
			s += ds;
			d += dd;
			}
	*/
	text = argv[3];
	for (i = 0; i < strlen(text); i++) {
		char c = text[i];
		int sc = c - 0x20;
		int sx = (sc % 0x10) * 8;
		int sy = (sc / 0x10) * 16;
		s = (unsigned char*)((int)imgbuf + (sy*128*3 + sx*3));
		d = (unsigned char*)((int)fb + i*8*3);
		for (j = 0; j < 16; j++) {
			memcpy(d, s, 8*3);
			s += ds;
			d += dd;
		}	
	}
		
cleanup:
	if (imgbuf != NULL) free(imgbuf);
	if (fb >= 0) close(fb);
	if (img >= 0) close(img);
	if (layer >= 0) close(layer);
	return 0;
}
