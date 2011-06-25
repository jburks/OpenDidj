/*
 * watchdog-ctl.c -- test watchdog
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#define WATCHDOG_DEVICE "/dev/watchdog"

int quiet_flag = 0;

struct option long_options[] =
{
	{"getTimeout", no_argument,	  0,		'g'},
	{"help",       no_argument,	  0,		'h'},
	{"quiet",      no_argument,	  &quiet_flag,	1  },
	{"setTimeout", required_argument, 0,		't'},
	{"status",     no_argument,	  0,		'S'},
	{"verbose",    no_argument,	  &quiet_flag,	0  },
	{0,            0,           	  0,		0  }
};

char *reasonToString(int status)
{
        switch(status) {
	        default:
			printf("%s.%d: unexpected value status=%d\n",
				__FUNCTION__, __LINE__, status);
			return("reasonToString: ERROR");
	        }
        return("state: UNKNOWN");
}

int getTimeout(void)
{
	int watchdog_fd;
	int status;
	int timeout;
	watchdog_fd = open(WATCHDOG_DEVICE, O_WRONLY);
	if (watchdog_fd < 0) {
		printf("Unable to open %s\n", WATCHDOG_DEVICE);
		exit(watchdog_fd);
	}
	status = ioctl(watchdog_fd, WDIOC_GETTIMEOUT, &timeout);
	if (status < 0) {
		printf("%s.%d, ioctl error: status = %d\n",
			__FUNCTION__, __LINE__, status);
		exit(watchdog_fd);
	}
	write(watchdog_fd,"V",1);	// signal normal watchdog close
	close(watchdog_fd);
	return(timeout);
}

int setTimeout(int timeInSecs)
{
	int watchdog_fd;
	int status;
	watchdog_fd = open(WATCHDOG_DEVICE, O_WRONLY);
	if (watchdog_fd < 0) {
		printf("Unable to open %s\n", WATCHDOG_DEVICE);
		exit(watchdog_fd);
	}
	status = ioctl(watchdog_fd, WDIOC_SETTIMEOUT, &timeInSecs);
	if (status < 0) {
		printf("%s.%d, ioctl error: status = %d\n",
			__FUNCTION__, __LINE__, status);
		exit(watchdog_fd);
	}
	write(watchdog_fd,"V",1);	// signal normal watchdog close
	close(watchdog_fd);
	return(0);
}

int status(void)
{
	int watchdog_fd;
	int status = 0;
	int result;
	watchdog_fd = open(WATCHDOG_DEVICE, O_WRONLY);
	if (watchdog_fd < 0) {
		printf("Unable to open %s\n", WATCHDOG_DEVICE);
		exit(watchdog_fd);
	}
	status = ioctl(watchdog_fd, WDIOC_GETSTATUS, &result);
	if (status < 0) {
		printf("%s.%d, ioctl error: result = %d\n",
			__FUNCTION__, __LINE__, result);
		exit(status);
	}
	write(watchdog_fd,"V",1);	// signal normal watchdog close
	close(watchdog_fd);
	return(result);
}

void showHelp(char *programName)
{
	printf("%s get/set watchdog settings, options:\n", programName);
	printf("  --getTimeout	get Watchdog timeout in Seconds\n");
	printf("  --help	this help info\n");
	printf("  --quiet	suppress text output, useful for scripting\n");
	printf("  --setTimeout  set Watchdog timeout value in Seconds\n");
	printf("  --status	get last shutdown state\n");
	printf("  --verbose	display text output\n");
}

int main (int argc, char **argv)
{
	int c;
	int timeInSecs;
	int retval = 0;

	if (argc < 2) {
		showHelp(argv[0]);		// show help info and exit
		exit(-1);
	}

	while (1)
	{
		int option_index = 0;
		c = getopt_long(argc, argv, "ghqSt:v",
			       	long_options, &option_index);

		if (c == -1) // end of the options.
			break;

		switch (c)
		{
			case 0:
				break;

			case 'g':
				timeInSecs = getTimeout();
				if (!quiet_flag)
					printf(
					"Timeout is %d seconds\n",
					timeInSecs);
				retval = timeInSecs;
				break;

			case 'h':
				showHelp(argv[0]);  // show help info and exit
				break;

			case 's':
				shutdown();
				break;

			case 'S':
				retval = status();
				if (!quiet_flag) {
					printf("Status = %s (%d)\n",
						reasonToString(retval), retval);
				}
				break;

			case 't':
				timeInSecs = atoi(optarg);
				setTimeout(timeInSecs);
				timeInSecs = getTimeout();
				if (!quiet_flag)
					printf(
					"Timeout is %d Seconds\n",
					timeInSecs);
				break;

			default:
				showHelp(argv[0]);
				exit(-1);
		}
	}
	return(retval);
}
