/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Timer functions.
 */

#ifndef HAKA_TIMER_H
#define HAKA_TIMER_H

#include <haka/types.h>
#include <haka/time.h>
#include <haka/thread.h>


/**
 * Timer mode.
 */
enum time_realm_mode {
	TIME_REALM_REALTIME, /* Real-time timer. */
	TIME_REALM_STATIC    /* Static time. The time must be updated manually. */
};

/** Opaque timer environment structure. */
struct time_realm {
	enum time_realm_mode   mode;
	local_storage_t        states; /* struct timer_group_state */;
};

struct timer;        /**< Opaque timer structure. */


/**
 * Timer callback called whenever a timer triggers.
 */
typedef void (*timer_callback)(int count, void *data);

/**
 * Create a new time realm.
 */
bool time_realm_initialize(struct time_realm *realm, enum time_realm_mode mode);

/**
 * Destroy a time realm.
 */
bool time_realm_destroy(struct time_realm *realm);

/**
 * Update the time of a time realm that is in TIMER_REALM_STATIC mode.
 * It does also check for timer to trigger.
 */
void time_realm_update_and_check(struct time_realm *realm, const struct time *value);

/**
 * Get the current local time of the time realm.
 */
const struct time *time_realm_current_time(struct time_realm *realm);

/**
 * Create a new timer.
 */
struct timer *time_realm_timer(struct time_realm *realm, timer_callback callback, void *user);

/**
 * Check and execute timer callbacks.
 */
bool time_realm_check(struct time_realm *realm);

/**
 * Initialize the current thread for timer support.
 * \return false if an error occurred.
 */
bool timer_init_thread();

/**
 * Destroy a timer.
 */
void timer_destroy(struct timer *timer);

/**
 * Start a timer to be trigger only once.
 * \return false if an error occurred.
 */
bool timer_once(struct timer *timer, struct time *delay);

/**
 * Start a timer to be repeated until it is stopped or destroyed.
 * \return false if an error occurred.
 */
bool timer_repeat(struct timer *timer, struct time *delay);

/**
 * Stop a timer.
 * \return false if an error occurred.
 */
bool timer_stop(struct timer *timer);

#endif /* HAKA_TIMER_H */
