#include <unistd.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

static int fd;
static channels = 1;
static sample_rate = 8000;
static format = AFMT_S16_LE;

char *fmt2str(int fmt) {

	switch(fmt) {
	case AFMT_MU_LAW: return "AFMT_MU_LAW";
	case AFMT_A_LAW: return "AFMT_A_LAW"; 
	case AFMT_IMA_ADPCM: return "AFMT_IMA_ADPCM"; 
	case AFMT_U8: return "AFMT_U8"; 
	case AFMT_S16_LE: return "AFMT_S16_LE"; 
	case AFMT_S16_BE: return "AFMT_S16_BE"; 
	case AFMT_S8: return "AFMT_S8"; 
	case AFMT_U16_LE: return "AFMT_U16_LE"; 
	case AFMT_U16_BE: return "AFMT_U16_BE"; 
	case AFMT_MPEG: return "AFMT_MPEG"; 
	case AFMT_AC3: return "AFMT_AC3"; 
	default: return "Unknown";
	}
}

void print_state(void)
{
	int arg;

	/* print supported formats */
	if(ioctl(fd, SNDCTL_DSP_GETFMTS, &arg) == -1) {
		perror("SNDCTL_DSP_GETFMTS");
		return;
	}
	printf("Supported audio formats:\n");
	if(arg & AFMT_MU_LAW) printf("AFMT_MU_LAW (%d)\n", AFMT_MU_LAW);
	if(arg & AFMT_A_LAW) printf("AFMT_A_LAW (%d)\n", AFMT_A_LAW);
	if(arg & AFMT_IMA_ADPCM) printf("AFMT_IMA_ADPCM (%d)\n", AFMT_A_LAW);
	if(arg & AFMT_U8) printf("AFMT_U8 (%d)\n", AFMT_U8);
	if(arg & AFMT_S16_LE) printf("AFMT_S16_LE (%d)\n", AFMT_S16_LE);
	if(arg & AFMT_S16_BE) printf("AFMT_S16_BE (%d)\n", AFMT_S16_BE);
	if(arg & AFMT_S8) printf("AFMT_S8 (%d)\n", AFMT_S8);
	if(arg & AFMT_U16_LE) printf("AFMT_U16_LE (%d)\n", AFMT_U16_LE);
	if(arg & AFMT_U16_BE) printf("AFMT_U16_BE (%d)\n", AFMT_U16_BE);
	if(arg & AFMT_MPEG) printf("AFMT_MPEG (%d)\n", AFMT_MPEG);
	if(arg & AFMT_AC3) printf("AFMT_AC3 (%d)\n", AFMT_AC3);
	printf("\n");

	/* print current format */
	arg = AFMT_QUERY;
	if(ioctl(fd, SNDCTL_DSP_SETFMT, &arg) == -1) {
		perror("SNDCTL_DSP_SETFMT");
		return;
	}
	printf("Current audio format: %s\n\n", fmt2str(arg));
}

int set_fmt(int fmt)
{
	int arg;
	arg = fmt;

	if(ioctl(fd, SNDCTL_DSP_SETFMT, &arg) == -1) {
		perror("SNDCTL_DSP_SETFMT");
		return 1;
	}
	if(fmt != arg) {
		printf("Couldn't change format to %s (%d)\n",
		       fmt2str(fmt), fmt);
	}
	printf("Changed format to %s\n\n", fmt2str(arg));
	format = arg;
	return 0;
}

int set_sample_rate(int rate)
{
	int arg;
	arg = rate;

	if(ioctl(fd, SNDCTL_DSP_SPEED, &arg) == -1) {
		perror("SNDCTL_DSP_SETSPEED");
		return 1;
	}
	if(arg != rate) {
		printf("Couldn't change rate to %d\n", rate);
	}
	printf("Changed rate to %d\n\n", arg);
	sample_rate = arg;
	return 0;
}

