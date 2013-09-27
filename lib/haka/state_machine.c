
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>
#include <haka/timer.h>
#include <haka/state_machine.h>
#include <haka/container/vector.h>


#define MODULE    L"state"


/*
 * Types
 */

enum transition_type {
	TRANSITION_ERROR,
	TRANSITION_TIMEOUT,
	TRANSITION_INPUT,
	TRANSITION_NONE
};

struct transition {
	enum transition_type    type;
	struct time             timeout;
	struct transition_data *callback;
};

struct state {
	char                   *name;
	struct transition       error;
	struct transition       input;
	struct vector           timeouts;
};

struct state_machine {
	bool                    compiled;
	struct vector           states;
	struct state           *initial;
};

struct state_machine_instance {
	struct state_machine  *state_machine;
	struct state          *current;
	struct vector          timers;
	int                    used_timer;
	bool                   in_transition;
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

static void state_destroy(void *_state)
{
	struct state *state = (struct state *)_state;
	vector_destroy(&state->timeouts);
	transition_destroy(&state->error);
	transition_destroy(&state->input);
	free(state->name);
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
		error(L"memory error");
		return NULL;
	}

	if (name) {
		state->name = strdup(name);
		if (!state->name) {
			error(L"memory error");
			free(state);
			return NULL;
		}
	}
	else {
		state->name = NULL;
	}

	state->error.type = TRANSITION_NONE;
	state->error.callback = NULL;
	state->input.type = TRANSITION_NONE;
	state->input.callback = NULL;
	vector_create(&state->timeouts, struct transition, transition_destroy);

	return state;
}

bool state_add_timeout_transition(struct state *state, struct time *timeout, struct transition_data *data)
{
	struct transition *trans = vector_push(&state->timeouts, struct transition);
	trans->type = TRANSITION_TIMEOUT;
	trans->timeout = *timeout;
	trans->callback = data;
	return true;
}

bool state_set_error_transition(struct state *state, struct transition_data *data)
{
	state->error.type = TRANSITION_ERROR;
	state->error.callback = data;
	return true;
}

bool state_set_input_transition(struct state *state, struct transition_data *data)
{
	state->error.type = TRANSITION_INPUT;
	state->error.callback = data;
	return true;
}


/*
 * State machine
 */

struct state_machine *state_machine_create()
{
	struct state_machine *machine = malloc(sizeof(struct state_machine));
	if (!machine) {
		error(L"memory error");
		return NULL;
	}

	machine->compiled = false;
	machine->initial = NULL;
	vector_create(&machine->states, struct state, state_destroy);
	if (check_error()) {
		free(machine);
		return NULL;
	}

	return machine;
}

void state_machine_destroy(struct state_machine *machine)
{
	vector_destroy(&machine->states);
}

bool state_machine_compile(struct state_machine *machine)
{
	if (!machine->compiled) {
		int i, count;

		vector_reserve(&machine->states, vector_count(&machine->states));
		if (check_error()) {
			return false;
		}

		count = vector_count(&machine->states);
		for (i=0; i<count; ++i) {
			struct state *state = vector_get(&machine->states, struct state, i);
			assert(state);

			if (!state_compile(state)) {
				return false;
			}
		}

		machine->compiled = true;
	}
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

static void state_machine_leave_state(struct state_machine_instance *instance)
{
	if (instance->current) {
		int i;
		for (i=0; i<instance->used_timer; ++i) {
			struct timeout_data *t = vector_get(&instance->timers, struct timeout_data, i);
			assert(t);
			timer_stop(t->timer);
		}

		instance->current = NULL;
	}
}

static void do_transition(struct state_machine_instance *instance, struct transition *trans)
{
	if (trans->callback && trans->callback->callback) {
		trans->callback->callback(instance, trans->callback);
	}
}

static void transition_timeout(int count, void *_data)
{
	struct transition *trans;
	struct timeout_data *data = (struct timeout_data *)_data;
	assert(data);
	assert(data->timer_index < data->instance->used_timer);

	if (!data->instance->in_transition) {
		trans = vector_get(&data->instance->current->timeouts, struct transition, data->timer_index);
		assert(trans);

		do_transition(data->instance, trans);
	}
}

static void state_machine_enter_state(struct state_machine_instance *instance, struct state *state)
{
	instance->in_transition = true;

	if (instance->current != state) {
		state_machine_leave_state(instance);

		if (state) {
			const int count = vector_count(&state->timeouts);

			instance->current = state;

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
						error(L"memory error");
						// TODO
						return;
					}

					timer->instance = instance;
					timer->timer_index = i;

					timer->timer = timer_init(transition_timeout, timer);
					if (!timer->timer) {
						assert(check_error());
						// TODO
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

	instance->in_transition = false;
}

struct state_machine_instance *state_machine_instance(struct state_machine *state_machine)
{
	struct state_machine_instance *instance = malloc(sizeof(struct state_machine_instance));
	if (!instance) {
		error(L"memory error");
		return NULL;
	}

	instance->state_machine = state_machine;
	instance->current = NULL;
	vector_create(&instance->timers, struct timeout_data, timeout_data_destroy);
	instance->used_timer = 0;
	instance->in_transition = false;

	state_machine_enter_state(instance, state_machine->initial);

	return instance;
}

void state_machine_instance_destroy(struct state_machine_instance *instance)
{
	state_machine_leave_state(instance);
	vector_destroy(&instance->timers);
}

void state_machine_instance_update(struct state_machine_instance *instance, struct state *newstate)
{
	state_machine_enter_state(instance, newstate);
}

void state_machine_instance_error(struct state_machine_instance *instance)
{
	assert(instance->current);
	do_transition(instance, &instance->current->error);
}

void state_machine_instance_input(struct state_machine_instance *instance, void *input)
{
	assert(instance->current);
	do_transition(instance, &instance->current->input);
}

struct state_machine *state_machine_instance_get(struct state_machine_instance *instance)
{
	return instance->state_machine;
}
