/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Time representation.
 */

#ifndef _HAKA_TIME_H
#define _HAKA_TIME_H

#include <haka/types.h>

#include <time.h>


/**
 * Time structure.
 */
struct time {
	time_t       secs;   /**< Seconds */
	uint32       nsecs;  /**< Nano-seconds */
};

#define TIME_BUFSIZE    27         /**< String buffer minimum size. */
#define INVALID_TIME    { 0, 0 }   /**< Static initializer for the struct time. */

extern const struct time  invalid_time;

/**
 * Build a new time structure from a number of seconds.
 */
void       time_build(struct time *t, double secs);

/**
 * Get a current timestamp.
 */
bool       time_gettimestamp(struct time *t);

/**
 * Add two time object.
 */
void       time_add(struct time *res, const struct time *t1, const struct time *t2);

/**
 * Compute the difference between two time object.
 * It returns the result of time_cmp(t1, t2).
 */
int        time_diff(struct time *res, const struct time *t1, const struct time *t2);

/**
 * Divide two time value.
 */
uint64     time_divide(const struct time *t1, const struct time *t2);

/**
 * Multiply a time value.
 */
void       time_mult(struct time *res, const struct time *t1, const int mult);

/**
 * Compare two time object. It returns -1, 1 or 0 respectivelly if t1 is smaller than
 * t2, t1 is larger than t2 or t1 is equal to t2.
 */
int        time_cmp(const struct time *t1, const struct time *t2);

/**
 * Convert time to a number of seconds.
 */
double     time_sec(const struct time *t);

/**
 * Convert time to a string
 * \see TIME_BUFSIZE
 */
bool       time_tostring(const struct time *t, char *buffer, size_t len);

/**
 * Check if the time is valid.
 */
bool       time_isvalid(const struct time *t);

#endif /* _HAKA_TIME_H */