int set_volume(unsigned int vol)
{
	unsigned int arg;
	arg = vol;

	if(ioctl(fd, SOUND_MIXER_PCM, &arg) == -1) {
		perror("SOUND_MIXER_PCM");
		return 1;
	}
	if(arg != vol) {
		printf("Couldn't change volume to %d\n", vol);
	}
	printf("Changed volume to %d\n\n", arg);
	return 0;
}

int set_channels(unsigned int chan)
{
	unsigned int arg;
	arg = chan;

	if(ioctl(fd, SNDCTL_DSP_CHANNELS, &arg) == -1) {
		perror("SNDCTL_DSP_CHANNELS");
		return 1;
	}
	if(arg != chan) {
		printf("Couldn't change channels to %d\n", chan);
	}
	printf("Changed channels to %d\n\n", arg);
	channels = arg;
	return 0;
}

int defer_play = 0;
char wave_file[200];

struct chunk {
	unsigned int chunk_id;
	unsigned int chunk_size;
} __attribute__((__packed__));

struct riff_header {
	unsigned int RIFF;
	unsigned int size_file;
	unsigned int WAVE;
} __attribute__((__packed__));

struct fmt_header {
	unsigned short fmt0001;
	unsigned short chan;
	unsigned int rate;
	unsigned int byte_rate;
	unsigned short block_align; //NumChannels * BitsPerSample/8
	unsigned short bits;
} __attribute__((__packed__));

struct wav_header {
	unsigned int RIFF;
	unsigned int size_file;
	unsigned int WAVE;

	unsigned int fmt20;
	unsigned int size1;
	unsigned short fmt0001;
	unsigned short chan;
	unsigned int rate;
	unsigned int byte_rate;
	unsigned short block_align; //NumChannels * BitsPerSample/8
	unsigned short bits;

	unsigned int data;
	unsigned int bytes;
} __attribute__((__packed__));


int handle_fmt_chunk(struct chunk *c, struct fmt_header *fmt, FILE *wf)
{

	int sfmt;
	char dump[256];
	int bytes;

	if ((c->chunk_id != 0x20746D66) ||
	    (c->chunk_size < sizeof(struct fmt_header))) {
		printf("Invalid fmt header");
		return 1;
	}
	bytes = fread((void *)fmt, 1, sizeof(struct fmt_header), wf);
	if(bytes != sizeof(struct fmt_header)) {
		printf("Failed to read fmt header\n");
		return 1;
	}
	printf("Sample Rate: %d\n", fmt->rate);
	if(set_sample_rate(fmt->rate))
		return 1;

	printf("Channels: %d\n", fmt->chan);
	if(set_channels(fmt->chan))
		return 1;

	printf("Bits per sample: %d\n", fmt->bits);
	switch (fmt->bits) {
	case 8:
		sfmt = AFMT_S8;
		break;
	case 16:
		sfmt = AFMT_S16_LE;
		break;
	default:
		printf("Unsupported bits per channel: %d\n", fmt->bits);
		return -1;
	}
	if(set_fmt(sfmt))
		return -1;

	/* consume remaining bytes, if any */
	bytes = fread(dump, 1, c->chunk_size - sizeof(struct fmt_header), wf);
	if(bytes != c->chunk_size - sizeof(struct fmt_header)) {
		printf("Failed to consumer remainder of fmt header\n");
		return 1;
	}

	return 0;
}


#define BUFFER_SIZE 4096
unsigned int buffer_size;

int play_data_chunk(struct chunk *c, FILE *wf)
{
	int bytes, total_read, total_written;
	char *buffer;

	buffer = (char *)malloc(buffer_size);
	if(!buffer) {
		printf("Failed to allocate buffer.\n");
		return 1;
	}

	/* parse data chunk */
	if(c->chunk_size < 16) {
		printf("Bad data chunk\n");
		return 1;
	}
	
	/* stream out data */
	printf("Bytes of data: %d\n", c->chunk_size);
	while(c->chunk_size) {
		/* read from file */
		total_read = fread(buffer, 1, buffer_size, wf);
		if(total_read == 0) {
			printf("Unexpected end of wav file\n");
			return 1;
		}
		c->chunk_size -= total_read;

		/* write to dsp */
		total_written = 0;
		while(total_read) {
			bytes = write(fd, &buffer[total_written], total_read);
			//printf("Wrote %d of %d bytes to dsp\n", bytes, total_read);
			if((bytes == 0) || (bytes == -1)) {
				printf("Unexpected failure to write dsp\n");
				return 1;
			}
			total_read -= bytes;
			total_written += bytes;
		}
	}

	free(buffer);
	return 0;
}

