/* monitord -- Lightning System Monitoring Daemon
 * 
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * monitord is the user-space System Monitoring Daemon.  Its job is to pass
 * certain events up to the monitored application, and ensure that the 
 * application takes the appropriate action (shuts down) in the allowed time.
 * When the application fails to shut down in time, monitord kills it.  
 *
 * monitord monitors the state of USB connection (VBUS) and the power button by
 * talking to drivers.  The application may specify shutdown timeouts for the
 * USB and Power events, and it can disable the timeouts as well.
 */

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

#include "monitord.h"
#include "socket.h"

#ifdef DEBUG_PRINT
#define dbprintf(...)	fprintf(console, "monitord: " __VA_ARGS__)
FILE *console;	/* for debug printing */
#else
#define dbprintf(...)
#endif

#define POLL_TIMEOUT	500
#define WF_FEED_COUNT	((WD_FEED_INTERVAL*1000)/POLL_TIMEOUT)

/* we maintain a one-shot timer, which may be set for either the following
 * types of events: */
typedef enum {
	USB_TIMER 	= 0,	/* USB vbus event */
	POWER_TIMER	= 1,	/* power button has been pressed */
	SYSTEM_TIMER	= 2,	/* system shutdown timer */
} app_timer_t;

static struct {
	unsigned running		: 1;
	unsigned app_timer_armed	: 1;
	unsigned watchdog		: 1;

	unsigned int usb_time, power_time, system_time;
	app_timer_t app_timer_type;
	timer_t app_timer;
	struct itimerspec app_timeout;
} monitor;

/*
 * set and arm the Application Timer
 */
int set_app_timer(unsigned int sec, unsigned int nsec, app_timer_t type)
{
	monitor.app_timeout.it_value.tv_sec = sec;
	monitor.app_timeout.it_value.tv_nsec = nsec;
	monitor.app_timeout.it_interval.tv_sec = 1;
	monitor.app_timeout.it_interval.tv_nsec = 0;
	monitor.app_timer_type = type;

	/* if we set a 0 timeout, then we're actually disarming the timer,
	 * otherwise we're arming it */
	monitor.app_timer_armed = (sec > 0 || nsec > 0);

	return timer_settime(monitor.app_timer, 0, &monitor.app_timeout, NULL);
}

/*
 * stop the Application Timer
 */
void stop_app_timer(void)
{
	monitor.app_timeout.it_value.tv_sec = 0;
	monitor.app_timeout.it_value.tv_nsec = 0;
	timer_settime(monitor.app_timer, 0, &monitor.app_timeout, NULL);
	monitor.app_timer_armed = 0;
}

/*
 * try to send a message to the application
 */
int send_message(struct app_message *msg, const char *path)
{
	int s;
	
	s = create_report_socket(path);
	if(s < 0)
		return s;

	if(send(s, msg, sizeof(struct app_message), 0) == -1) {
		dbprintf("failed to send message\n");
		return -1;
	}

	close(s);
	return 0;
}

/*
 * Application one-shot timer has expired.  Disarm the timer, kill the 
 * Application (if it's still around), and take action based on the type of
 * timer we had set.
 */
void event_timer_expired(void)
{
	stop_app_timer();
	system("killmainapp");

	switch(monitor.app_timer_type) {
		case POWER_TIMER:
		dbprintf("power timer expired\n");	
		/* arm the system timer */
		if(set_app_timer(monitor.system_time, 0, SYSTEM_TIMER) < 0) {
			dbprintf("failed to set system timer\n");
		}
		break;

		case USB_TIMER:
		dbprintf("usb timer expired\n");
		system("usbctl -d mass_storage -a enable");
		break;

		case SYSTEM_TIMER:
		dbprintf("system timer expired\n");
		system("poweroff");
		break;
	}
}

/*
 * handle Power Button
 */
void event_power(int s, struct sockaddr_un *sa, unsigned int code)
{
	struct app_message msg;

	/* tell the Application that the power button was pushed (and that it
	 * should shut down) */

	msg.type = APP_MSG_SET_POWER;
	switch(code) {
		case KEY_POWER:
		msg.payload = EVENT_POWER;
		break;
		case KEY_BATTERY:
		msg.payload = EVENT_BATTERY;
		break;
		default:
		msg.payload = 0;
		break;
	}

	if(send_message(&msg, POWER_SOCK) < 0) {
		dbprintf("unable to send Power Button to application\n");
	}

	/* arm the Application Timer */

	if(!monitor.app_timer_armed) {
		if(set_app_timer(monitor.power_time, 0, POWER_TIMER) < 0)
			dbprintf("failed to set app timer\n");
	}
}

