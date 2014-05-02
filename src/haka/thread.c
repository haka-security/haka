/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <lua.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <haka/log.h>
#include <haka/packet_module.h>
#include <haka/error.h>
#include <haka/thread.h>
#include <haka/stat.h>
#include <haka/timer.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>
#include <haka/luadebug/debugger.h>

#include "thread.h"
#include "app.h"


struct pkt_stats {
	size_t                      total_packets;
	size_t                      total_bytes;
};

enum {
	STATE_NOTSARTED = 0,
	STATE_ERROR,
	STATE_FINISHED,
	STATE_RUNNING,
	STATE_CANCELED,
	STATE_JOINED,
};

struct thread_state {
	int                         thread_id;
	int                         state;
	struct packet_module       *packet_module;
	struct packet_module_state *capture;
	struct lua_state           *lua;
	int                         lua_function;
	thread_t                    thread;
	bool                        canceled;
	int32                       attach_debugger;
	struct thread_pool         *pool;
	struct pkt_stats            stats;
};

struct thread_pool {
	int                         count;
	bool                        single;
	int32                       attach_debugger;
	barrier_t                   thread_start_sync;
	barrier_t                   thread_sync;
	struct thread_state       **threads;
};

extern bool lua_pushppacket(lua_State *L, struct packet *pkt);

static void filter_wrapper(struct thread_state *state, struct packet *pkt)
{
	int h;
	LUA_STACK_MARK(state->lua->L);

	packet_addref(pkt);

	lua_pushcfunction(state->lua->L, lua_state_error_formater);
	h = lua_gettop(state->lua->L);

	lua_getglobal(state->lua->L, "haka");
	lua_getfield(state->lua->L, -1, "filter");

	if (!lua_isnil(state->lua->L, -1)) {
		if (!lua_pushppacket(state->lua->L, pkt)) {
			message(HAKA_LOG_ERROR, L"core", L"packet internal error");
			packet_drop(pkt);
		}
		else {
			if (lua_pcall(state->lua->L, 1, 0, h)) {
				lua_state_print_error(state->lua->L, L"filter");
				packet_drop(pkt);
			}
		}

		if (packet_mode() == MODE_PASSTHROUGH &&
			packet_state(pkt) == STATUS_NORMAL) {
			message(HAKA_LOG_WARNING, L"core", L"pass-through error: packet has been blocked");
		}
	}
	else {
		lua_pop(state->lua->L, 1);
		packet_drop(pkt);
	}

	lua_pop(state->lua->L, 2);
	LUA_STACK_CHECK(state->lua->L, 0);

	packet_release(pkt);
}

static void cleanup_thread_state_lua(struct thread_state *state)
{
	assert(state);
	assert(state->packet_module);

	if (state->lua) {
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
	state->stats.total_packets = 0;
	state->stats.total_bytes = 0;
	state->packet_module = packet_module;
	state->state = STATE_NOTSARTED;

	messagef(HAKA_LOG_INFO, L"core", L"initializing thread %d", thread_id);

	state->lua = lua_state_init();
	if (!state->lua) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create lua state");
		cleanup_thread_state(state);
		return NULL;
	}

	/* Load Lua sources */
	lua_state_require(state->lua, "rule");
	lua_state_require(state->lua, "rule_group");
	lua_state_require(state->lua, "interactive");
	lua_state_require(state->lua, "protocol/raw");

	state->capture = packet_module->init_state(thread_id);
	if (!state->capture) {
		message(HAKA_LOG_FATAL, L"core", L"unable to create packet capture state");
		cleanup_thread_state(state);
		return NULL;
	}

	return state;
}

