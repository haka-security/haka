/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/engine.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/container/list2.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>
#include <haka/lua/marshal.h>

#include <lua.h>
#include <string.h>
#include <assert.h>
#include <signal.h>


struct remote_launch {
	struct list2_elem   list;
	void              (*callback)(void *);
	void               *data;
	int                 state;
	const wchar_t      *error;
	bool                own_error;
	semaphore_t         sync;
};

struct engine_thread {
	volatile enum thread_status    status;
	volatile struct packet_stats   packet_stats;
	mutex_t                        remote_launch_lock;
	thread_t                       thread;
	int                            id;
	struct lua_State              *lua_state;
	struct list2                   remote_launches;
};

static local_storage_t engine_thread_localstorage;
static struct engine_thread **engine_threads = NULL;
static size_t engine_threads_count = 0;

static void _handler(int sig, siginfo_t *si, void *uc)
{
	/* Nothing to do */
}

INIT static void engine_init()
{
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = _handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		messagef(HAKA_LOG_FATAL, L"engine", L"%s", errno_error(errno));
		abort();
	}

	local_storage_init(&engine_thread_localstorage, NULL);
}

FINI static void engine_fini()
{
	free(engine_threads);
	local_storage_destroy(&engine_thread_localstorage);
}

bool engine_prepare(int thread_count)
{
	engine_threads_count = thread_count;

	engine_threads = malloc(sizeof(struct engine_thread *)*thread_count);
	if (!engine_threads) {
		error(L"memory error");
		return false;
	}

	memset(engine_threads, 0, sizeof(struct engine_thread *)*thread_count);
	return true;
}

struct engine_thread *engine_thread_init(struct lua_State *state, int id)
{
	sigset_t mask;

	struct engine_thread *new = malloc(sizeof(struct engine_thread));
	if (!new) {
		error(L"memory error");
		return NULL;
	}

	memset(new, 0, sizeof(*new));
	new->status = THREAD_RUNNING;
	mutex_init(&new->remote_launch_lock, true);
	new->thread = thread_current();
	new->lua_state = state;
	new->id = id;
	list2_init(&new->remote_launches);
	thread_setid(id);

	sigfillset(&mask);
	sigaddset(&mask, SIGUSR1);
	if (!thread_sigmask(SIG_UNBLOCK, &mask, NULL)) {
		free(new);
		return NULL;
	}

	assert(id < engine_threads_count);
	engine_threads[id] = new;
	local_storage_set(&engine_thread_localstorage, new);
	return new;
}

struct engine_thread *engine_thread_byid(int id)
{
	if (id < engine_threads_count) return engine_threads[id];
	else return NULL;
}

int engine_thread_id(struct engine_thread *thread)
{
	return thread->id;
}

static void remote_launch_release(struct engine_thread *thread, struct remote_launch *current)
{
	semaphore_post(&current->sync);
}

void engine_thread_cleanup(struct engine_thread *thread)
{
	list2_iter end, iter;

	assert(thread);

	mutex_lock(&thread->remote_launch_lock);

	end = list2_end(&thread->remote_launches);
	for (iter = list2_begin(&thread->remote_launches); iter != end; ) {
		struct remote_launch *current = list2_get(iter, struct remote_launch, list);

		current->state = -1;
		current->error = L"aborted";
		current->own_error = false;

		iter = list2_erase(iter);
		remote_launch_release(thread, current);
	}

	mutex_unlock(&thread->remote_launch_lock);
	mutex_destroy(&thread->remote_launch_lock);

	engine_threads[thread->id] = NULL;
	local_storage_set(&engine_thread_localstorage, NULL);
	free((void*)thread);
}

struct engine_thread *engine_thread_current()
{
	return local_storage_get(&engine_thread_localstorage);
}

enum thread_status engine_thread_status(struct engine_thread *thread)
{
	assert(thread);
	return thread->status;
}

enum thread_status engine_thread_update_status(struct engine_thread *thread, enum thread_status status)
{
	assert(thread);
	enum thread_status old = thread->status;
	thread->status = status;
	return old;
}

volatile struct packet_stats *engine_thread_statistics(struct engine_thread *thread)
{
	if (thread) return &thread->packet_stats;
	else return NULL;
}

