
#include <assert.h>
#include <stdlib.h>
#include <lua.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <haka/log.h>
#include <haka/packet_module.h>
#include <haka/error.h>
#include <haka/thread.h>

#include "thread.h"
#include "app.h"


struct thread_state {
	int                         thread_id;
	struct packet_module       *packet_module;
	struct packet_module_state *capture;
	lua_state                  *lua;
	int                         lua_function;
	pthread_t                   thread;
	struct thread_pool         *pool;
};

struct thread_pool {
	int                         count;
	bool                        single;
	struct thread_state       **threads;
};

extern void lua_pushppacket(lua_State *L, struct packet *pkt);

static void filter_wrapper(struct thread_state *state, struct packet *pkt)
{
	lua_pushcfunction(state->lua, lua_error_formater);
	lua_getglobal(state->lua, "haka");
	lua_getfield(state->lua, -1, "filter");
	lua_pushppacket(state->lua, pkt);
	if (lua_pcall(state->lua, 1, 0, 1)) {
		print_error(state->lua, L"filter function");
		packet_drop(pkt);
	}
}

static void cleanup_thread_state(struct thread_state *state)
{
	assert(state);
	assert(state->packet_module);

	if (state->lua) {
		cleanup_state(state->lua);
		state->lua = NULL;
	}

	if (state->capture) {
		state->packet_module->cleanup_state(state->capture);
		state->capture = NULL;
	}

	free(state);
}

static struct thread_state *init_thread_state(struct packet_module *packet_module, int thread_id)
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

	state->lua = init_state();
	if (!state->lua) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create lua state");
		cleanup_thread_state(state);
		return NULL;
	}

	state->capture = packet_module->init_state(thread_id);
	if (!state->capture) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create packet capture state");
		cleanup_thread_state(state);
		return NULL;
	}

	lua_getglobal(state->lua, "require");
	lua_pushstring(state->lua, "rule");
	if (lua_pcall(state->lua, 1, 0, 0)) {
		print_error(state->lua, NULL);
		cleanup_thread_state(state);
		return NULL;
	}

	if (run_file(state->lua, get_configuration_script(), 0, NULL)) {
		cleanup_thread_state(state);
		return NULL;
	}

	lua_getglobal(state->lua, "haka");
	lua_getfield(state->lua, -1, "rule_summary");
	if (lua_pcall(state->lua, 0, 0, 0)) {
		print_error(state->lua, NULL);
		cleanup_thread_state(state);
		return NULL;
	}
	lua_pop(state->lua, 2);

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
		if (pthread_sigmask(SIG_BLOCK, &set, NULL)) {
			messagef(HAKA_LOG_FATAL, L"core", L"thread initialization error: %s", errno_error(errno));
			return NULL;
		}
	}

	struct packet_module *packet_module = get_packet_module();
	assert(packet_module);

	thread_set_id(state->thread_id);

	while (packet_module->receive(state->capture, &pkt) == 0) {
		/* The packet can be NULL in case of failure in packet receive */
		if (pkt) {
			filter_wrapper(state, pkt);
			pkt = NULL;
		}
	}

	return NULL;
}

static void start_thread(struct thread_state *state)
{
	int err;

	err = pthread_create(&state->thread, NULL, thread_main_loop, state);
	if (err) {
		error(L"thread creation error: %s", errno_error(err));
		return;
	}
}

static void start_single(struct thread_state *state)
{
	thread_main_loop(state);
}

struct thread_pool *thread_pool_create(int count, struct packet_module *packet_module)
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
		pool->threads[i] = init_thread_state(packet_module, i);
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
		pthread_join(pool->threads[i]->thread, &ret);
	}
}

void thread_pool_cancel(struct thread_pool *pool)
{
	if (!pool->single) {
		int i;

		for (i=0; i<pool->single; ++i) {
			pthread_cancel(pool->threads[i]->thread);
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