static bool init_thread_lua_state(struct thread_state *state)
{
	int h;
	LUA_STACK_MARK(state->lua->L);

	if (state->pool->attach_debugger > state->attach_debugger) {
		luadebug_debugger_start(state->lua->L, false);
	}
	state->pool->attach_debugger = state->attach_debugger;

	lua_pushcfunction(state->lua->L, lua_state_error_formater);
	h = lua_gettop(state->lua->L);

	lua_getglobal(state->lua->L, "require");
	lua_pushstring(state->lua->L, "rule");
	if (lua_pcall(state->lua->L, 1, 0, h)) {
		lua_state_print_error(state->lua->L, L"init");
		lua_pop(state->lua->L, 1);
		return false;
	}

	if (!lua_state_run_file(state->lua, get_configuration_script(), 0, NULL)) {
		lua_pop(state->lua->L, 1);
		return false;
	}

	lua_getglobal(state->lua->L, "haka");
	lua_getfield(state->lua->L, -1, "rule_summary");
	if (lua_pcall(state->lua->L, 0, 0, h)) {
		lua_state_print_error(state->lua->L, L"init");
		lua_pop(state->lua->L, 1);
		return false;
	}
	lua_pop(state->lua->L, 2);

	LUA_STACK_CHECK(state->lua->L, 0);
	return true;
}

static void *thread_main_loop(void *_state)
{
	struct thread_state *state = (struct thread_state *)_state;
	struct packet *pkt = NULL;
	sigset_t set;

	thread_setid(state->thread_id);

	if (!state->pool->single) {
		/* Block all signal to let the main thread handle them */
		sigfillset(&set);
		sigdelset(&set, SIGSEGV);
		sigdelset(&set, SIGILL);
		sigdelset(&set, SIGFPE);

		if (!thread_sigmask(SIG_BLOCK, &set, NULL)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			barrier_wait(&state->pool->thread_start_sync);
			state->state = STATE_ERROR;
			return NULL;
		}

		if (!timer_init_thread()) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			barrier_wait(&state->pool->thread_start_sync);
			state->state = STATE_ERROR;
			return NULL;
		}

		/* To make sure we can still cancel even if some thread are locked in
		 * infinite loops */
		if (!thread_setcanceltype(THREAD_CANCEL_ASYNCHRONOUS)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			barrier_wait(&state->pool->thread_start_sync);
			state->state = STATE_ERROR;
			return NULL;
		}

		if (!init_thread_lua_state(state)) {
			barrier_wait(&state->pool->thread_start_sync);
			state->state = STATE_ERROR;
			return NULL;
		}
	}

	packet_init(state->capture);

	if (!state->pool->single) {
		if (!barrier_wait(&state->pool->thread_start_sync)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			state->state = STATE_ERROR;
			return NULL;
		}
	}

	if (!state->pool->single) {
		if (!barrier_wait(&state->pool->thread_sync)) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			state->state = STATE_ERROR;
			return NULL;
		}
	}

	while (packet_receive(&pkt) == 0) {
		/* The packet can be NULL in case of failure in packet receive */
		if (pkt) {
			state->stats.total_packets++;
			state->stats.total_bytes += vbuffer_size(packet_payload(pkt));

			filter_wrapper(state, pkt);
			pkt = NULL;
		}

		lua_state_runinterrupt(state->lua);

		if (state->pool->attach_debugger > state->attach_debugger) {
			luadebug_debugger_start(state->lua->L, true);
			state->attach_debugger = state->pool->attach_debugger;
		}
	}

	state->state = STATE_FINISHED;
	return NULL;
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
	pool->single = count == 1;

	if (!barrier_init(&pool->thread_sync, count+1)) {
		thread_pool_cleanup(pool);
		return NULL;
	}

	if (!barrier_init(&pool->thread_start_sync, 2)) {
		thread_pool_cleanup(pool);
		return NULL;
	}

	if (attach_debugger) {
		thread_pool_attachdebugger(pool);
	}

	for (i=0; i<count; ++i) {
		pool->threads[i] = init_thread_state(packet_module, i);
		if (!pool->threads[i]) {
			error(L"thread initialization error");
			thread_pool_cleanup(pool);
			return NULL;
		}

		pool->threads[i]->pool = pool;

		if (pool->single) {
			if (!init_thread_lua_state(pool->threads[i])) {
				error(L"thread initialization error");
				thread_pool_cleanup(pool);
				return NULL;
			}
		}
		else {
			if (!thread_create(&pool->threads[i]->thread, thread_main_loop, pool->threads[i])) {
				thread_pool_cleanup(pool);
				return NULL;
			}

			pool->threads[i]->state = STATE_RUNNING;

			if (!barrier_wait(&pool->thread_start_sync)) {
				thread_pool_cleanup(pool);
				return NULL;
			}

			if (pool->threads[i]->state == STATE_ERROR) {
				error(L"thread initialization error");
				thread_pool_cleanup(pool);
				return NULL;
			}
		}
	}

	return pool;
}

