/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <haka/timer.h>
#include <haka/error.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/compiler.h>
#include <haka/container/list2.h>


struct timer {
	struct list2_elem       list;
	bool                    armed:1;
	bool                    repeat:1;
	struct time             trigger_time;
	struct time             delay;
	timer_callback          callback;
	void                   *data;
	struct time_realm      *realm;
};

struct time_realm_state {
	timer_t            timer;
	struct list2       sorted_timer;
	bool               check_timer;
	struct time_realm *realm;
};


static void timer_handler(int sig, siginfo_t *si, void *uc)
{
	struct time_realm_state *state = (struct time_realm_state *)si->si_value.sival_ptr;
	if (state) {
		state->check_timer = true;
	}
}

static struct time_realm_state *create_time_realm_state(struct time_realm *realm)
{
	struct sigevent sev;

	struct time_realm_state *state = malloc(sizeof(struct time_realm_state));
	if (!state) {
		error("memory error");
		return NULL;
	}

	list2_init(&state->sorted_timer);

	if (realm->mode == TIME_REALM_REALTIME) {
		memset(&sev, 0, sizeof(sev));
		sev.sigev_notify = SIGEV_THREAD_ID;
		sev.sigev_signo = SIGALRM;
		sev.sigev_value.sival_ptr = state;
		sev._sigev_un._tid = syscall(SYS_gettid);

		if (timer_create(CLOCK_MONOTONIC, &sev, &state->timer)) {
			free(state);
			error("timer creation error: %s", errno_error(errno));
			return NULL;
		}
	}

	state->check_timer = false;
	state->realm = realm;

	return state;
}

static struct time_realm_state *get_time_realm_state(struct time_realm *realm, bool create)
{
	struct time_realm_state *state = (struct time_realm_state *)local_storage_get(&realm->states);
	if (!state && create) {
		state = create_time_realm_state(realm);
		local_storage_set(&realm->states, state);
	}
	return state;
}

static void free_time_realm_state(void *_ptr)
{
	struct time_realm_state *state = (struct time_realm_state *)_ptr;
	if (state) {
		const list2_iter end = list2_end(&state->sorted_timer);
		list2_iter iter = list2_begin(&state->sorted_timer);
		while (iter != end) {
			struct timer *timer = list2_get(iter, struct timer, list);
			timer->armed = false;

			iter = list2_erase(iter);
		}

		if (state->realm->mode == TIME_REALM_REALTIME) {
			timer_delete(state->timer);
		}
		free(state);
	}
}

bool time_realm_initialize(struct time_realm *realm, enum time_realm_mode mode)
{
	realm->mode = mode;

	switch (mode) {
	case TIME_REALM_REALTIME:
	case TIME_REALM_STATIC:
		break;

	default:
		error("invalid timer mode");
		return false;
	}

	realm->mode = mode;
	local_storage_init(&realm->states, free_time_realm_state);
	return true;
}

bool time_realm_destroy(struct time_realm *realm)
{
	/* It is needed to cleanup the main thread state local data as it is not
	 * done by destroy.
	 */
	free_time_realm_state(local_storage_get(&realm->states));
	local_storage_set(&realm->states, NULL);

	return local_storage_destroy(&realm->states);
}

void time_realm_update(struct time_realm *realm, const struct time *value)
{
	struct time difftime, oldtime = realm->time;
	int sign;
	assert(realm->mode == TIME_REALM_STATIC);

	sign = time_diff(&difftime, value, &oldtime);
	if (sign < 0) {
		LOG_DEBUG("timer", "static time going backward (ignored)");
	}
	else {
		LOG_DEBUG("timer", "static time offset %s%f seconds", sign >= 0? "+" : "-", time_sec(&difftime));

		realm->time = *value;
		realm->check_timer = true;
	}
}

const struct time *time_realm_current_time(struct time_realm *realm)
{
	switch (realm->mode) {
	case TIME_REALM_REALTIME:
		time_gettimestamp(&realm->time);
		/* no break */

	case TIME_REALM_STATIC:
		return &realm->time;

	default:
		error("invalid timer mode");
		return NULL;
	}
}

INIT static void _timer_init()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		LOG_FATAL("timer", "%s", errno_error(errno));
		abort();
	}

	timer_init_thread();
}

bool timer_init_thread()
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	if (!thread_sigmask(SIG_UNBLOCK, &mask, NULL)) {
		return false;
	}

	return true;
}

struct timer *time_realm_timer(struct time_realm *realm, timer_callback callback, void *user)
{
	struct timer *timer;
	struct time_realm_state *state = get_time_realm_state(realm, true);
	if (!state) return NULL;

	timer = malloc(sizeof(struct timer));
	if (!timer) {
		error("memory error");
		return NULL;
	}

	list2_elem_init(&timer->list);
	timer->armed = false;
	timer->repeat = false;
	timer->trigger_time = invalid_time;
	timer->delay = invalid_time;
	timer->callback = callback;
	timer->data = user;
	timer->realm = realm;

	return timer;
}

static bool time_realm_insert_timer(struct time_realm_state *state,
		struct timer *timer)
{
	const list2_iter begin = list2_begin(&state->sorted_timer);
	const list2_iter end = list2_end(&state->sorted_timer);
	list2_iter iter;

