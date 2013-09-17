
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>

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

bool ctl_print(int fd)
{
	char buffer[1024];
	int len;

	do {
		len = recv(fd, buffer, 1024, 0);
		if (len < 0) {
			printf("\n");
			fprintf(stderr, "cannot read from ctl socket: %i, %i %s\n", len , errno, strerror(errno));
			return false;
		}

		printf("%.*s", len, buffer);
	} while (len > 0);

	printf("\n");
	return true;
}

bool ctl_check(int fd, const char *str)
{
	char c;
	int len;
	bool ret = true;
	const char *iter = str;

	do {
		len = recv(fd, &c, 1, 0);
		if (len < 0) {
			return false;
		}
		else if (len == 0) {
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