int play_wave(char *file) {

	FILE *wf;
	struct chunk chunk;
	struct riff_header riff;
	struct fmt_header fmt;
	char dump;
	int bytes;
	int configured;

	if(!(wf = fopen(file, "rb"))) {
		return 1;
	}
	
	/* Read the RIFF header */
	bytes = fread(&riff, 1, sizeof(riff), wf);
	if(bytes != sizeof(riff)) {
		printf("Failed to find a riff header\n");
		return 1;
	}
	bytes = riff.size_file + 8 - sizeof(riff);
	if (riff.RIFF != 0x46464952 || riff.WAVE != 0x45564157 || bytes < 0) {
		printf ("Not a wave file\n");
		return 1;
	}

	/* Read chunks */
	configured = 0;
	while(fread(&chunk, 1, sizeof(chunk), wf) == sizeof(chunk)) {
		
		if(chunk.chunk_id == 0x20746D66) {
			/* fmt chunk */
			if(handle_fmt_chunk(&chunk, &fmt, wf)) {
				printf("Failed to parse fmt chunk\n");
				return 1;
			}
			configured = 1;

		} else if(chunk.chunk_id == 0x61746164) {
			/* data chunk */
			if(!configured) {
				printf("fmt chunk must come before data chunk.\n");
				return 1;
			}
				
			if(play_data_chunk(&chunk, wf)) {
				return 1;
			}
		} else {
			/* unsupported chunk */
			printf("Ignoring unsupported chunk 0x%x\n",
			       chunk.chunk_id);
			while(chunk.chunk_size) {
				bytes = fread(&dump, 1, 1, wf);
				chunk.chunk_size--;
			}
		}
	}
	
	fclose (wf);
	return 0;
}

int defer_rec = 0;
int seconds = 2;

int record_wave(char *file) {
	FILE *wf;
	struct wav_header *header;
	int bytes, fmt, total_read, total_written, total_bytes, bytes_to_read;
	char *buffer;

	buffer = (char *)malloc(buffer_size);
	if(!buffer) {
		printf("Failed to allocate buffer.\n");
		return 1;
	}

	header = (struct wav_header *)malloc(sizeof(struct wav_header));
	if(!header) {
		printf("Failed to allocate wav header.\n");
		return 1;
	}

	if(!(wf = fopen(file, "wb"))) {
		return 1;
	}

	/* calculate bytes to capture */
	total_bytes = seconds * sample_rate * channels;
	if(format == AFMT_S16_LE)
		total_bytes *= 2;

	/* set up the wave header */
	header->RIFF = 0x46464952;
	header->size_file = sizeof(struct wav_header) + total_bytes;
	header->WAVE = 0x45564157;

	header->fmt20 = 0x20746d66;
	header->size1 = 0x10;
	header->fmt0001 = 0x0001;
	header->chan = channels;
	header->rate = sample_rate;

	header->data = 0x61746164;
	header->bytes = total_bytes;

	switch (format) {
	case AFMT_S8:
		header->byte_rate = sample_rate*channels;
		header->block_align = channels;
		header->bits = 8;
		break;
	case AFMT_S16_LE:
		header->byte_rate = sample_rate*channels*2;
		header->block_align = channels*2;
		header->bits = 16;
		break;
	default:
		printf("Unsupported format\n");
	}

	bytes = fwrite(header, 1, sizeof(struct wav_header), wf);
	if(bytes != sizeof(struct wav_header)) {
		printf("Failed to write wav header\n");
		return 1;
	}

	/* stream in data */
	while(total_bytes) {
		/* read from device */
		total_read = 0;
		if(total_bytes < buffer_size)
			bytes_to_read = total_bytes;
		else
			bytes_to_read = buffer_size;

		while(total_read < bytes_to_read) {
			bytes = read(fd, &buffer[total_read], bytes_to_read - total_read);
			if((bytes == 0) || (bytes == -1)) {
				printf("Unexpected failure to read dsp\n");
				return 1;
			}
			total_read += bytes;
		}
		
		total_bytes -= total_read;

		/* write to file */
		total_written = fwrite(buffer, 1, total_read, wf);
		if(total_written == 0) {
			printf("Unexpected failure writing file\n");
			return 1;
		}
	}

	free(buffer);
	free(header);
	fclose (wf);
	return 0;
}


