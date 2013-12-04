/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <haka/time.h>
#include <haka/error.h>


time_us time_gettimestamp()
{
	struct timespec time;
	if (clock_gettime(CLOCK_REALTIME, &time)) {
		error(L"time error: %s", errno_error(errno));
		return INVALID_TIME;
	}

	return (((time_us)time.tv_sec)*1000000) + (time.tv_nsec / 1000);
}

difftime_us time_diff(time_us t1, time_us t2)
{
	return ((difftime_us)t1) - t2;
}

bool time_tostring(time_us t, char *buffer)
{
	time_t time = t / 1000000;
	if (!ctime_r(&time, buffer)) {
		error(L"time convertion error");
		return false;
	}

	assert(strlen(buffer) > 0);
	buffer[strlen(buffer)-1] = 0; /* remove trailing '\n' */
	return true;
}
