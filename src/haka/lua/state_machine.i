%module state_machine

%{
#include <haka/state_machine.h>
#include <haka/time.h>
#include <haka/error.h>
#include <haka/lua/ref.h>
#include <haka/lua/state.h>


struct lua_transition_data {
	struct transition_data   data;
	struct lua_ref           states;
	struct lua_ref           function;
};

void lua_transition_callback(struct state_machine_instance *state_machine, struct transition_data *_data)
{
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	struct state *newstate;
	lua_State *L = data->function.state->L;

	assert(lua_ref_isvalid(&data->states));
	assert(lua_ref_isvalid(&data->function));

	lua_ref_push(L, &data->function);
	lua_ref_push(L, &data->states);
	//SWIG_NewPointerObj(L, state_machine, SWIGTYPE_p_state_machine, 0);
	lua_call(L, 1, 1);

	if (SWIG_IsOK(SWIG_ConvertPtr(L, -1, (void**)&newstate, SWIGTYPE_p_state, 0))) {
		printf("%p\n", newstate);
		if (newstate) {
			state_machine_instance_update(state_machine, newstate);
		}
	}

	lua_pop(L, 1);
}

static void lua_transition_data_destroy(struct transition_data *_data)
{
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	lua_ref_clear(&data->states);
	lua_ref_clear(&data->function);
	free(_data);
}

static struct transition_data *lua_transition_data_new(struct lua_ref *states, struct lua_ref *func)
{
	struct lua_transition_data *ret = malloc(sizeof(struct lua_transition_data));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->data.callback = lua_transition_callback;
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
			struct transition_data *trans = lua_transition_data_new(&states, &func);
			if (!trans) return;

			time_build(&timeout, secs);
			state_add_timeout_transition($self, &timeout, trans);
		}

		void transition_error(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func);
			if (!trans) return;

			state_set_error_transition($self, trans);
		}

		void transition_input(struct lua_ref states, struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&states, &func);
			if (!trans) return;

			state_set_input_transition($self, trans);
		}
	}
};

%newobject state_machine::instanciate;

struct state_machine {
	%extend {
		state_machine()
		{
			return state_machine_create();
		}

		~state_machine()
		{
			state_machine_destroy($self);
		}

		%rename(create_state) _create_state;
		struct state *_create_state()
		{
			return state_machine_create_state($self, NULL);
		}

		struct state_machine_instance *instanciate()
		{
			return state_machine_instance($self);
		}
	}
};

struct state_machine_instance {
	%extend {
		~state_machine_instance()
		{
			state_machine_instance_destroy($self);
		}

		%rename(update) _update;
		void _update(struct state *state)
		{
			state_machine_instance_update($self, state);
		}
	}
};


%luacode {
	local this = unpack({...})

	local state_machine_lua = require('state_machine')

	this.new = state_machine_lua.new
	this.on_input = state_machine_lua.on_input
	this.on_timeout = state_machine_lua.on_timeout
	this.on_error = state_machine_lua.on_error
}
