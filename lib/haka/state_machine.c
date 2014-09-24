/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>
#include <haka/timer.h>
#include <haka/packet.h>
#include <haka/state_machine.h>
#include <haka/container/vector.h>


#define MODULE    "state-machine"


/*
 * Types
 */

enum transition_type {
	TRANSITION_NONE = 0,
	TRANSITION_FAIL,
	TRANSITION_TIMEOUT,
	TRANSITION_ENTER,
	TRANSITION_LEAVE,
	TRANSITION_INIT,
	TRANSITION_FINISH
};

struct transition {
	enum transition_type    type;
	struct time             timeout;
	struct transition_data *callback;
};

struct state {
	struct list             list;
	char                   *name;
	struct transition       fail;
	struct transition       enter;
	struct transition       leave;
	struct transition       init;
	struct transition       finish;
	struct vector           timeouts;
};

struct state_machine {
	char                   *name;
	bool                    compiled;
	struct state           *states;
	struct state           *initial;
};

struct state_machine_instance {
	struct state_machine         *state_machine;
	struct state                 *current;
	struct state_machine_context *context;
	struct vector                 timers;
	int                           used_timer;
	bool                          in_transition:1;
	bool                          finished:1;
	bool                          failed:1;
	bool                          in_failure:1;
};

struct timeout_data {
	struct timer                  *timer;
	struct state_machine_instance *instance;
	int                            timer_index;
};


/*
 * State
 */

static void transition_destroy(void *_transition)
{
	struct transition *transition = (struct transition *)_transition;
	if (transition->callback) {
		transition->callback->destroy(transition->callback);
	}
}

static void state_destroy(struct state *state)
{
	vector_destroy(&state->timeouts);
	transition_destroy(&state->fail);
	transition_destroy(&state->enter);
	transition_destroy(&state->leave);
	transition_destroy(&state->init);
	transition_destroy(&state->finish);
	free(state->name);
	free(state);
}

static bool state_compile(struct state *state)
{
	vector_reserve(&state->timeouts, vector_count(&state->timeouts));
	if (check_error()) {
		return false;
	}
	return true;
}

struct state *state_machine_create_state(struct state_machine *state_machine, const char *name)
{
	struct state *state = malloc(sizeof(struct state));
	if (!state) {
		error("memory error");
		return NULL;
	}

	list_init(state);

	if (name) {
		state->name = strdup(name);
		if (!state->name) {
			error("memory error");
			free(state);
			return NULL;
		}
	}
	else {
		state->name = NULL;
	}

	state->fail.type = TRANSITION_NONE;
	state->fail.callback = NULL;
	state->enter.type = TRANSITION_NONE;
	state->enter.callback = NULL;
	state->leave.type = TRANSITION_NONE;
	state->leave.callback = NULL;
	state->init.type = TRANSITION_NONE;
	state->init.callback = NULL;
	state->finish.type = TRANSITION_NONE;
	state->finish.callback = NULL;
	vector_create(&state->timeouts, struct transition, transition_destroy);

	list_insert_before(state, NULL, &state_machine->states, NULL);

	return state;
}

bool state_add_timeout_transition(struct state *state, struct time *timeout, struct transition_data *data)
{
	struct transition *trans = vector_push(&state->timeouts, struct transition);
	trans->type = TRANSITION_TIMEOUT;
	trans->timeout = *timeout;
	trans->callback = data;
	assert(data->callback);
	return true;
}

bool state_set_fail_transition(struct state *state, struct transition_data *data)
{
	state->fail.type = TRANSITION_FAIL;
	state->fail.callback = data;
	assert(data->callback);
	return true;
}

bool state_set_enter_transition(struct state *state, struct transition_data *data)
{
	state->enter.type = TRANSITION_ENTER;
	state->enter.callback = data;
	assert(data->callback);
	return true;
}

bool state_set_leave_transition(struct state *state, struct transition_data *data)
{
	state->leave.type = TRANSITION_LEAVE;
	state->leave.callback = data;
	assert(data->callback);
	return true;
}

bool state_set_init_transition(struct state *state, struct transition_data *data)
{
	state->init.type = TRANSITION_INIT;
	state->init.callback = data;
	assert(data->callback);
	return true;
}

bool state_set_finish_transition(struct state *state, struct transition_data *data)
{
	state->finish.type = TRANSITION_FINISH;
	state->finish.callback = data;
	assert(data->callback);
	return true;
}

static struct state _state_machine_fail_state = {
	name: "FAI",
	fail: {0},
	enter: {0},
	leave: {0},
	init: {0},
	finish: {0},
	timeouts: VECTOR_INIT(struct transition, transition_destroy)
};
struct state * const state_machine_fail_state = &_state_machine_fail_state;

static struct state _state_machine_finish_state = {
	name: "FINISH",
	fail: {0},
	enter: {0},
	leave: {0},
	init: {0},
	finish: {0},
	timeouts: VECTOR_INIT(struct transition, transition_destroy)
};
struct state * const state_machine_finish_state = &_state_machine_finish_state;