void square_wave(char *buffer, unsigned int length, unsigned int freq)
{
	int i;
	int sample = 0;
	if(format == AFMT_S16_LE) {
		length >>= 1;
		for(i=0; i<length; i++) {
			((short *)buffer)[i] = sample;
			if(i%freq)
				sample = 8000;
		}
	} else {
		for(i=0; i<length; i++) {
			buffer[i] = sample;
			if(i%freq)
				sample = 100;
		}
	}
}

int defer_command = 0;

/* The reset_sync test writes part of a buffer with a click and then
 * uses reset or sync to purge it. */
#define SYNC 1
#define RESET 2

void reset_sync(int test) {
	unsigned int block_size, total_written, bytes;
	char *buffer;
	int i;
	
	if(ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &block_size) == -1) {
		perror("SNDCTL_DSP_GETBLKSIZE");
		return;
	}
	
	/* make a buffer that is half of the buffer size */
	block_size >>= 1;
	buffer = (char *)malloc(block_size);
	if(!buffer) {
		printf("failed to allocate buffer\n");
		return;
	}

	/* make a square wave */
	square_wave(buffer, block_size, sample_rate>>2);

	printf("sending clicks to dsp\n");
	for(i=0; i<5; i++) {
		/* write to dsp */
		total_written = 0;
		while(total_written < block_size) {
			bytes = write(fd, &buffer[total_written],
				      block_size - total_written);
			if((bytes == 0) || (bytes == -1)) {
				printf("Unexpected failure to write dsp\n");
				return;
			}
			total_written += bytes;
		}
		
		/* okay, the dsp's buffer is half full.  If we reset, some
		 * sound should come out.
		 */
		if(test == SYNC) {
			if(ioctl(fd, SNDCTL_DSP_SYNC, &block_size) == -1) {
				perror("SNDCTL_DSP_RESET");
				return;
			}
		} else {
			if(ioctl(fd, SNDCTL_DSP_RESET, &block_size) == -1) {
				perror("SNDCTL_DSP_RESET");
				return;
			}
		}
		
		sleep(1);
	}

	if(test == SYNC) {
		printf("If you heard a series of clicks, sync works.\n");
	} else {
		printf("If you heard a series of clicks, reset works.\n");
	}
}

void echo(void) {

	unsigned int bytes, total_read, total_written;
	char *buffer;

	buffer = (char *)malloc(buffer_size);
	if(!buffer) {
		printf("Failed to allocate buffer.\n");
		return;
	}

	/* echo data */
	while(1) {
		/* read from device */
		total_read = 0;
		while(total_read < buffer_size) {
			bytes = read(fd, &buffer[total_read], buffer_size - total_read);
			if((bytes == 0) || (bytes == -1)) {
				printf("Unexpected failure to read dsp\n");
				return;
			}
			total_read += bytes;
		}
		
		/* write to dsp */
		total_written = 0;
		while(total_written < buffer_size) {
			bytes = write(fd, &buffer[total_written], buffer_size - total_written);
			if((bytes == 0) || (bytes == -1)) {
				printf("Unexpected failure to write dsp\n");
				return;
			}
			total_written += bytes;
		}
	}
}

char command[50];

