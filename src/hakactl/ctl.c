
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>

#include "ctl.h"
#include "config.h"


int ctl_open_socket()
{
	int fd;
	struct sockaddr_un addr;
	socklen_t len;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "cannot create ctl socket: %s\n", strerror(errno));
		return -2;
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, HAKA_CTL_SOCKET_FILE);

	len = strlen(addr.sun_path) + sizeof(addr.sun_family);
	if (connect(fd, (struct sockaddr *)&addr, len)) {
		return -1;
	}

	return fd;
}

bool ctl_send(int fd, const char *command)
{
	const size_t len = strlen(command);

	if (send(fd, command, len+1, 0) < 0) {
		fprintf(stderr, "cannot write on ctl socket: %s\n", strerror(errno));
		return false;
	}

	return true;
}

static int ctl_next_char(int fd)
{
	int len;
	char c;

	len = recv(fd, &c, 1, 0);
	if (len < 0) {
		return -1;
	}
	else if (len == 0) {
		return -1;
	}

	return c;
}

static int ctl_next_wchar(int fd)
{
	int len;
	wchar_t c;

	len = recv(fd, &c, sizeof(wchar_t), 0);
	if (len <= 0) {
		return -1;
	}

	return c;
}

bool ctl_receive_chars(int fd, char *buffer, size_t len)
{
	while (true) {
		const int c = ctl_next_char(fd);
		if (c < 0) {
			return false;
		}

		if (len > 0) {
			len--;
			if (len > 0) {
				*buffer++ = c;
			}
			else {
				*buffer = 0;
			}
		}

		if (c == 0) {
			break;
		}
	}

	return true;
}

bool ctl_receive_wchars(int fd, wchar_t *buffer, size_t len)
{
	while (true) {
		const int c = ctl_next_wchar(fd);
		if (c < 0) {
			return false;
		}

		if (len > 0) {
			len--;
			if (len > 0) {
				*buffer++ = c;
			}
			else {
				*buffer = 0;
			}
		}

		if (c == 0) {
			break;
		}
	}

	return true;
}

bool ctl_check(int fd, const char *str)
{
	int c;
	bool ret = true;
	const char *iter = str;

	do {
		c = ctl_next_char(fd);
		if (c < 0) {
			return false;
		}

		if (iter) {
			if (*iter != c) {
				ret = false;
			}

			if (*iter == 0) {
				iter = NULL;
			}
			else {
				++iter;
			}
		}
	} while (c != 0);

	return ret;
}
