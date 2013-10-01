
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
	transition_callback    callback;
	void                 (*destroy)(struct transition_data *data);
};

struct state_machine *state_machine_create();
void                  state_machine_destroy(struct state_machine *machine);
bool                  state_machine_compile(struct state_machine *machine);
struct state         *state_machine_create_state(struct state_machine *state_machine, const char *name);

bool                  state_add_timeout_transition(struct state *state, struct time *timeout, struct transition_data *data);
bool                  state_set_error_transition(struct state *state, struct transition_data *data);
bool                  state_set_input_transition(struct state *state, struct transition_data *data);
bool                  state_set_enter_transition(struct state *state, struct transition_data *data);
bool                  state_set_leave_transition(struct state *state, struct transition_data *data);

struct state_machine_instance *state_machine_instance(struct state_machine *machine);
struct state_machine *state_machine_instance_get(struct state_machine_instance *instance);
void                  state_machine_instance_finish(struct state_machine_instance *instance);
void                  state_machine_instance_destroy(struct state_machine_instance *instance);
void                  state_machine_instance_update(struct state_machine_instance *instance, struct state *newstate);
void                  state_machine_instance_error(struct state_machine_instance *instance);
void                  state_machine_instance_input(struct state_machine_instance *instance, void *input);

#endif /* _STATE_MACHINE_H */