/*
 * handle a change in the USB Vbus
 */
void event_vbus(unsigned int value, int s, struct sockaddr_un *sa)
{
	struct app_message msg;

	dbprintf("vbus is %s\n", value == 0 ? "low" : "high");

	/* tell the Application about the Vbus change */

	msg.type = APP_MSG_SET_USB;
	msg.payload = !!value;

	if(send_message(&msg, USB_SOCK) < 0) {
		dbprintf("unable to send VBUS state to application\n");
	}

	/* if Vbus went high (ie: the cable got plugged in), the Application
	 * should shut down.  We set a timer after which we'll kill the 
	 * Application */

	if(value != 0 && !monitor.app_timer_armed) { 
		if(set_app_timer(monitor.usb_time, 0, USB_TIMER) < 0)
			dbprintf("failed to set app timer\n");
	}
}

/*
 * handle a message from the application
 */
void event_app_message(struct app_message *msg, int s)
{
	int ret;
	struct app_message resp;

	switch(msg->type) {
		case APP_MSG_GET_USB:
		dbprintf("app asked for value of USB timer\n");
		resp.type = APP_MSG_GET_USB;
		resp.payload = monitor.usb_time;	
		ret = send(s, &resp, sizeof(struct app_message), MSG_NOSIGNAL);
		if(ret < 0) {
			dbprintf("can't send response\n");
		}
		break;

		case APP_MSG_SET_USB:
		dbprintf("app set usb timer to %d\n", msg->payload);
		monitor.usb_time = msg->payload;
		/* if a USB timer was already running, start a new timer with
		 * the new timeout value */
		if(monitor.app_timer_armed && 
			monitor.app_timer_type == USB_TIMER) {
			stop_app_timer();
			set_app_timer(msg->payload, 0, USB_TIMER);
		}
		break;

		case APP_MSG_GET_POWER:
		dbprintf("app asked for value of Power timer\n");
		resp.type = APP_MSG_GET_POWER;
		resp.payload = monitor.power_time;
		ret = send(s, &resp, sizeof(struct app_message), MSG_NOSIGNAL);
		if(ret < 0) {
			dbprintf("can't send response\n");
		}
		break;

		case APP_MSG_SET_POWER:
		dbprintf("app set power timer to %d\n", msg->payload);
		monitor.power_time = msg->payload;
		/* if a Power timer was already running, start a new timer with
		 * the new timeout value */
		if(monitor.app_timer_armed && 
			monitor.app_timer_type == POWER_TIMER) {
			stop_app_timer();
			set_app_timer(msg->payload, 0, POWER_TIMER);
		}
		break;

		case APP_MSG_GET_SYSTEM:
		dbprintf("app asked for value of System timer\n");
		resp.payload = monitor.system_time;
		ret = send(s, &resp, sizeof(struct app_message), MSG_NOSIGNAL);
		if(ret < 0) {
			dbprintf("can't send response\n");
		}
		break;

		case APP_MSG_SET_SYSTEM:
		dbprintf("app set system timer to %d\n", msg->payload);
		monitor.system_time = msg->payload;
		/* if a System timer was already running, start a new timer 
		 * with the new timeout value */
		if(monitor.app_timer_armed && 
			monitor.app_timer_type == SYSTEM_TIMER) {
			stop_app_timer();
			set_app_timer(msg->payload, 0, SYSTEM_TIMER);
		}
		break;

		default:
		dbprintf("got unknown message type %d\n", msg->type);
		break;
	}
}

/*
 * daemonize our process by losing the controlling shell and disconnecting from
 * the file descriptors
 */
