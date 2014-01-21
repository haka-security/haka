/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_TIME_H
#define _HAKA_TIME_H

#include <haka/types.h>

#include <time.h>


struct time {
	time_t       secs;   /* seconds */
	uint32       nsecs;  /* nano-seconds */
};

#define TIME_BUFSIZE    27
#define INVALID_TIME    { 0, 0 }

void       time_build(struct time *t, double secs);
bool       time_gettimestamp(struct time *t);
void       time_add(struct time *t1, const struct time *t2);
double     time_diff(const struct time *t1, const struct time *t2);
int        time_cmp(const struct time *t1, const struct time *t2);
double     time_sec(const struct time *t);
bool       time_tostring(const struct time *t, char *buffer, size_t len);
bool       time_isvalid(const struct time *t);

#endif /* _HAKA_TIME_H */
