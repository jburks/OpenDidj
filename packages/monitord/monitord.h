#ifndef MONITORD_H
#define MONITORD_H

/*
 * settings for device driver communication
 */

#define MAX_DEVNODES	8

/*
 * defaults
 */

/* watchdog feeding interval, in seconds */
#define WD_FEED_INTERVAL	10

/* application shutdown timers, in seconds */
#define USB_TIME_DEFAULT	6
#define POWER_TIME_DEFAULT	6
#define SYSTEM_TIME_DEFAULT	5

/*
 * messages for inter-process communication
 */

/* socket that monitord listens on */
#define MONITOR_SOCK	"/tmp/monitoring_socket"
/* socket on which monitord reports USB events */
#define USB_SOCK	"/tmp/usb_events_socket"
/* socketn on which monitord reports Power events */
#define POWER_SOCK	"/tmp/power_events_socket"

/* message types supported */
#define APP_MSG_GET_USB		0
#define APP_MSG_SET_USB		1
#define APP_MSG_GET_POWER	2
#define APP_MSG_SET_POWER	3
#define APP_MSG_GET_SYSTEM	4
#define APP_MSG_SET_SYSTEM	5

/* payload for monitor->app APP_MSG_SET_POWER message */
#define EVENT_POWER	1
#define EVENT_BATTERY	2

struct app_message {
	unsigned int type;
	unsigned int payload;
} __attribute__((__packed__));

#endif