#ifndef DEBUG_NO_DAEMON
void daemonize(const char *cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit r1;
	struct sigaction sact;

	umask(0);

	if(getrlimit(RLIMIT_NOFILE, &r1) < 0) {
		exit(1);
	}

	/*
	 * become a session leader to lose controlling TTY
	 */

	if((pid = fork()) < 0) {
		exit(1);
	}
	else if(pid != 0) /* parent */
		exit(0);
	setsid();

	/*
	 * ensure future opens won't allocate controlling TTYs
	 */

	sact.sa_handler = SIG_IGN;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	if(sigaction(SIGHUP, &sact, NULL) == -1) {
		exit(1);
	}
	if((pid = fork()) < 0) {
		exit(1);
	}
	else if(pid != 0) /* parent */
		exit(0);

	/*
	 * change current working directory to the root so that we won't
	 * prevent file systems from being unmounted
	 */

	if(chdir("/") < 0) {
		exit(1);
	}

	/*
	 * close all open file descriptors
	 */
	if(r1.rlim_max == RLIM_INFINITY)
		r1.rlim_max = 20; /* XXX */
	for(i = 0; i < r1.rlim_max; i++)
		close(i);

	/*
	 * attach file descriptors 0, 1, 2 to /dev/null
	 */

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	/*
	 * initiate log file
	 */

	if(fd0 != 0 || fd1 != 1 || fd2 != 2) {
		exit(1);
	}
}
#endif /* DEBUG_NO_DAEMON */

/*
 * find and open an input device by name
 */
