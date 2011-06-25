#ifndef MONITORD_SOCKET_H
#define MONITORD_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

int create_report_socket(const char *path);
int create_listening_socket(const char *path);

#endif /* MONITORD_SOCKET_H */
