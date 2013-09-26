
#include <time.h>
#include <errno.h>
#include <math.h>

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
	if (clock_gettime(CLOCK_MONOTONIC, &time)) {
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
