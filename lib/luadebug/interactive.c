
#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <luadebug/user.h>
#include <luadebug/interactive.h>
#include "complete.h"
#include "utils.h"


struct luadebug_interactive
{
	lua_State                   *L;
	int                          env_index;
	struct luadebug_complete     complete;
};

#define EOF_MARKER   "'<eof>'"

static mutex_t active_session_mutex = MUTEX_INIT;
static struct luadebug_interactive *current_session;
static struct luadebug_user        *current_user = NULL;

static const complete_callback completions[] = {
	&complete_callback_lua_keyword,
	&complete_callback_global,
	&complete_callback_fenv,
	&complete_callback_table,
	&complete_callback_swig_get,
	&complete_callback_swig_fn,
	NULL
};

static char *generator(const char *text, int state)
{
	return complete_generator(current_session->L, &current_session->complete,
			completions, text, state);
}

static generator_callback *completion(const char *line, int start)
{
	return generator;
}

void luadebug_interactive_user(struct luadebug_user *user)
{
	mutex_lock(&active_session_mutex);

	if (current_user) {
		current_user->destroy(current_user);
		current_user = NULL;
	}

	current_user = user;

	mutex_unlock(&active_session_mutex);
}

static void on_user_error()
{
	if (current_user) {
		current_user->destroy(current_user);
		current_user = NULL;
	}
}

void luadebug_interactive_enter(struct lua_State *L, const char *single, const char *multi,
		const char *msg)
{
	char *line, *full_line = NULL;
	bool multiline = false;
	bool stop = false;
	struct luadebug_interactive session;

	mutex_lock(&active_session_mutex);

	if (!current_user) {
		message(HAKA_LOG_ERROR, L"interactive", L"no input/output handler");
		mutex_unlock(&active_session_mutex);
		return;
	}

	if (!single) single = GREEN BOLD ">  " CLEAR;
	if (!multi) multi = GREEN BOLD ">> " CLEAR;

	current_user->completion = completion;

	if (!current_user->start(current_user, "haka")) {
		on_user_error();
		mutex_unlock(&active_session_mutex);
		return;
	}

	current_session = &session;
	LUA_STACK_MARK(L);

	current_user->print(current_user, GREEN "entering interactive session" CLEAR ": %s", msg);

	session.L = L;
	session.env_index = capture_env(L, 1);
	assert(session.env_index != -1);

	session.complete.stack_env = session.env_index;

	while (!stop && (line = current_user->readline(current_user, multiline ? multi : single))) {
		int status = -1;
		bool is_return = false;
		char *current_line;

		if (multiline) {
			const size_t full_line_size = strlen(full_line);

			assert(full_line);

			full_line = realloc(full_line, full_line_size + 1 + strlen(line) + 1);
			full_line[full_line_size] = ' ';
			strcpy(full_line + full_line_size + 1, line);

			current_line = full_line;
		}
		else {
			current_user->addhistory(current_user, line);
			current_line = line;
		}

		while (*current_line == ' ' || *current_line == '\t') {
			++current_line;
		}

		is_return = (strncmp(current_line, "return ", 7) == 0) ||
				(strcmp(current_line, "return") == 0);

		if (!multiline && !is_return) {
			char *return_line = malloc(7 + strlen(current_line) + 1);
			strcpy(return_line, "return ");
			strcpy(return_line+7, current_line);

			status = luaL_loadbuffer(L, return_line, strlen(return_line), "stdin");
			if (status) {
				lua_pop(L, 1);
				status = -1;
			}

			free(return_line);
		}

		if (status == -1) {
			status = luaL_loadbuffer(L, current_line, strlen(current_line), "stdin");
		}

		multiline = false;

		/* Change the chunk environment */
		lua_pushvalue(L, session.env_index);
		lua_setfenv(L, -2);

		if (status == LUA_ERRSYNTAX) {
			const char *message;
			const int k = sizeof(EOF_MARKER) / sizeof(char) - 1;
			size_t n;

			message = lua_tolstring(L, -1, &n);

			if (n > k &&
				!strncmp(message + n - k, EOF_MARKER, k) &&
				strlen(line) > 0) {
				multiline = true;
			} else {
				current_user->print(current_user, RED "%s" CLEAR "\n", lua_tostring(L, -1));
			}

			lua_pop(L, 1);
		}
		else if (status == LUA_ERRMEM) {
			current_user->print(current_user, RED "%s" CLEAR "\n", lua_tostring(L, -1));

			lua_pop(L, 1);
		}
		else {
			execute_print(L, current_user);
			lua_pop(L, 1);

			if (is_return) {
				stop = true;
			}
		}

		if (!multiline) {
			free(full_line);
			full_line = NULL;
		}
		else if (!full_line) {
			full_line = strdup(line);
		}

		free(line);
	}

	free(full_line);

	current_user->print(current_user, "\n");
	current_user->print(current_user, GREEN "continue" CLEAR "\n");

	lua_pop(L, 1);
	LUA_STACK_CHECK(L, 0);

	if (!current_user->stop(current_user)) {
		on_user_error();
	}

	current_session = NULL;

	mutex_unlock(&active_session_mutex);
}

FINI void _luadebug_interactive_fini()
{
	if (current_user) {
		current_user->destroy(current_user);
		current_user = NULL;
	}
}
