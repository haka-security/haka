/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <haka/error.h>
#include <haka/log.h>
#include <haka/types.h>

#include <haka/luadebug/user.h>

#define MODULE    SECTION_REMOTE


struct luadebug_remote_user {
	struct luadebug_user   user;
	mutex_t                active;
	int                    fd;
	bool                   error;
};

static int read_int(int fd)
{
	int32 len;
	if (read(fd, &len, sizeof(len)) != sizeof(len)) {
		return -1;
	}
	return SWAP_FROM_LE(int32, len);
}

static char *read_string(int fd)
{
	const uint32 len = read_int(fd);
	if (len > 0) {
		char *ret = malloc(len);
		if (!ret) {
			return NULL;
		}

		if (read(fd, ret, len) != len ||
			ret[len-1] != 0) {
			free(ret);
			return NULL;
		}

		return ret;
	}
	else {
		return NULL;
	}
}

static bool write_int(int fd, int i)
{
	const uint32 lei = SWAP_TO_LE(int32, i);
	if (write(fd, &lei, sizeof(lei)) != sizeof(lei)) {
		return false;
	}
	return true;
}

static bool write_string(int fd, const char *string)
{
	if (string) {
		const uint32 len = strlen(string)+1;
		if (!write_int(fd, len) || write(fd, string, len) != len) {
			return false;
		}
	}
	else {
		const uint32 len = 0;
		if (!write_int(fd, len)) {
			return false;
		}
	}

	return true;
}

static void report_error(struct luadebug_remote_user *user, int err)
{
	if (!user->error) {
		LOG_ERROR(MODULE, "remote communication error: %s", errno_error(err));
		user->error = true;
	}
}

static bool start(struct luadebug_user *_user, const char *name)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	mutex_lock(&user->active);

	if (write(user->fd, "s", 1) != 1 ||
		!write_string(user->fd, name)) {
		report_error(user, errno);
		return false;
	}

	return true;
}

static char *my_readline(struct luadebug_user *_user, const char *prompt)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	if (write(user->fd, "r", 1) != 1 ||
		!write_string(user->fd, prompt)) {
		report_error(user, errno);
		return NULL;
	}

	while (true) {
		char command;
		const int rc = read(user->fd, &command, 1);

		if (rc != 1) {
			if (rc == -1 && errno != EINTR) {
				report_error(user, errno);
				return NULL;
			}
		}
		else if (command == 'c') {
			/* completion request */
			generator_callback *generator = NULL;
			char *line;
			char *res;
			int state = 0;
			int start;

			start = read_int(user->fd);
			line = read_string(user->fd);
			if (start == -1 || !line) {
				report_error(user, errno);
				return NULL;
			}

			if (_user->completion) {
				generator = _user->completion(line, start);
			}
			free(line);

			line = read_string(user->fd);
			if (!line) {
				report_error(user, errno);
				return NULL;
			}

			if (generator) {
				while ((res = generator(line, state++))) {
					if (!write_string(user->fd, res)) {
						report_error(user, errno);
						return NULL;
					}
				}
			}

			if (!write_string(user->fd, NULL)) {
				report_error(user, errno);
				return NULL;
			}
		}
		else if (command == '1') {
			break;
		}
		else {
			report_error(user, errno);
			return NULL;
		}
	}

	return read_string(user->fd);
}

static void addhistory(struct luadebug_user *_user, const char *line)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	if (write(user->fd, "h ", 1) != 1 ||
		!write_string(user->fd, line)) {
		report_error(user, errno);
	}
}

static bool stop(struct luadebug_user *_user)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	if (user->error) {
		return false;
	}

	if (write(user->fd, "e", 1) != 1) {
		report_error(user, errno);
		return false;
	}

	mutex_unlock(&user->active);

	return true;
}

static void print(struct luadebug_user *_user, const char *format, ...)
{
	va_list args;
	char *line;
	int err = 0;
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	va_start(args, format);
	err = vasprintf(&line, format, args);
	va_end(args);

	if (!line || err < 0) {
		return;
	}

	if (write(user->fd, "p", 1) != 1 ||
		!write_string(user->fd, line)) {
		report_error(user, errno);
	}

	free(line);
}

static bool check(struct luadebug_user *_user)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;

	if (user->error) {
		return false;
	}

	if (write(user->fd, "P", 1) != 1) {
		return false;
	}
	return true;
}

static void destroy(struct luadebug_user *_user)
{
	struct luadebug_remote_user *user = (struct luadebug_remote_user*)_user;
	close(user->fd);
	mutex_destroy(&user->active);
	free(user);
}

struct luadebug_user *luadebug_user_remote(int fd)
{
	struct luadebug_remote_user *ret = malloc(sizeof(struct luadebug_remote_user));
	if (!ret) {
		error("memory error");
		return NULL;
	}

	luadebug_user_init(&ret->user);
	ret->user.start = start;
	ret->user.readline = my_readline;
	ret->user.addhistory = addhistory;
	ret->user.stop = stop;
	ret->user.print = print;
	ret->user.check = check;
	ret->user.destroy = destroy;
	ret->fd = fd;
	ret->error = false;
	mutex_init(&ret->active, true);

	return &ret->user;
}

static int server_completion_fd = -1;

char *remote_generator(const char *text, int state)
{
	if (state == 0) {
		write_string(server_completion_fd, text);
	}

	return read_string(server_completion_fd);
}

generator_callback *remote_completion(const char *line, int start)
{
	if (write(server_completion_fd, "c", 1) != 1 ||
		!write_int(server_completion_fd, start) ||
		!write_string(server_completion_fd, line)) {
		return NULL;
	}

	return remote_generator;
}

static bool luadebug_user_remote_server_session(int fd, struct luadebug_user *user)
{
	char command;

	server_completion_fd = fd;
	user->completion = remote_completion;

	while ((read(fd, &command, 1) == 1)) {
		switch (command) {
		case 'e':
		{
			user->stop(user);
			return true;
		}

		case 'p':
		{
			char *line = read_string(fd);
			if (!line) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				return false;
			}

			user->print(user, line);
			free(line);
			break;
		}

		case 'h':
		{
			char *line = read_string(fd);
			if (!line) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				return false;
			}

			user->addhistory(user, line);
			free(line);
			break;
		}

		case 'r':
		{
			char *line = read_string(fd);
			if (!line) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				return false;
			}

			char *rdline = user->readline(user, line);
			if (!rdline) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				free(line);
				return false;
			}

			free(line);

			command = '1';
			if (write(fd, &command, 1) != 1) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				return false;
			}
			write_string(fd, rdline);
			free(rdline);
			break;
		}

		case 'P':
		{
			break;
		}

		default:
			LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
			return false;
		}
	}

	return false;
}

void luadebug_user_remote_server(int fd, struct luadebug_user *user)
{
	char command;
	while ((read(fd, &command, 1) == 1)) {
		switch (command) {
		case 's':
		{
			char *line = read_string(fd);
			if (!line) {
				LOG_ERROR(MODULE, "remote communication error: %s", errno_error(errno));
				return;
			}

			user->start(user, line);
			free(line);

			if (!luadebug_user_remote_server_session(fd, user)) {
				LOG_ERROR(MODULE, "remote communication error");
				return;
			}
			break;
		}

		case 'P':
		{
			break;
		}

		default:
			LOG_ERROR(MODULE, "remote communication error");
			return;
		}
	}

	luadebug_user_release(&user);
}
