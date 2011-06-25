/* test_monitord.c - Application-side tester for monitord
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * This test's job is to pretend to be a monitored application's USB or Power
 * thread by:
 * - asking for the current shutdown timer
 * - setting the app shutdown timer
 * - waiting for monitord events and taking (or not taking) some action.
 *
 * TODO: - add command line arguments for deciding whether to behave or
 * 	   misbehave.
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
#include <linux/input.h>

#include "monitord.h"
#include "socket.h"

enum {
	USB	= 0,
	POWER	= 1,
} test_mode;

void handle_message(struct app_message *msg)
{
	printf("type %d, payload %d\n", msg->type, msg->payload);
}

const char *get_path(void)
{
	if(test_mode == USB)
		return USB_SOCK;
	return POWER_SOCK;
}

void handle_monitor_message(struct app_message *msg)
{
	switch(msg->type) {
		case APP_MSG_SET_USB:
		printf("got USB message, payload: %d\n", msg->payload);
		break;
		case APP_MSG_SET_POWER:
		printf("got Power message, payload: %d\n", msg->payload);
		break;
		default:
		printf("got unknown message type %d\n", msg->type);
		break;
	}
}

int get_timer_value(void)
{
	struct app_message msg, resp;
	int sock;
	int ret, size;

	sock = create_report_socket(MONITOR_SOCK);
	if(sock < 0) {
		perror("report socket");
		return sock;
	}

	msg.type = test_mode == USB ? APP_MSG_GET_USB : APP_MSG_GET_POWER;
	ret = send(sock, &msg, sizeof(msg), 0);
	if(ret < 0) {
		perror("send message");
		close(sock);
		return ret;
	}
	
	size = recv(sock, &resp, sizeof(struct app_message), 0);
	if(size < 0) {
		perror("receive");
		close(sock);
		return size;
	}

	if(size == sizeof(struct app_message) && resp.type == msg.type)
		printf("current timer is: %d\n", resp.payload);
	else
		printf("got unexpected response\n");
	
	close(sock);
	return resp.payload;
}

int set_timer_value(unsigned int val)
{
	int sock, ret;
	struct app_message msg;

	sock = create_report_socket(MONITOR_SOCK);
	if(sock < 0) {
		perror("report socket");
		return sock;
	}

	printf("setting timer...\n");

	msg.type = test_mode == USB ? APP_MSG_SET_USB : APP_MSG_SET_POWER;
	msg.payload = 5;

	ret = send(sock, &msg, sizeof(struct app_message), MSG_NOSIGNAL);
	if(ret < 0)
		perror("send");

	close(sock);
	return 0;
}

int main(int argc, char **argv)
{
	int sock, ls, ms;
	int size, cur, ret;
	struct sockaddr_un mon;
	socklen_t s_mon = sizeof(mon);
	struct app_message msg;
	int running = 1;

	/*
	 * check arguments
	 */

	if(argc < 2) {
		printf("usage: %s <mode>\n\nmodes: 'usb' or 'power'\n", 
				argv[0]);
		return 1;
	}
	if(!strncmp(argv[1], "usb", 3))
		test_mode = USB;
	else if(!strncmp(argv[1], "power", 5))
		test_mode = POWER;
	else {
		printf("invalid test mode\n");
		return 1;
	}

	ls = create_listening_socket(get_path());
	if(ls < 0) {
		perror("listening socket");
		exit(1);
	}
	fcntl(ls, F_SETFL, O_NONBLOCK);

	cur = get_timer_value();
	assert(cur >= 0);

	ret = set_timer_value(5);
	assert(ret == 0);

	cur = get_timer_value();
	assert(cur == 5);

	/*
	 * wait for messages from the monitor
	 */
	
	printf("listening...\n");
	while(running) {
		ms = accept(ls, (struct sockaddr *)&mon, &s_mon);
		if(ms > 0) {
			while(1) { /* receive monitor message(s) */
				size = recv(ms, &msg, 
						sizeof(struct app_message), 0);
				if(size == sizeof(struct app_message))
					handle_monitor_message(&msg);
				else
					break;
			}
			close(ms);
		}

		/* introduce some delay... */
		usleep(1000*100);
	}

	close(sock);
	return 0;
}
