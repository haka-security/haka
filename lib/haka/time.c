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


#define SEC_TO_NSEC      1000000000ULL
#define SEC_TO_NSEC_F    1000000000.

const struct time invalid_time = INVALID_TIME;

void time_build(struct time *t, double secs)
{
	t->secs = secs;
	t->nsecs = (secs - floor(secs)) * SEC_TO_NSEC_F;
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

void time_add(struct time *res, const struct time *t1, const struct time *t2)
{
	res->nsecs = t1->nsecs + t2->nsecs;
	res->secs = t1->secs + t2->secs + res->nsecs / SEC_TO_NSEC;
	res->nsecs %= SEC_TO_NSEC;
}

static void _time_diff(struct time *res, const struct time *t1, const struct time *t2)
{
	assert(t1->secs >= t2->secs);
	assert(time_cmp(t1, t2) >= 0);

	res->secs = t1->secs - t2->secs;

	if (t1->nsecs >= t2->nsecs) {
		res->nsecs = t1->nsecs - t2->nsecs;
	}
	else {
		assert(t1->nsecs + SEC_TO_NSEC >= t2->nsecs);
		res->nsecs = t1->nsecs + SEC_TO_NSEC - t2->nsecs;
		res->secs -= 1;
	}
}

int time_diff(struct time *res, const struct time *t1, const struct time *t2)
{
	const int cmp = time_cmp(t1, t2);
	if (cmp >= 0) {
		_time_diff(res, t1, t2);
	}
	else {
		_time_diff(res, t2, t1);
	}
	return cmp;
}

uint64 time_divide(const struct time *t1, const struct time *t2)
{
	const uint64 a = t1->secs * SEC_TO_NSEC + t1->nsecs;
	const uint64 b = t2->secs * SEC_TO_NSEC + t2->nsecs;
	return a / b;
}

void time_mult(struct time *res, const struct time *t1, const int mult)
{
	const uint64 nsecs = ((uint64)t1->nsecs) * mult;

	res->secs = t1->secs * mult;
	res->secs += nsecs / SEC_TO_NSEC;
	res->nsecs = nsecs % SEC_TO_NSEC;
}

double time_sec(const struct time *t)
{
	return ((double)t->secs) + t->nsecs / SEC_TO_NSEC_F;
}

int time_cmp(const struct time *t1, const struct time *t2)
{
	if (t1->secs < t2->secs) {
		return -1;
	}
	else if (t1->secs > t2->secs) {
		return 1;
	}
	else {
		if (t1->nsecs < t2->nsecs) {
			return -1;
		}
		else if (t1->nsecs > t2->nsecs) {
			return 1;
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

bool time_tofstring(const struct time *t, const char *format, char *buffer, size_t len)
{
	size_t size;
	struct tm tm;
	if (!gmtime_r(&t->secs, &tm)) {
		error(L"time error: %s", errno_error(errno));
		return false;
	}

	size = strftime(buffer, len, format, &tm);
	buffer[size] = '\0';

	return true;
}

bool time_isvalid(const struct time *t)
{
	return t->secs != 0 || t->nsecs != 0;
}
