/* a simple volume checker */

#include <unistd.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

static int fd;
int main(void)
{
	unsigned int arg = 0;
	int ret;

	fd = open("/dev/dsp", O_RDWR);
	if(fd < 0) {
		perror("failed to open /dev/dsp\n");
		return 1;
	}

	ret = ioctl(fd, SOUND_MIXER_PCM, &arg);
        if(ret == -1) {
		perror("SOUND_MIXER_PCM");
		return 1;
	}
	printf("Current volume is %d,%d\n\n", (arg>>8)&0xFF, arg & 0xFF);

	close(fd);
	return 0;
}
