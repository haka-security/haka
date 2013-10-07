
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>
#include <haka/timer.h>
#include <haka/state_machine.h>
#include <haka/container/vector.h>


#define MODULE    L"state machine"


/*
 * Types
 */

enum transition_type {
	TRANSITION_NONE = 0,
	TRANSITION_ERROR,
	TRANSITION_TIMEOUT,
	TRANSITION_INPUT,
	TRANSITION_OUTPUT,
	TRANSITION_ENTER,
	TRANSITION_LEAVE
};

struct transition {
	enum transition_type    type;
	struct time             timeout;
	struct transition_data *callback;
};

struct state {
	struct list             list;
	char                   *name;
	struct transition       error;
	struct transition       input;
	struct transition       output;
	struct transition       enter;
	struct transition       leave;
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
	bool                          in_transition;
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
	transition_destroy(&state->error);
	transition_destroy(&state->input);
	transition_destroy(&state->output);
	transition_destroy(&state->enter);
	transition_destroy(&state->leave);
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
		error(L"memory error");
		return NULL;
	}

	list_init(state);

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
	state->output.type = TRANSITION_NONE;
	state->output.callback = NULL;
	state->enter.type = TRANSITION_NONE;
	state->enter.callback = NULL;
	state->leave.type = TRANSITION_NONE;
	state->leave.callback = NULL;
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

bool state_set_error_transition(struct state *state, struct transition_data *data)
{
	state->error.type = TRANSITION_ERROR;
	state->error.callback = data;
	assert(data->callback);
	return true;
}

bool state_set_input_transition(struct state *state, struct transition_data *data)
{
	state->input.type = TRANSITION_INPUT;
	state->input.callback = data;
	assert(data->input_callback);
	return true;
}

bool state_set_output_transition(struct state *state, struct transition_data *data)
{
	state->output.type = TRANSITION_OUTPUT;
	state->output.callback = data;
	assert(data->input_callback);
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

static struct state _state_machine_error_state = {
	name: "ERROR",
	error: {0},
	input: {0},
	output: {0},
	enter: {0},
	leave: {0},
	timeouts: VECTOR_INIT(struct transition, transition_destroy)
};
struct state * const state_machine_error_state = &_state_machine_error_state;

static struct state _state_machine_finish_state = {
	name: "FINISH",
	error: {0},
	input: {0},
	output: {0},
	enter: {0},
	leave: {0},
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
		error(L"memory error");
		return NULL;
	}

	machine->name = strdup(name);
	if (!machine->name) {
		error(L"memory error");
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
			error(L"%s: no initial state", machine->name);
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
			messagef(HAKA_LOG_DEBUG, MODULE, L"%s: leave transition on state '%s'",
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

		messagef(HAKA_LOG_DEBUG, MODULE, L"%s: timeout trigger on state '%s'",
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

	if (instance->current != state) {
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
				messagef(HAKA_LOG_DEBUG, MODULE, L"%s: enter transition on state '%s'",
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

					timer->timer = timer_init(transition_timeout, timer);
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
		error(L"memory error");
		return NULL;
	}

	instance->state_machine = state_machine;
	instance->current = NULL;
	instance->context = context;
	vector_create(&instance->timers, struct timeout_data, timeout_data_destroy);
	instance->used_timer = 0;
	instance->in_transition = false;

	state_machine_enter_state(instance, state_machine->initial);

	messagef(HAKA_LOG_DEBUG, MODULE, L"%s: initial state '%s'",
			instance->state_machine->name, state_machine->initial->name);

	return instance;
}

void state_machine_instance_finish(struct state_machine_instance *instance)
{
	if (instance->current) {
		messagef(HAKA_LOG_DEBUG, MODULE, L"%s: finish from state '%s'",
				instance->state_machine->name, instance->current->name);
	}

	state_machine_leave_state(instance);
}

void state_machine_instance_destroy(struct state_machine_instance *instance)
{
	state_machine_instance_finish(instance);

	if (instance->context) {
		instance->context->destroy(instance->context);
	}

	vector_destroy(&instance->timers);
	free(instance);
}

void state_machine_instance_update(struct state_machine_instance *instance, struct state *newstate)
{
	assert(newstate);

	if (newstate == state_machine_error_state) {
		state_machine_instance_error(instance);
	}
	else if (newstate == state_machine_finish_state) {
		state_machine_instance_finish(instance);
	}
	else {
		if (instance->current) {
			messagef(HAKA_LOG_DEBUG, MODULE, L"%s: transition from state '%s' to state '%s'",
					instance->state_machine->name, instance->current->name, newstate->name);
		}
		else {
			messagef(HAKA_LOG_DEBUG, MODULE, L"%s: transition to state '%s'",
					instance->state_machine->name, newstate->name);
		}

		state_machine_enter_state(instance, newstate);
	}
}

static void _state_machine_instance_transition(struct state_machine_instance *instance,
		struct transition *trans, bool input_callback, const char *type, void *arg)
{
	struct state *newstate;

	if (instance->current) {
		if (trans->callback) {
			if (input_callback ? trans->callback->input_callback != NULL : trans->callback->callback != NULL) {
				messagef(HAKA_LOG_DEBUG, MODULE, L"%s: %s transition on state '%s'",
						instance->state_machine->name, type, instance->current->name);

				if (input_callback) {
					newstate = trans->callback->input_callback(instance, trans->callback, arg);
				}
				else {
					newstate = trans->callback->callback(instance, trans->callback);
				}

				if (newstate) {
					state_machine_instance_update(instance, newstate);
				}
			}
		}
	}
}

void state_machine_instance_error(struct state_machine_instance *instance)
{
	_state_machine_instance_transition(instance, &instance->current->error, false, "error", NULL);
}

void state_machine_instance_input(struct state_machine_instance *instance, void *input)
{
	_state_machine_instance_transition(instance, &instance->current->input, true, "input", input);
}

void state_machine_instance_output(struct state_machine_instance *instance, void *output)
{
	_state_machine_instance_transition(instance, &instance->current->output, true, "output", output);
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
