/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include <haka/container/vector.h>
#include <haka/time.h>


struct state;
struct state_machine;
struct transition_data;
struct state_machine_instance;

typedef struct state *(*transition_callback)(struct state_machine_instance *state_machine, struct transition_data *data);

struct transition_data {
	transition_callback        callback;
	void                     (*destroy)(struct transition_data *data);
};

struct state_machine_context {
	void                     (*destroy)(struct state_machine_context *data);
};

struct state_machine *state_machine_create(const char *name);
void                  state_machine_destroy(struct state_machine *machine);
bool                  state_machine_compile(struct state_machine *machine);
struct state         *state_machine_create_state(struct state_machine *state_machine, const char *name);
extern struct state * const state_machine_fail_state;
extern struct state * const state_machine_finish_state;
bool                  state_machine_set_initial(struct state_machine *machine, struct state *initial);

bool                  state_add_timeout_transition(struct state *state, struct time *timeout, struct transition_data *data);
bool                  state_set_fail_transition(struct state *state, struct transition_data *data);
bool                  state_set_enter_transition(struct state *state, struct transition_data *data);
bool                  state_set_leave_transition(struct state *state, struct transition_data *data);
bool                  state_set_init_transition(struct state *state, struct transition_data *data);
bool                  state_set_finish_transition(struct state *state, struct transition_data *data);
const char           *state_name(struct state *state);

struct state_machine_instance *state_machine_instance(struct state_machine *machine, struct state_machine_context *context);
struct state_machine *state_machine_instance_get(struct state_machine_instance *instance);
struct state_machine_context *state_machine_instance_context(struct state_machine_instance *instance);
void                  state_machine_instance_init(struct state_machine_instance *instance);
void                  state_machine_instance_finish(struct state_machine_instance *instance);
void                  state_machine_instance_destroy(struct state_machine_instance *instance);
void                  state_machine_instance_update(struct state_machine_instance *instance, struct state *newstate);
void                  state_machine_instance_fail(struct state_machine_instance *instance);
struct state         *state_machine_instance_state(struct state_machine_instance *instance);
bool                  state_machine_instance_isfinished(struct state_machine_instance *instance);
bool                  state_machine_instance_isfailed(struct state_machine_instance *instance);

#endif /* _STATE_MACHINE_H */