	for (iter = begin; iter != end; iter = list2_next(iter)) {
		struct timer *cur = list2_get(iter, struct timer, list);
		if (time_cmp(&timer->trigger_time, &cur->trigger_time) < 0) {
			break;
		}
	}

	list2_insert(iter, &timer->list);

	return iter == begin;
}

static bool time_realm_update_timer_list(struct time_realm_state *state,
		bool update_timer)
{
	if (!update_timer) {
		return true;
	}

	if (state->realm->mode == TIME_REALM_REALTIME) {
		struct itimerspec ts;
		memset(&ts, 0, sizeof(ts));

		if (list2_empty(&state->sorted_timer)) {
			/* stop the timer */
			if (timer_settime(state->timer, 0, &ts, NULL) != 0) {
				error("%s", errno_error(errno));
				return false;
			}
		}
		else {
			struct time diff;
			struct timer *first = list2_first(&state->sorted_timer, struct timer, list);
			struct time current = *time_realm_current_time(state->realm);

			if (time_diff(&diff, &first->trigger_time, &current) <= 0) {
				state->check_timer = true;

				/* stop the timer, the timer will be restarted if needed by the
				 * next call to timer_realm_check(). */
				if (timer_settime(state->timer, 0, &ts, NULL) != 0) {
					error("%s", errno_error(errno));
					return false;
				}
			}
			else {
				ts.it_value.tv_sec = diff.secs;
				ts.it_value.tv_nsec = diff.nsecs;

				if (timer_settime(state->timer, 0, &ts, NULL) != 0) {
					error("%s", errno_error(errno));
					return false;
				}

				LOG_DEBUG("timer", "next timer in %f seconds", time_sec(&diff));
			}
		}
	}
	else {
		if (!list2_empty(&state->sorted_timer)) {
			struct time diff;
			struct timer *first = list2_first(&state->sorted_timer, struct timer, list);
			struct time current = *time_realm_current_time(state->realm);

			if (time_diff(&diff, &first->trigger_time, &current) > 0) {
				LOG_DEBUG("timer", "next timer in %f seconds", time_sec(&diff));
			}
		}
	}

	return true;
}

void timer_destroy(struct timer *timer)
{
	timer_stop(timer);
	free(timer);
}

static bool timer_start(struct timer *timer, struct time *delay, bool repeat)
{
	struct time_realm_state *state = get_time_realm_state(timer->realm, true);

	if (delay->secs == 0 && delay->nsecs == 0) {
		error("invalid timer delay");
		return false;
	}

	timer_stop(timer);

	timer->delay = *delay;
	time_add(&timer->trigger_time, time_realm_current_time(state->realm), &timer->delay);
	timer->armed = true;
	timer->repeat = repeat;

	return time_realm_update_timer_list(state, time_realm_insert_timer(state, timer));
}

bool timer_once(struct timer *timer, struct time *delay)
{
	return timer_start(timer, delay, false);
}

bool timer_repeat(struct timer *timer, struct time *delay)
{
	return timer_start(timer, delay, true);
}

bool timer_stop(struct timer *timer)
{
	struct time_realm_state *state = get_time_realm_state(timer->realm, true);
	bool is_first;

	if (!timer->armed) return false;

	is_first = list2_prev(&timer->list) == list2_end(&state->sorted_timer);
	list2_erase(&timer->list);
	timer->armed = false;
	return time_realm_update_timer_list(state, is_first);
}

bool time_realm_check(struct time_realm *realm)
{
	struct time_realm_state *state = get_time_realm_state(realm, false);
	if (state && (state->check_timer || state->realm->check_timer)) {
		bool need_update = false;
		struct list2 repeat_list; /* store repeat timers to reinsert them safely */
		struct time current = *time_realm_current_time(state->realm);
		list2_iter iter = list2_begin(&state->sorted_timer);
		list2_iter end = list2_end(&state->sorted_timer);

		state->check_timer = false;
		state->realm->check_timer = false;

		list2_init(&repeat_list);

		while (iter != end) {
			struct timer *timer = list2_get(iter, struct timer, list);
			if (time_cmp(&timer->trigger_time, &current) <= 0) {
				int count;

				iter = list2_erase(&timer->list);
				if (timer->repeat) {
					struct time offset;
					list2_insert(list2_end(&repeat_list), &timer->list);

					/* Compute the next trigger time as well as the number
					 * of missed triggers. */
					if (time_diff(&offset, &timer->trigger_time, &current) <= 0) {
						count = time_divide(&offset, &timer->delay) + 1;
						time_mult(&offset, &timer->delay, count);
						time_add(&timer->trigger_time, &timer->trigger_time, &offset);

						assert(time_cmp(&timer->trigger_time, &current) >= 0);
					}
					else {
						/* The time seams to have gone back in time, this is case
						 * should not be reached. */
						time_add(&timer->trigger_time, &current, &timer->delay);
						count = 1;
					}
				}
				else {
					count = 1;
					timer->armed = false;
				}

				(*timer->callback)(count, timer->data);

				need_update = true;
			}
			else {
				break;
			}
		}

		if (!list2_empty(&repeat_list)) {
			iter = list2_begin(&repeat_list);
			end = list2_end(&repeat_list);
			while (iter != end) {
				struct timer *timer = list2_get(iter, struct timer, list);
				iter = list2_erase(iter);
				time_realm_insert_timer(state, timer);
			}
		}

		return time_realm_update_timer_list(state, need_update);
	}
	return true;
}