bool engine_thread_remote_launch(struct engine_thread *thread, void (*callback)(void *), void *data)
{
	struct remote_launch new;
	assert(thread);

	list2_elem_init(&new.list);
	new.callback = callback;
	new.data = data;
	new.state = -1;
	new.error = NULL;
	new.own_error = false;
	semaphore_init(&new.sync, 0);

	mutex_lock(&thread->remote_launch_lock);
	list2_insert(list2_end(&thread->remote_launches), &new.list);
	mutex_unlock(&thread->remote_launch_lock);

	engine_thread_force_unblock(thread);

	semaphore_wait(&new.sync);

	if (new.error) {
		error(new.error);

		if (new.own_error) {
			free((void *)new.error);
		}

		return false;
	}
	else {
		return true;
	}
}

struct lua_remote_launch_data {
	char          *code;
	size_t         size;
	char          *res;
	size_t         res_size;
};

static void _lua_remote_launcher(void *_data)
{
	struct lua_remote_launch_data *data = (struct lua_remote_launch_data *)_data;
	struct engine_thread *thread = engine_thread_current();
	int h;

	assert(thread);
	assert(thread->lua_state);

	LUA_STACK_MARK(thread->lua_state);

	lua_pushcfunction(thread->lua_state, lua_state_error_formater);
	h = lua_gettop(thread->lua_state);

	if (lua_unmarshal(thread->lua_state, data->code, data->size)) {
		if (lua_pcall(thread->lua_state, 0, 1, h)) {
			error(L"%s", lua_tostring(thread->lua_state, -1));
		}
		else {
			data->res = lua_marshal(thread->lua_state, h+1, &data->res_size);
		}
		lua_pop(thread->lua_state, 1);
	}

	lua_pop(thread->lua_state, 1);
	LUA_STACK_CHECK(thread->lua_state, 0);
}

int engine_thread_lua_remote_launch(struct engine_thread *thread, struct lua_State *L, int index)
{
	size_t codesize;
	char *result, *code = lua_marshal(L, index, &codesize);
	if (!code) {
		return -1;
	}

	result = engine_thread_raw_lua_remote_launch(thread, code, &codesize);
	if (check_error()) {
		free(code);
		return -1;
	}

	free(code);

	if (result) {
		if (!lua_unmarshal(L, result, codesize)) {
			free(result);
			return -1;
		}
		free(result);
		return 1;
	}
	else {
		return 0;
	}
}

char* engine_thread_raw_lua_remote_launch(struct engine_thread *thread, const char *code, size_t *size)
{
	struct lua_remote_launch_data data;

	assert(code);
	assert(size);

	data.code = (char *)code;
	data.size = *size;
	data.res = NULL;
	data.res_size = 0;

	messagef(HAKA_LOG_DEBUG, L"engine", L"lua remote launch on thread %d: %llu bytes",
	         engine_thread_id(thread), data.size);

	if (!engine_thread_remote_launch(thread, _lua_remote_launcher, &data)) {
		return NULL;
	}

	messagef(HAKA_LOG_DEBUG, L"engine", L"lua remote launch result on thread %d: %llu bytes",
	         engine_thread_id(thread), data.res_size);

	if (data.res) {
		*size = data.res_size;
		return data.res;
	}
	else {
		return NULL;
	}
}

static void _engine_thread_check_remote_launch(void *_thread)
{
	struct engine_thread *thread = (struct engine_thread *)_thread;

	mutex_lock(&thread->remote_launch_lock);

	const list2_iter end = list2_end(&thread->remote_launches);
	list2_iter iter;
	for (iter = list2_begin(&thread->remote_launches); iter != end; ) {
		struct remote_launch *current = list2_get(iter, struct remote_launch, list);

		messagef(HAKA_LOG_DEBUG, L"engine", L"execute lua remote launch on thread %d",
		         engine_thread_id(thread));

		current->callback(current->data);
		if (check_error()) {
			current->error = wcsdup(clear_error());
			current->own_error = true;
			current->state = -1;

			messagef(HAKA_LOG_DEBUG, L"engine", L"remote launch error on thread %d: %ls",
			         engine_thread_id(thread), current->error);
		}
		else {
			current->state = 0;
		}

		iter = list2_erase(iter);
		remote_launch_release(thread, current);
	}
}

static void _engine_thread_check_remote_cleanup(void *_thread)
{
	struct engine_thread *thread = (struct engine_thread *)_thread;
	mutex_unlock(&thread->remote_launch_lock);
}

void engine_thread_check_remote_launch(struct engine_thread *thread)
{
	if (!list2_empty(&thread->remote_launches)) {
		thread_protect(_engine_thread_check_remote_launch, thread,
				_engine_thread_check_remote_cleanup, thread);
	}
}

void engine_thread_force_unblock(struct engine_thread *thread)
{
	/* Send SIGUSR1 to exit any waiting function (capture function for instance) */
	thread_signal(thread->thread, SIGUSR1);
}