const char *state_name(struct state *state)
{
	return state->name;
}


/*
 * State machine
 */

struct state_machine *state_machine_create(const char *name)
{
	struct state_machine *machine;

	assert(name);

	machine = malloc(sizeof(struct state_machine));
	if (!machine) {
		error("memory error");
		return NULL;
	}

	machine->name = strdup(name);
	if (!machine->name) {
		error("memory error");
		free(machine);
		return NULL;
	}

	machine->compiled = false;
	machine->initial = NULL;
	machine->states = NULL;

	return machine;
}

void state_machine_destroy(struct state_machine *machine)
{
	struct state *iter = machine->states;
	while (iter) {
		struct state *current = iter;
		iter = list_next(iter);
		state_destroy(current);
	}

	free(machine->name);
	free(machine);
}

bool state_machine_compile(struct state_machine *machine)
{
	if (!machine->compiled) {
		struct state *iter = machine->states;
		while (iter) {
			if (!state_compile(iter)) {
				return false;
			}

			iter = list_next(iter);
		}

		if (!machine->initial) {
			error("%s: no initial state", machine->name);
			return false;
		}

		machine->compiled = true;
	}
	return true;
}

bool state_machine_set_initial(struct state_machine *machine, struct state *initial)
{
	machine->initial = initial;
	return true;
}


/*
 * State machine instance
 */

void timeout_data_destroy(void *_elem)
{
	struct timeout_data *elem = (struct timeout_data *)_elem;
	timer_destroy(elem->timer);
}

static bool have_transition(struct state_machine_instance *instance, struct transition *trans)
{
	return trans->callback && trans->callback->callback;
}

static struct state *do_transition(struct state_machine_instance *instance, struct transition *trans)
{
	assert(trans->callback && trans->callback->callback);
	return trans->callback->callback(instance, trans->callback);
}

static struct state *state_machine_leave_state(struct state_machine_instance *instance)
{
	struct state *newstate = NULL;

	if (instance->current) {
		int i;
		for (i=0; i<instance->used_timer; ++i) {
			struct timeout_data *t = vector_get(&instance->timers, struct timeout_data, i);
			assert(t);
			timer_stop(t->timer);
		}

		if (have_transition(instance, &instance->current->leave)) {
			messagef(HAKA_LOG_DEBUG, MODULE, "%s: leave transition on state '%s'",
					instance->state_machine->name, instance->current->name);

			newstate = do_transition(instance, &instance->current->leave);
		}

		instance->current = NULL;
	}
	return newstate;
}

static void transition_timeout(int count, void *_data)
{
	struct transition *trans;
	struct timeout_data *data = (struct timeout_data *)_data;
	assert(data);
	assert(data->timer_index < data->instance->used_timer);

	if (!data->instance->in_transition) {
		struct state *newstate;
		assert(data->instance->current);

		trans = vector_get(&data->instance->current->timeouts, struct transition, data->timer_index);
		assert(trans);

		messagef(HAKA_LOG_DEBUG, MODULE, "%s: timeout trigger on state '%s'",
				data->instance->state_machine->name, data->instance->current->name);

		newstate = do_transition(data->instance, trans);
		if (newstate) {
			state_machine_instance_update(data->instance, newstate);
		}
	}
}

static void state_machine_enter_state(struct state_machine_instance *instance, struct state *state)
{
	const bool was_in_transition = instance->in_transition;
	instance->in_transition = true;

	{
		struct state *newstate = state_machine_leave_state(instance);
		if (newstate) {
			state_machine_instance_update(instance, newstate);
			instance->in_transition = was_in_transition;
			return;
		}

		if (state) {
			const int count = vector_count(&state->timeouts);

			instance->current = state;

			if (have_transition(instance, &instance->current->enter)) {
				messagef(HAKA_LOG_DEBUG, MODULE, "%s: enter transition on state '%s'",
					instance->state_machine->name, instance->current->name);

				newstate = do_transition(instance, &instance->current->enter);
				if (newstate) {
					state_machine_instance_update(instance, newstate);
					instance->in_transition = was_in_transition;
					return;
				}
			}

			/* Create missing timers */
			{
				const int tcount = vector_count(&instance->timers);
				int i;

				if (count > tcount) {
					vector_reserve(&instance->timers, count);
				}

				for (i=tcount; i<count; ++i) {
					struct timeout_data *timer = vector_push(&instance->timers, struct timeout_data);
					if (!timer) {
						message(HAKA_LOG_ERROR, MODULE, clear_error());
						state_machine_leave_state(instance);
						return;
					}

					timer->instance = instance;
					timer->timer_index = i;

					timer->timer = time_realm_timer(&network_time, transition_timeout, timer);
					if (!timer->timer) {
						vector_pop(&instance->timers);
						assert(check_error());
						message(HAKA_LOG_ERROR, MODULE, clear_error());
						state_machine_leave_state(instance);
						return;
					}
				}
			}

			instance->used_timer = count;

			/* Start timers */
			{
				int i;
				for (i=0; i<count; ++i) {
					struct timeout_data *timer = vector_get(&instance->timers, struct timeout_data, i);
					struct transition *trans = vector_get(&state->timeouts, struct transition, i);

					assert(trans);
					assert(trans->type == TRANSITION_TIMEOUT);

					timer_repeat(timer->timer, &trans->timeout);
				}
			}

		}
	}

	instance->in_transition = was_in_transition;
}

