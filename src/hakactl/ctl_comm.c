/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <errno.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <haka/error.h>
#include <haka/types.h>

#include "ctl_comm.h"

#define SIZE 1024


static bool ctl_check_error(int err, int expected, bool forread)
{
	if (err == 0 && forread) {
		error(L"end of file");
		return false;
	}
	else if (err <= 0) {
		error(L"%s", errno_error(errno));
		return false;
	}
	else if (err != expected) {
		error(L"communication error");
		return false;
	}
	return true;
}

bool ctl_send_chars(int fd, const char *str, size_t len)
{
	if (len == (size_t)-1) len = strlen(str);

	if (!ctl_send_int(fd, len)) {
		return false;
	}

	if (!ctl_check_error(write(fd, str, len), len, false)) {
		return false;
	}

	return true;
}

bool ctl_send_wchars(int fd, const wchar_t *str, size_t len)
{
	if (len == (size_t)-1) len = wcslen(str);

	if (!ctl_send_int(fd, len)) {
		return false;
	}

	while (*str) {
		const wchar_t c = SWAP_TO_LE(wchar_t, *str++);
		if (!ctl_check_error(write(fd, &c, sizeof(c)), sizeof(c), false)) {
			return false;
		}
	}

	return true;
}

bool ctl_send_int(int fd, int32 i)
{
	const int32 bi = SWAP_TO_LE(int32, i);
	if (!ctl_check_error(write(fd, &bi, sizeof(bi)), sizeof(bi), false)) {
		return false;
	}
	return true;
}

char *ctl_recv_chars(int fd, size_t *_len)
{
	char *str;
	const int32 len = ctl_recv_int(fd);
	if (check_error()) {
		return NULL;
	}

	str = malloc(len+1);
	if (!str) {
		error(L"memory error");
		return NULL;
	}

	if (!ctl_check_error(read(fd, str, len), len, true)) {
		return NULL;
	}

	if (_len) *_len = len;

	str[len] = 0;
	return str;
}

wchar_t *ctl_recv_wchars(int fd, size_t *_len)
{
	wchar_t *str;
	int i;
	const int32 len = ctl_recv_int(fd);
	if (check_error()) {
		return NULL;
	}

	if (len < 0) {
		error(L"communication error");
		return NULL;
	}

	str = malloc(sizeof(wchar_t)*(len+1));
	if (!str) {
		error(L"memory error");
		return NULL;
	}

	for (i=0; i<len; ++i) {
		wchar_t c;
		if (!ctl_check_error(read(fd, &c, sizeof(c)), sizeof(c), true)) {
			return NULL;
		}

		str[i] = SWAP_FROM_LE(wchar_t, c);
	}

	str[len] = 0;

	if (_len) *_len = len;

	return str;
}

int32 ctl_recv_int(int fd)
{
	int32 bi;
	if (!ctl_check_error(read(fd, &bi, sizeof(bi)), sizeof(bi), true)) {
		return false;
	}

	return SWAP_FROM_LE(int32, bi);
}

bool ctl_expect_chars(int fd, const char *str)
{
	int i;
	bool ret = true;
	const int32 len = ctl_recv_int(fd);
	if (check_error()) {
		return false;
	}
	if (len < 0) {
		error(L"communication error");
		return false;
	}

	for (i=0; i<len; ++i) {
		char c;
		if (!ctl_check_error(read(fd, &c, sizeof(c)), sizeof(c), true)) {
			return false;
		}

		if (str) {
			if (c != *str) {
				ret = false;
			}

			if (*str == 0) {
				str = NULL;
				ret = false;
			}
			else {
				++str;
			}
		}
	}

	if (!str || *str != 0) {
		ret = false;
	}

	return ret;
}

void ctl_output_redirect_chars(int fd)
{
	char msg[SIZE];
	int amt;
	while ((amt = read(fd, msg, SIZE-1)) > 0) {
		msg[amt] = '\0';
		printf("%s", msg);
	}
}