int run_command(void)
{
	if (strcmp("echo", command) == 0) {
		echo();
	} else if (strcmp("reset", command) == 0) {
		reset_sync(RESET);
	} else if (strcmp("sync", command) == 0) {
		reset_sync(SYNC);
	} else {
		printf("Unsupported command %s\n", command);
		return 1;
	}

	return 0;
}

#define help "Usage: \n"\
	"\tTo test ioctls:\n" \
	"\toss -f <format> -r <rate> -v <volume> -c <channels>\n" \
	"\tTo playback a wav file:\n" \
	"\toss -w <wave_file> -v <volume> -b <buffer_size>\n" \
	"\tTo record a wav files (m is for 'make'):\n" \
	"\toss -m <wave_file> -f <format> -r <rate> -b <buffer_size> -s <seconds>\n" \
	"\tTo run a test. 'echo','reset' and 'sync' are the only supported tests\n" \
	"\toss -t <test> -f <format> -r <rate> -b <buffer_size>\n" \

int handle_option(char *option, char *arg)
{
	int argval;

	switch(option[1]) {
	case 'h':
		printf(help);
		break;

	case 'f':
		/* change format */
		if(!sscanf(arg, "%d", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change format */
			if(set_fmt(argval)) {
				printf("Failed to change format.\n");
			}
		}
		break;
	case 'v':
		/* set volume */
		if(!sscanf(arg, "%u", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change volume */
			if(set_volume(argval)) {
				printf("Failed to change rate.\n");
			}
		}		
		break;
	case 'r':
		/* change sample rate */
		if(!sscanf(arg, "%d", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change rate */
			if(set_sample_rate(argval)) {
				printf("Failed to change rate.\n");
			}
		}
		break;
	case 'c':
		/* change number of channels */
		if(!sscanf(arg, "%d", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change channels */
			if(set_channels(argval)) {
				printf("Failed to change channels.\n");
			}
		}
		break;
	case 'w':
		/* play wav file */
		if(!sscanf(arg, "%s", wave_file)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, play wave file */
			defer_play = 1;
		}
		break;
	case 'm':
		/* record wav file */
		if(!sscanf(arg, "%s", wave_file)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, record wave file */
			defer_rec = 1;
		}
		break;
	case 'b':
		/* change buffer size */
		/* This option is useful for testing to see if a
		 * driver properly supports various buffer sizes.
		 */
		if(!sscanf(arg, "%d", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change buffer size */
			buffer_size = argval;
		}
		break;
	case 's':
		/* change record time in seconds */
		if(!sscanf(arg, "%d", &argval)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, change seconds */
			seconds = argval;
		}
		break;
	case 't':
		/* run test */
		if(!sscanf(arg, "%s", command)) {
			printf("Invalid argument to option -%c\n", option[1]);
			return 1;
		} else {
			/* arg is good, run command */
			defer_command = 1;
		}
		break;
	case '\0':
		printf("Found - with no option.\n");
		return 1;
	default:
		printf("Unsupported option %c\n", option);
		break;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int argnum, arg;
	char option;
	
	/* default buffer size. */
	buffer_size = BUFFER_SIZE;

	fd = open("/dev/dsp", O_RDWR);
	if(fd == -1) {
		printf("Failed to open /dev/dsp\n");
		return;
	}

	if(argc == 1) {
		/* print driver state */
		print_state();
	} else {
		/* parse args */
		argnum = 1;
		while(argnum < argc) {
			if((argv[argnum][0] == '-') && ((argnum+1) < argc)) {
				if(handle_option(argv[argnum], argv[argnum+1])) {
					printf(help);
					return 1;
				}
				argnum+=2;
			} else {
				printf("Expected option and arg, found %s\n",
				       argv[argnum]);
				printf(help);
				return 1;
			}
		}
	}

	if(defer_play) {
		if(play_wave(wave_file))
			return 1;
	}
	if(defer_rec) {
		if(record_wave(wave_file))
			return 1;
	}
	if(defer_command)
		run_command();

	close(fd);
	return 0;

}
