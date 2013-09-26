
#include <assert.h>
#include <stdlib.h>
#include <lua.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <haka/log.h>
#include <haka/packet_module.h>
#include <haka/error.h>
#include <haka/thread.h>
#include <haka/timer.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>
#include <luadebug/debugger.h>

#include "thread.h"
#include "app.h"


struct thread_state {
	int                         thread_id;
	struct packet_module       *packet_module;
	struct packet_module_state *capture;
	struct lua_state           *lua;
	int                         lua_function;
	thread_t                    thread;
	int32                       attach_debugger;
	struct thread_pool         *pool;
};

struct thread_pool {
	int                         count;
	bool                        single;
	int32                       attach_debugger;
	struct thread_state       **threads;
};

extern void lua_pushppacket(lua_State *L, struct packet *pkt);

static void filter_wrapper(struct thread_state *state, struct packet *pkt)
{
	int h;
	LUA_STACK_MARK(state->lua->L);

	packet_addref(pkt);

	lua_pushcfunction(state->lua->L, lua_state_error_formater);
	h = lua_gettop(state->lua->L);

	lua_getglobal(state->lua->L, "haka");
	lua_getfield(state->lua->L, -1, "filter");
	lua_pushppacket(state->lua->L, pkt);

	if (lua_pcall(state->lua->L, 1, 0, h)) {
		lua_state_print_error(state->lua->L, L"filter function");
		packet_drop(pkt);
	}

	lua_pop(state->lua->L, 2);

	LUA_STACK_CHECK(state->lua->L, 0);

	if (packet_mode() == MODE_PASSTHROUGH &&
	    packet_state(pkt) == STATUS_NORMAL) {
		message(HAKA_LOG_WARNING, L"core", L"pass-through error: packet is not transmitted");
	}

	packet_release(pkt);
}

static void lua_on_exit(lua_State *L)
{
	int h;
	LUA_STACK_MARK(L);

	lua_pushcfunction(L, lua_state_error_formater);
	h = lua_gettop(L);

	lua_getglobal(L, "haka");
	lua_getfield(L, -1, "_exiting");

	if (lua_pcall(L, 0, 0, h)) {
		lua_state_print_error(L, L"exit");
	}

	lua_pop(L, 2);

	LUA_STACK_CHECK(L, 0);
}

static void cleanup_thread_state_lua(struct thread_state *state)
{
	assert(state);
	assert(state->packet_module);

	if (state->lua) {
		lua_on_exit(state->lua->L);
		lua_state_close(state->lua);
		state->lua = NULL;
	}
}

static void cleanup_thread_state(struct thread_state *state)
{
	assert(state);
	assert(state->packet_module);

	cleanup_thread_state_lua(state);

	if (state->capture) {
		state->packet_module->cleanup_state(state->capture);
		state->capture = NULL;
	}

	free(state);
}

static struct thread_state *init_thread_state(struct packet_module *packet_module, int thread_id,
		bool attach_debugger)
{
	struct thread_state *state;

	assert(packet_module);

	state = malloc(sizeof(struct thread_state));
	if (!state) {
		return NULL;
	}

	memset(state, 0, sizeof(struct thread_state));

	state->thread_id = thread_id;
	state->packet_module = packet_module;

	messagef(HAKA_LOG_INFO, L"core", L"initializing thread %d", thread_id);

	state->lua = haka_init_state();
	if (!state->lua) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create lua state");
		cleanup_thread_state(state);
		return NULL;
	}

	LUA_STACK_MARK(state->lua->L);

	state->capture = packet_module->init_state(thread_id);
	if (!state->capture) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create packet capture state");
		cleanup_thread_state(state);
		return NULL;
	}

	if (attach_debugger) {
		luadebug_debugger_start(state->lua->L, true);
	}

	lua_getglobal(state->lua->L, "require");
	lua_pushstring(state->lua->L, "rule");
	if (lua_pcall(state->lua->L, 1, 0, 0)) {
		lua_state_print_error(state->lua->L, NULL);
		cleanup_thread_state(state);
		return NULL;
	}

	if (run_file(state->lua->L, get_configuration_script(), 0, NULL)) {
		cleanup_thread_state(state);
		return NULL;
	}

	lua_getglobal(state->lua->L, "haka");
	lua_getfield(state->lua->L, -1, "rule_summary");
	if (lua_pcall(state->lua->L, 0, 0, 0)) {
		lua_state_print_error(state->lua->L, NULL);
		cleanup_thread_state(state);
		return NULL;
	}
	lua_pop(state->lua->L, 1);

	LUA_STACK_CHECK(state->lua->L, 0);
	return state;
}