int open_input_device(char *input_name)
{
	char dev[20];
	char name[32];
	int fd, i;

	for(i = 0; i < MAX_DEVNODES; i++) {
		sprintf(dev, "/dev/input/event%d", i);
		fd = open(dev, O_RDONLY);
		if(fd < 0) {
			return 1;
		}

		if(ioctl(fd, EVIOCGNAME(32), name) < 0) {
			close(fd);
			return 1;
		}

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

/*
 * tell the watchdog that we're going away
 */
void watchdog_shutdown(int wd)
{
	write(wd, "V", 1); /* see watchdog-api.txt in kernel documentation */
#ifdef WATCHDOG_CLEANUP
	/* by closing the file, we stop the watchdog from shutting down the
	 * system after we've stopped feeding it */
	close(wd);
#endif /* WATCHDOG_CLEANUP */
}

int no_usb_timer_mode(void)
{
	char buf[32];
	int size;
	int fd = open("/flags/usb_mass_storage", O_RDONLY);

	if(fd < 0)
		return 0;

	size = read(fd, buf, 31);
	close(fd);

	if(size > 0 && !strncmp(buf, "NOWATCHDOG", 10))
		return 1;

	return 0;
}

/*
 * handle signals
 */
void handle_signal(int sig)
{
	switch(sig) {
		/* the daemon needs to shut down */
		case SIGTERM:
		case SIGINT:
		monitor.running = 0;
		break;

		/* the Application Timer has expired */
		case SIGALRM:
		event_timer_expired();
		break;

		default:
		dbprintf("unknown signal\n");
		break;
	}
}

#define FD_USB	0
#define FD_PWR	1

int main(int argc, char **argv)
{
	int ret, size;
	int fd_usb, fd_pwr, fd_wd;
	struct app_message msg;
	struct input_event ev;
	struct sockaddr_un app;
	struct sockaddr_un sa_usb, sa_pwr;
	socklen_t s_app = sizeof(struct sockaddr_un);
	struct pollfd fds[2];
	int num_fds = 0;
	int ls, ms, as_usb, as_pwr;
	struct sigaction sa_int;
	struct sigevent evp;
#ifdef KERNEL_WATCHDOG
	int wd_count = 0;
#endif

	/* 
	 * initialize our state
	 */

	memset(&monitor, 0, sizeof(monitor));
	monitor.power_time = POWER_TIME_DEFAULT;
	monitor.usb_time = no_usb_timer_mode() ? 0 : USB_TIME_DEFAULT;
	monitor.system_time = SYSTEM_TIME_DEFAULT;

#ifndef DEBUG_NO_DAEMON
	daemonize(argv[0]);	
#endif

#ifdef DEBUG_PRINT
	console = fopen("/dev/console", "w");
	if(console == NULL)
		exit(1);
#endif
	/* 
	 * grab the USB input device so we can monitor VBUS 
	 */

	fd_usb = open_input_device("LF1000 USB");
	if(fd_usb < 0) {
		dbprintf("can't open USB input device\n");
		goto fail_usb;
	}
	fds[FD_USB].fd = fd_usb;
	fds[FD_USB].events = POLLIN;
	num_fds++;

	/* 
	 * grab the Power Button input device 
	 */

	fd_pwr = open_input_device("Power Button");
	if(fd_pwr < 0) {
		dbprintf("can't open power button input device\n");
		goto fail_pwr;
	}
	fds[FD_PWR].fd = fd_pwr;
	fds[FD_PWR].events = POLLIN;
	num_fds++;

	/*
	 * set up a socket for receiving application messages
	 */

	ls = create_listening_socket(MONITOR_SOCK);
	if(ls < 0) {
		dbprintf("can't make listening socket\n");
		goto fail_ls;
	}
	fcntl(ls, F_SETFL, O_NONBLOCK);

	/*
	 * trap SIGTERM so that we clean up before exiting
	 */

	monitor.running = 1;
        sigemptyset(&sa_int.sa_mask);
	sa_int.sa_handler = handle_signal;
	sa_int.sa_flags = 0;
	if(sigaction(SIGTERM, &sa_int, NULL) == -1) {
		dbprintf("can't trap SIGTERM\n");
		goto fail_sig;
	}
#ifdef DEBUG_NO_DAEMON
	if(sigaction(SIGINT, &sa_int, NULL) == -1) {
		dbprintf("can't trap SIGINT\n");
		goto fail_sig;
	}
#endif

	/*
	 * create Application Timer
	 */

	evp.sigev_signo = SIGALRM;
	evp.sigev_notify = SIGEV_SIGNAL;

	ret = timer_create(CLOCK_REALTIME, &evp, &monitor.app_timer);
	if(ret < 0) {
		dbprintf("can't create app timer\n");
		goto fail_timer;
	}
	if(sigaction(SIGALRM, &sa_int, NULL) == -1) {
		dbprintf("can't register handler for app timer\n");
		timer_delete(monitor.app_timer);
		goto fail_timer;
	}

	/*
	 * set up the kernel-side watchdog
	 */

#ifdef KERNEL_WATCHDOG
	fd_wd = open("/dev/watchdog", O_WRONLY);
	if(fd_wd >= 0)
		monitor.watchdog = 1;
	else
		dbprintf("can't open /dev/watchdog\n");
#endif /* KERNEL_WATCHDOG */

	/*
	 * monitor
	 */

	while(monitor.running) {
#ifdef KERNEL_WATCHDOG
		/* feed the kernel-side watchdog */
		if(monitor.watchdog && ++wd_count >= WF_FEED_COUNT) {
			write(fd_wd, "", 1);
			wd_count = 0;
		}
#endif /* KERNEL_WATCHDOG */

		/* handle messages from the Application */
		ms = accept(ls, (struct sockaddr *)&app, &s_app);
		if(ms > 0) { /* there are pending connections */
			while(1) { /* read messages */
				size = recv(ms, &msg, sizeof(msg), 0);
				if(size == sizeof(msg))
					event_app_message(&msg, ms);
				else
					break;
			}
			close(ms);
		}

		/* monitor our file descriptors for activity */
		ret = poll(fds, num_fds, POLL_TIMEOUT);
		if(ret > 0) {
			/* did we get a USB event? */
			if(fds[FD_USB].revents & POLLIN) {
				dbprintf("USB event\n");
				size = read(fd_usb, &ev, sizeof(ev));
				/* USB vbus switch event */
				if(ev.type == EV_SW && ev.code == SW_LID)
					event_vbus(ev.value, as_usb, &sa_usb);
			}
			/* did we get a Power event? */
			if(fds[FD_PWR].revents & POLLIN) {
				dbprintf("Power Button event\n");
				size = read(fd_pwr, &ev, sizeof(ev));
				event_power(as_pwr, &sa_pwr, ev.code);
			}
		}
	}

	dbprintf("exiting...\n");

	timer_delete(monitor.app_timer);
	close(ls);
	close(fd_pwr);
	close(fd_usb);
	remove(MONITOR_SOCK);
#ifdef DEBUG_PRINT
	fclose(console);
#endif
	if(monitor.watchdog)
		watchdog_shutdown(fd_wd);
	exit(0);

fail_timer:
fail_sig:
	close(ls);
	remove(MONITOR_SOCK);
fail_ls:
	close(fd_usb);
fail_pwr:
	close(fd_pwr);
fail_usb:
#ifdef DEBUG_PRINT
	fclose(console);
#endif
	exit(1);
}