void thread_pool_cleanup(struct thread_pool *pool)
{
	int i;

	if (!pool->single) {
		thread_pool_cancel(pool);
	}

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

	barrier_destroy(&pool->thread_sync);
	barrier_destroy(&pool->thread_start_sync);

	free(pool->threads);
	free(pool);
}

void thread_pool_wait(struct thread_pool *pool)
{
	int i;

	for (i=0; i<pool->count; ++i) {
		if (pool->threads[i] && pool->threads[i]->state != STATE_NOTSARTED &&
		    pool->threads[i]->state != STATE_JOINED) {
			void *ret;
			if (!thread_join(pool->threads[i]->thread, &ret)) {
				message(HAKA_LOG_FATAL, L"core", clear_error());
			}
			pool->threads[i]->state = STATE_JOINED;
		}
	}
}

void thread_pool_cancel(struct thread_pool *pool)
{
	if (!pool->single) {
		int i;

		for (i=0; i<pool->count; ++i) {
			if (pool->threads[i] && pool->threads[i]->state == STATE_RUNNING) {
				if (!thread_cancel(pool->threads[i]->thread)) {
					message(HAKA_LOG_FATAL, L"core", clear_error());
				}
				pool->threads[i]->state = STATE_CANCELED;
			}
		}
	}
}

bool thread_pool_issingle(struct thread_pool *pool)
{
	return pool->single;
}

void thread_pool_start(struct thread_pool *pool)
{
	if (pool->count == 1) {
		assert(pool->threads[0]);
		thread_main_loop(pool->threads[0]);
	}
	else if (pool->count > 1) {
		barrier_wait(&pool->thread_sync);
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

static const size_t columnsize = 20;

void pad_output_stat_bytes(FILE *f, size_t value)
{
	const size_t len = stat_print_formated_bytes(f, value);
	if (len != (size_t)-1) {
		stat_printf(f, "%*s", columnsize-len, "");
	}
}

void pad_output_stat_chars(FILE *f, char *s)
{
	const size_t len = stat_printf(f, s);
	if (len != (size_t)-1) {
		stat_printf(f, "%*s", columnsize-len, "");
	}
}

void thread_pool_dump_stat(struct thread_pool *pool, FILE *f)
{
	size_t total_packets = 0, total_bytes = 0;
	size_t thread_packets, thread_bytes;

	assert(pool);

	pad_output_stat_chars(f, "");
	pad_output_stat_chars(f, "Packets");
	pad_output_stat_chars(f, "Bytes");
	stat_printf(f, "\n");
	int i;
	for (i = 0; i < pool->count; i++) {
		const size_t len = stat_printf(f, "Thread %d", i+1);
		if (len != (size_t)-1) {
			stat_printf(f, "%*s", columnsize-len, "");
		}
		thread_packets = pool->threads[i]->stats.total_packets;
		thread_bytes = pool->threads[i]->stats.total_bytes;
		pad_output_stat_bytes(f, thread_packets);
		pad_output_stat_bytes(f, thread_bytes);
		stat_printf(f, "\n");

		total_packets += thread_packets;
		total_bytes += thread_bytes;
	}
	pad_output_stat_chars(f, "All Threads");
	pad_output_stat_bytes(f, total_packets);
	pad_output_stat_bytes(f, total_bytes);
	stat_printf(f, "\n");
}
