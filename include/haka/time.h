
#ifndef _HAKA_TIME_H
#define _HAKA_TIME_H

#include <haka/types.h>

#include <time.h>


struct time {
	time_t       secs;   /* seconds */
	uint32       nsecs;  /* nano-seconds */
};

void       time_build(struct time *t, double secs);
bool       time_gettimestamp(struct time *t);
void       time_add(struct time *t1, const struct time *t2);
double     time_diff(const struct time *t1, const struct time *t2);
int        time_cmp(const struct time *t1, const struct time *t2);
double     time_sec(const struct time *t);

#endif /* _HAKA_TIME_H */
