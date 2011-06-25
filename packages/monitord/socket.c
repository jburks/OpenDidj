#include "socket.h"

/*
 * set up a connected socket for sending messages
 */
int create_report_socket(const char *path)
{
	int s;
	int ret;
	struct sockaddr_un sa;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s == -1)
		return s;

	memset(&sa, 0, sizeof(struct sockaddr_un));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, strlen(path));

	ret = connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
	if(ret < 0) {
		close(s);
		return ret;
	}

	return s;
}

/*
 * open and bind a listening socket
 */
int create_listening_socket(const char *path)
{
	struct sockaddr_un sa;
	int ls, ret;
      	struct stat st;

	ls = socket(AF_UNIX, SOCK_STREAM, 0);
	if(ls == -1)
		return ls;

	if(stat(path, &st) == 0) /* file exists */
		remove(path);

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, strlen(path));
	ret = bind(ls, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
	if(ret < 0) {
		close(ls);
		return ret;
	}

	ret = listen(ls, 20);
	if(ret < 0) {
		close(ls);
		unlink(path);
		return ret;
	}

	return ls;
}
