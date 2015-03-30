/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CTL_COMM_H
#define CTL_COMM_H

#include <haka/types.h>
#include <wchar.h>

bool ctl_send_chars(int fd, const char *str, size_t len);
bool ctl_send_wchars(int fd, const wchar_t *str, size_t len);
bool ctl_send_int(int fd, int32 i);
bool ctl_send_status(int fd, int ret, const char *err);

char *ctl_recv_chars(int fd, size_t *len);
wchar_t *ctl_recv_wchars(int fd, size_t *len);
int32 ctl_recv_int(int fd);
int   ctl_recv_status(int fd);

void ctl_output_redirect_chars(int fd);

bool ctl_expect_chars(int fd, const char *str);

#endif /* CTL_COMM_H */
