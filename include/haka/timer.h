/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Timer functions.
 */

#ifndef _HAKA_TIMER_H
#define _HAKA_TIMER_H

#include <haka/types.h>
#include <haka/time.h>


struct timer;

/**
 * Timer callback called whenever a timer triggers.
 */
typedef void (*timer_callback)(int count, void *data);

/**
 * Initialize the current thread for timer support.
 */
bool timer_init_thread();

/**
 * Create a new timer.
 */
struct timer *timer_init(timer_callback callback, void *user);

/**
 * Destroy a timer.
 */
void timer_destroy(struct timer *timer);

/**
 * Start a timer to be trigger only once.
 */
bool timer_once(struct timer *timer, struct time *delay);

/**
 * Start a timer to be repeated until it is stopped or destroyed.
 */
bool timer_repeat(struct timer *timer, struct time *delay);

/**
 * Stop a timer.
 */
bool timer_stop(struct timer *timer);

/**
 * Protect a block of code from timer interrupt.
 * \see timer_unguard()
 */
bool timer_guard();

/**
 * Leave a protected block.
 */
bool timer_unguard();

#endif /* _HAKA_TIMER_H */
