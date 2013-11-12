
#ifndef _HAKA_TIME_H
#define _HAKA_TIME_H

#include <haka/types.h>

typedef uint64   time_us; /* time in micro-seconds since the Epoch */
typedef int64    difftime_us;

#define INVALID_TIME    0
#define TIME_BUFSIZE    27

time_us          time_gettimestamp();
difftime_us      time_diff(time_us t1, time_us t2);
bool             time_tostring(time_us t, char *buffer);

#endif /* _HAKA_TIME_H */