struct state_machine_instance *state_machine_instance(struct state_machine *state_machine,
		struct state_machine_context *context)
{
	struct state_machine_instance *instance = malloc(sizeof(struct state_machine_instance));
	if (!instance) {
		error("memory error");
		return NULL;
	}

	instance->state_machine = state_machine;
	instance->current = NULL;
	instance->context = context;
	vector_create(&instance->timers, struct timeout_data, timeout_data_destroy);
	instance->used_timer = 0;
	instance->in_transition = false;
	instance->finished = false;
	instance->failed = false;
	instance->in_failure = false;

	messagef(HAKA_LOG_DEBUG, MODULE, "%s: initial state '%s'",
			instance->state_machine->name, state_machine->initial->name);

	return instance;
}

void state_machine_instance_init(struct state_machine_instance *instance)
{
	assert(!instance->current);

	state_machine_enter_state(instance, instance->state_machine->initial);
	instance->finished = false;
	instance->failed = false;
	instance->in_failure = false;

	if (have_transition(instance, &instance->current->init)) {
		messagef(HAKA_LOG_DEBUG, MODULE, "%s: init transition on state '%s'",
				instance->state_machine->name, instance->current->name);

		do_transition(instance, &instance->current->init);
	}
}

void state_machine_instance_finish(struct state_machine_instance *instance)
{
	if (instance->finished) {
		error("state machine instance has finished");
		return;
	}

	if (instance->current) {
		struct state *current = instance->current;

		state_machine_leave_state(instance);

		messagef(HAKA_LOG_DEBUG, MODULE, "%s: finish from state '%s'",
				instance->state_machine->name, current->name);

		if (have_transition(instance, &current->finish)) {
			messagef(HAKA_LOG_DEBUG, MODULE, "%s: finish transition on state '%s'",
					instance->state_machine->name, current->name);

			do_transition(instance, &current->finish);
		}
	}

	instance->finished = true;
}

void state_machine_instance_destroy(struct state_machine_instance *instance)
{
	if (!instance->finished) {
		state_machine_instance_finish(instance);
	}

	if (instance->context) {
		instance->context->destroy(instance->context);
	}

	vector_destroy(&instance->timers);
	free(instance);
}

void state_machine_instance_update(struct state_machine_instance *instance, struct state *newstate)
{
	assert(newstate);

	if (instance->finished) {
		error("state machine instance has finished");
		return;
	}

	if (newstate == state_machine_fail_state) {
		state_machine_instance_fail(instance);
	}
	else if (newstate == state_machine_finish_state) {
		state_machine_instance_finish(instance);
	}
	else {
		if (instance->current) {
			messagef(HAKA_LOG_DEBUG, MODULE, "%s: transition from state '%s' to state '%s'",
					instance->state_machine->name, instance->current->name, newstate->name);
		}
		else {
			messagef(HAKA_LOG_DEBUG, MODULE, "%s: transition to state '%s'",
					instance->state_machine->name, newstate->name);
		}

		state_machine_enter_state(instance, newstate);
	}
}

static void _state_machine_instance_transition(struct state_machine_instance *instance,
		struct transition *trans, const char *type, bool allow_state_change)
{
	struct state *newstate;

	if (instance->current) {
		if (trans->callback) {
			if (trans->callback->callback) {
				messagef(HAKA_LOG_DEBUG, MODULE, "%s: %s transition on state '%s'",
						instance->state_machine->name, type, instance->current->name);

				newstate = trans->callback->callback(instance, trans->callback);
				if (newstate && allow_state_change) {
					state_machine_instance_update(instance, newstate);
				}
			}
		}
	}
}

void state_machine_instance_fail(struct state_machine_instance *instance)
{
	if (instance->in_failure) {
		return;
	}

	if (instance->finished) {
		error("state machine instance has finished");
		return;
	}

	instance->failed = true;
	instance->in_failure = true;

	_state_machine_instance_transition(instance, &instance->current->fail, "fail", false);
	state_machine_instance_finish(instance);

	instance->in_failure = false;
}

struct state_machine *state_machine_instance_get(struct state_machine_instance *instance)
{
	return instance->state_machine;
}

struct state_machine_context *state_machine_instance_context(struct state_machine_instance *instance)
{
	return instance->context;
}

struct state *state_machine_instance_state(struct state_machine_instance *instance)
{
	return instance->current;
}

bool state_machine_instance_isfinished(struct state_machine_instance *instance)
{
	return instance->finished;
}

bool state_machine_instance_isfailed(struct state_machine_instance *instance)
{
	return instance->failed;
}
