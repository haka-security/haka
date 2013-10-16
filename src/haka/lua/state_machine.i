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
	struct lua_ref           function;
};

struct lua_state_machine_context {
	struct state_machine_context   super;
	struct lua_ref                 states;
};

void lua_state_machine_context_destroy(struct state_machine_context *_context)
{
	struct lua_state_machine_context *context = (struct lua_state_machine_context *)_context;
	lua_ref_clear(&context->states);
	free(context);
}

struct state *lua_transition_callback(struct state_machine_instance *state_machine, struct transition_data *_data)
{
	struct lua_transition_data *data = (struct lua_transition_data *)_data;
	struct lua_state_machine_context *context = (struct lua_state_machine_context *)state_machine_instance_context(state_machine);
	struct state *newstate = NULL;
	lua_State *L = data->function.state->L;

	assert(lua_ref_isvalid(&context->states));
	assert(lua_ref_isvalid(&data->function));

	lua_ref_push(L, &data->function);
	lua_ref_push(L, &context->states);
	lua_call(L, 1, 1);

	if (!lua_isnil(L, -1)) {
		if (SWIG_IsOK(SWIG_ConvertPtr(L, -1, (void**)&newstate, SWIGTYPE_p_state, 0))) {
			lua_pop(L, 1);
		}
		else {
			if (!lua_istable(L, -1)) {
				message(HAKA_LOG_ERROR, L"state machine", L"transition failed, invalid state");
			}
			else {
				lua_getfield(L, -1, "_state");
				assert(!lua_isnil(L, -1));

				if (!SWIG_IsOK(SWIG_ConvertPtr(L, -1, (void**)&newstate, SWIGTYPE_p_state, 0))) {
					message(HAKA_LOG_ERROR, L"state machine", L"transition failed, invalid state");
				}

				lua_pop(L, 2);
			}
		}
	}
	else {
		lua_pop(L, 1);
	}

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
		state_machine_instance_update(data->state_machine, newstate);
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
	lua_ref_clear(&data->function);
	free(_data);
}

enum lua_transition_type {
	TIMEOUT_TRANSITION,
	OTHER_TRANSITION,
};

static struct transition_data *lua_transition_data_new(struct lua_ref *func, enum lua_transition_type type)
{
	struct lua_transition_data *ret = malloc(sizeof(struct lua_transition_data));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	memset(ret, 0, sizeof(struct lua_transition_data));

	switch (type) {
	case TIMEOUT_TRANSITION: ret->data.callback = lua_transition_timeout_callback; break;
	case OTHER_TRANSITION:   ret->data.callback = lua_transition_callback; break;
	default:                 assert(0); break;
	}

	ret->data.destroy = lua_transition_data_destroy;
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
		void transition_timeout(double secs, struct lua_ref func)
		{
			struct time timeout;
			struct transition_data *trans = lua_transition_data_new(&func, TIMEOUT_TRANSITION);
			if (!trans) return;

			time_build(&timeout, secs);
			state_add_timeout_transition($self, &timeout, trans);
		}

		void transition_error(struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&func, OTHER_TRANSITION);
			if (!trans) return;

			state_set_error_transition($self, trans);
		}

		void transition_leave(struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&func, OTHER_TRANSITION);
			if (!trans) return;

			state_set_leave_transition($self, trans);
		}

		void transition_enter(struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&func, OTHER_TRANSITION);
			if (!trans) return;

			state_set_enter_transition($self, trans);
		}

		void transition_init(struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&func, OTHER_TRANSITION);
			if (!trans) return;

			state_set_init_transition($self, trans);
		}

		void transition_finish(struct lua_ref func)
		{
			struct transition_data *trans = lua_transition_data_new(&func, OTHER_TRANSITION);
			if (!trans) return;

			state_set_finish_transition($self, trans);
		}

		%immutable;
		const char *name;
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

		struct state_machine_instance *instanciate(struct lua_ref states)
		{
			struct lua_state_machine_context *context = malloc(sizeof(struct lua_state_machine_context));
			if (!context) {
				error(L"memory error");
				return NULL;
			}

			context->super.destroy = lua_state_machine_context_destroy;
			context->states = states;

			return state_machine_instance($self, &context->super);
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
			state_machine_instance_update($self, state);
		}

		%rename(finish) _finish;
		void _finish()
		{
			state_machine_instance_finish($self);
		}

		%immutable;
		const char *state;
	}
};

STRUCT_UNKNOWN_KEY_ERROR(state_machine_instance);


%{

const char *state_name_get(struct state *state)
	{ return state_name(state); }

const char *state_machine_instance_state_get(struct state_machine_instance *instance)
{
	struct state *current = state_machine_instance_state(instance);
	if (current) return state_name(current);
	else return NULL;
}

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
}
