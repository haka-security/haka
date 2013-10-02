%module state_machine

%{
#include <haka/state_machine.h>
#include <haka/time.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/lua/lua.h>
#include <haka/lua/ref.h>
#include <haka/lua/state.h>


struct lua_transition_data {
	struct transition_data   data;
	struct lua_ref           states;
	struct lua_ref           function;
};

struct state *lua_transition_callback(struct state_machine_instance *state_machine, struct transition_data *_data)
{
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	struct state *newstate = NULL;
	lua_State *L = data->function.state->L;

	assert(lua_ref_isvalid(&data->states));
	assert(lua_ref_isvalid(&data->function));

	lua_ref_push(L, &data->function);
	lua_ref_push(L, &data->states);
	//SWIG_NewPointerObj(L, state_machine, SWIGTYPE_p_state_machine, 0);
	lua_call(L, 1, 1);

	if (!SWIG_IsOK(SWIG_ConvertPtr(L, -1, (void**)&newstate, SWIGTYPE_p_state, 0))) {
		message(HAKA_LOG_ERROR, L"state machine", L"transition failed, invalid state");
	}

	lua_pop(L, 1);
	return newstate;
}

struct lua_transtion_deferred_data {
	struct state_machine_instance *state_machine;
	struct lua_transition_data    *data;
};

static void lua_transtion_deferred_data_destroy(void *data)
{
	free(data);
}

int lua_transition_deferred(lua_State *L)
{
	struct state *newstate;
	const struct lua_transtion_deferred_data *data;

	assert(lua_islightuserdata(L, -1));
	data = (const struct lua_transtion_deferred_data *)lua_topointer(L, -1);
	assert(data);

	newstate = lua_transition_callback(data->state_machine, &data->data->data);
	if (newstate) {
		state_machine_instance_update(data->state_machine, newstate, "timeout trigger");
	}

	if (check_error()) {
		lua_pushwstring(L, clear_error());
		lua_error(L);
	}

	return 0;
}

struct state *lua_transition_timeout_callback(struct state_machine_instance *state_machine, struct transition_data *_data)
{
	struct lua_transtion_deferred_data *deferred_data;
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	struct lua_state *state = data->function.state;

	deferred_data = malloc(sizeof(struct lua_transtion_deferred_data));
	if (!deferred_data) {
		error(L"memory error");
		return NULL;
	}

	deferred_data->state_machine = state_machine;
	deferred_data->data = data;

	lua_state_interrupt(state, lua_transition_deferred, deferred_data, lua_transtion_deferred_data_destroy);
	return NULL;
}

static void lua_transition_data_destroy(struct transition_data *_data)
{
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	lua_ref_clear(&data->states);
	lua_ref_clear(&data->function);
	free(_data);
}

static struct transition_data *lua_transition_data_new(struct lua_ref *states, struct lua_ref *func,
		bool timeout)
{
	struct lua_transition_data *ret = malloc(sizeof(struct lua_transition_data));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->data.callback = timeout ? lua_transition_timeout_callback : lua_transition_callback;
	ret->data.destroy = lua_transition_data_destroy;
	ret->states = *states;
	ret->function = *func;

	return &ret->data;
}

%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"
%include "haka/lua/ref.si"


%nodefaultctor;
%nodefaultdtor;

struct state {
	%extend {
		void transition_timeout(double secs, struct lua_ref states, struct lua_ref func)
		{
			struct time timeout;
			struct transition_data *trans = lua_transition_data_new(&states, &func, true);
			if (!trans) return;

			time_build(&timeout, secs);
			state_add_timeout_transition($self, &timeout, trans);
		}

		void transition_error(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func, false);
			if (!trans) return;

			state_set_error_transition($self, trans);
		}

		void transition_input(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func, false);
			if (!trans) return;

			state_set_input_transition($self, trans);
		}

		void transition_leave(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func, false);
			if (!trans) return;

			state_set_leave_transition($self, trans);
		}

		void transition_enter(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func, false);
			if (!trans) return;

			state_set_enter_transition($self, trans);
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(state);


%newobject state_machine::instanciate;

struct state_machine {
	%extend {
		state_machine(const char *name)
		{
			if (!name) {
				error(L"missing name parameter");
				return NULL;
			}

			return state_machine_create(name);
		}

		~state_machine()
		{
			state_machine_destroy($self);
		}

		%rename(create_state) _create_state;
		struct state *_create_state(const char *name)
		{
			return state_machine_create_state($self, name);
		}

		%rename(compile) _compile;
		void _compile()
		{
			state_machine_compile($self);
		}

		struct state_machine_instance *instanciate()
		{
			return state_machine_instance($self);
		}

		struct state *initial;

		%immutable;
		struct state *error_state;
		struct state *finish_state;
	}
};

STRUCT_UNKNOWN_KEY_ERROR(state_machine);


struct state_machine_instance {
	%extend {
		~state_machine_instance()
		{
			state_machine_instance_destroy($self);
		}

		%rename(update) _update;
		void _update(struct state *state)
		{
			state_machine_instance_update($self, state, NULL);
		}

		%rename(finish) _finish;
		void _finish()
		{
			state_machine_instance_finish($self);
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(state_machine_instance);


%{

struct state *state_machine_initial_get(struct state_machine *machine)
	{ return NULL; }

void state_machine_initial_set(struct state_machine *machine, struct state *initial)
	{ state_machine_set_initial(machine, initial); }

struct state *state_machine_error_state_get(struct state_machine *machine)
	{ return state_machine_error_state; }

struct state *state_machine_finish_state_get(struct state_machine *machine)
	{ return state_machine_finish_state; }

%}

%luacode {
	local this = unpack({...})

	local state_machine_lua = require('state_machine')

	this.new = state_machine_lua.new
	this.on_input = state_machine_lua.on_input
	this.on_timeout = state_machine_lua.on_timeout
	this.on_error = state_machine_lua.on_error
	this.on_enter = state_machine_lua.on_enter
	this.on_leave = state_machine_lua.on_leave
}
