/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <time.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include <haka/time.h>
#include <haka/error.h>


void time_build(struct time *t, double secs)
{
	t->secs = secs;
	t->nsecs = (secs - floor(secs)) * 1000000000.;
}

bool time_gettimestamp(struct time *t)
{
	struct timespec time;
	if (clock_gettime(CLOCK_REALTIME, &time)) {
		error(L"time error: %s", errno_error(errno));
		return false;
	}

	t->secs = time.tv_sec;
	t->nsecs= time.tv_nsec;
	return true;
}

void time_add(struct time *t1, const struct time *t2)
{
	t1->nsecs += t2->nsecs;
	t1->secs += t2->secs + t1->nsecs / 1000000000;
	t1->nsecs %= 1000000000;
}

double time_diff(const struct time *t1, const struct time *t2)
{
	if (time_cmp(t1, t2) <= 0) {
		double sec = (t1->secs - t2->secs);
		sec += ((int32)t1->nsecs - t2->nsecs) / 1000000000.;
		return sec;
	}
	else {
		return -time_diff(t2, t1);
	}
}

double time_sec(const struct time *t)
{
	return ((double)t->secs) + t->nsecs / 1000000000.;
}

int time_cmp(const struct time *t1, const struct time *t2)
{
	if (t1->secs < t2->secs) {
		return 1;
	}
	else if (t1->secs > t2->secs) {
		return -1;
	}
	else {
		if (t1->nsecs < t2->nsecs) {
			return 1;
		}
		else if (t1->nsecs > t2->nsecs) {
			return -1;
		}
		else {
			return 0;
		}
	}
}

bool time_tostring(const struct time *t, char *buffer, size_t len)
{
	assert(len >= TIME_BUFSIZE);

	if (!ctime_r(&t->secs, buffer)) {
		error(L"time convertion error");
		return false;
	}

	assert(strlen(buffer) > 0);
	buffer[strlen(buffer)-1] = 0; /* remove trailing '\n' */
	return true;
}

bool time_isvalid(const struct time *t)
{
	return t->secs != 0 || t->nsecs != 0;
}
