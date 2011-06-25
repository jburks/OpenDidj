/* monitor-ctl.c - User interface for monitord
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * This is a simple user interface for inspecting and/or modifying monitord's 
 * configuration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "monitord.h"
#include "socket.h"

enum timer_type {
	UNKNOWN	= 0,
	USB	= 1,
	POWER	= 2,
	SYSTEM	= 3,
};

int set_timer_value(enum timer_type type, unsigned int val)
{
	int sock, ret;
	struct app_message msg;

	switch(type) {
		case USB:
		msg.type = APP_MSG_SET_USB;
		break;
		case POWER:
		msg.type = APP_MSG_SET_POWER;
		break;
		case SYSTEM:
		msg.type = APP_MSG_SET_SYSTEM;
		break;
		case UNKNOWN:
		default:
		return -1;
	}

	msg.payload = val;

	sock = create_report_socket(MONITOR_SOCK);
	if(sock < 0) {
		perror("report socket");
		return -1;
	}

	ret = send(sock, &msg, sizeof(msg), 0);
	if(ret < 0) {
		perror("send message");
		close(sock);
		return ret;
	}

	close(sock);
	return 0;
}

int get_timer_value(enum timer_type type)
{
	struct app_message msg, resp;
	int sock;
	int ret, size;

	/* make a request */
	
	switch(type) {
		case USB:
		msg.type = APP_MSG_GET_USB;
		break;
		case POWER:
		msg.type = APP_MSG_GET_POWER;
		break;
		case SYSTEM:
		msg.type = APP_MSG_GET_SYSTEM;
		break;
		case UNKNOWN:
		default:
		return -1;
	}
	msg.payload = 0;

	sock = create_report_socket(MONITOR_SOCK);
	if(sock < 0) {
		perror("report socket");
		return -1;
	}

	ret = send(sock, &msg, sizeof(msg), 0);
	if(ret < 0) {
		perror("send message");
		close(sock);
		return ret;
	}

	/* get the response */

	size = recv(sock, &resp, sizeof(resp), 0);
	if(size < 0) {
		perror("receive");
		close(sock);
		return ret;
	}

	if(size == sizeof(resp))
		return resp.payload;

	return -1;
}

static void print_usage(const char *name)
{
	printf("usage: %s <get|set> <usb|power|system> [sec]\n", name);
}

enum timer_type get_timer_type(const char *cmd)
{
	if(!strncmp(cmd, "usb", 3))
		return USB;
	if(!strncmp(cmd, "power", 5))
		return POWER;
	if(!strncmp(cmd, "system", 6))
		return SYSTEM;
	return UNKNOWN;
}

int main(int argc, char **argv)
{
	enum timer_type type;
	unsigned int sec;

	if(argc < 3) {
		print_usage(argv[0]);
		return 0;
	}

	type = get_timer_type(argv[2]);
	if(type == UNKNOWN) {
		printf("invalid option\n");
		print_usage(argv[0]);
		return 1;
	}

	if(!strncmp(argv[1], "get", 3)) {
		sec = get_timer_value(type);
		if(sec >= 0) {
			printf("%s timer: %d\n", argv[2], sec);
			return 0;
		}
		else {
			printf("error: got invalid response\n");
			return 1;
		}
	}
	else if(!strncmp(argv[1], "set", 3)) {
		if(argc < 4) {
			printf("specify a time in seconds\n");
			return 1;
		}
		sec = atoi(argv[3]);
		if(set_timer_value(type, sec) != 0) {
			printf("error: failed to set timer\n");
			return 1;
		}
	}
	else {
		printf("invalid option\n");
		print_usage(argv[0]);
		return 1;
	}

	return 0;
}