static void *thread_main_loop(void *_state)
{
	struct thread_state *state = (struct thread_state *)_state;
	struct packet *pkt = NULL;
	sigset_t set;

	if (!state->pool->single) {
		/* Block all signal to let the main thread handle them */
		sigfillset(&set);
		if (!thread_sigmask(SIG_BLOCK, &set, NULL)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			return NULL;
		}

		timer_init_thread();

		/* To make sure we can still cancel even if some thread are locked in
		 * infinite loops */
		if (!thread_setcanceltype(THREAD_CANCEL_ASYNCHRONOUS)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			return NULL;
		}
	}

	thread_set_id(state->thread_id);

	packet_init(state->capture);

	while (packet_receive(&pkt) == 0) {
		/* The packet can be NULL in case of failure in packet receive */
		if (pkt) {
			filter_wrapper(state, pkt);
			pkt = NULL;
		}

		if (state->pool->attach_debugger > state->attach_debugger) {
			luadebug_debugger_start(state->lua->L, true);
			state->attach_debugger = state->pool->attach_debugger;
		}
	}

	return NULL;
}

static void start_thread(struct thread_state *state)
{
	thread_create(&state->thread, thread_main_loop, state);
}

static void start_single(struct thread_state *state)
{
	thread_main_loop(state);
}

struct thread_pool *thread_pool_create(int count, struct packet_module *packet_module,
		bool attach_debugger)
{
	int i;
	struct thread_pool *pool;

	assert(count > 0);

	pool = malloc(sizeof(struct thread_pool));
	if (!pool) {
		error(L"memory error");
		return NULL;
	}

	memset(pool, 0, sizeof(struct thread_pool));

	pool->threads = malloc(sizeof(struct thread_state*)*count);
	if (!pool) {
		error(L"memory error");
		thread_pool_cleanup(pool);
		return NULL;
	}

	memset(pool->threads, 0, sizeof(struct thread_state*)*count);

	pool->count = count;

	for (i=0; i<count; ++i) {
		pool->threads[i] = init_thread_state(packet_module, i, attach_debugger);
		if (!pool->threads[i]) {
			error(L"thread initialization error");
			thread_pool_cleanup(pool);
			return NULL;
		}

		pool->threads[i]->pool = pool;
	}

	return pool;
}

void thread_pool_cleanup(struct thread_pool *pool)
{
	int i;

	/* Clean all Lua states first, to trigger the unload of the
	 * extensions before cleaning the thread capture states.
	 */
	for (i=0; i<pool->count; ++i) {
		if (pool->threads[i]) {
			cleanup_thread_state_lua(pool->threads[i]);
		}
	}

	/* Finalize cleanup.
	 */
	for (i=0; i<pool->count; ++i) {
		if (pool->threads[i]) {
			cleanup_thread_state(pool->threads[i]);
		}
	}

	free(pool->threads);
	free(pool);
}

void thread_pool_wait(struct thread_pool *pool)
{
	int i;

	for (i=0; i<pool->count; ++i) {
		void *ret;
		thread_join(pool->threads[i]->thread, &ret);
	}
}

void thread_pool_cancel(struct thread_pool *pool)
{
	if (!pool->single) {
		int i;

		for (i=0; i<pool->count; ++i) {
			thread_cancel(pool->threads[i]->thread);
		}

		thread_pool_wait(pool);
	}
}

void thread_pool_start(struct thread_pool *pool)
{
	int i;

	if (pool->count == 1) {
		assert(pool->threads[0]);
		pool->single = true;
		start_single(pool->threads[0]);
	}
	else if (pool->count > 1) {
		pool->single = false;
		for (i=0; i<pool->count; ++i) {
			start_thread(pool->threads[i]);
			if (check_error()) {
				return;
			}
		}
		thread_pool_wait(pool);
	}
	else {
		error(L"no thread to run");
	}
}

int thread_pool_count(struct thread_pool *pool)
{
	assert(pool);
	return pool->count;
}

void thread_pool_attachdebugger(struct thread_pool *pool)
{
	assert(pool);
	++pool->attach_debugger;
}
