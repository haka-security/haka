
#define _GNU_SOURCE

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "session.h"
#include "complete.h"
#include "utils.h"
#include "readline.h"


struct luadebug_session
{
	lua_State                   *L;
	char                        *prompt_single;
	char                        *prompt_multi;
	int                          env_index;

	struct luadebug_complete     complete;
};


#define EOF_MARKER   "'<eof>'"


static mutex_t active_session_mutex = MUTEX_INIT;
static struct luadebug_session *current_session;

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
	char *match = complete_generator(current_session->L, &current_session->complete,
			completions, text, state);
	rl_completion_suppress_append = !current_session->complete.space;
	return match;
}

static char **complete(const char *text, int start, int end)
{
	current_session->complete.stack_env = current_session->env_index;
	return rl_completion_matches(text, generator);
}

struct luadebug_session *luadebug_session_create(struct lua_State *L)
{
	struct luadebug_session *ret = malloc(sizeof(struct luadebug_session));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->L = L;
	ret->prompt_single = NULL;
	ret->prompt_multi = NULL;

	luadebug_session_setprompts(ret, GREEN BOLD ">  " CLEAR, GREEN BOLD ">> " CLEAR);

	return ret;
}

void luadebug_session_cleanup(struct luadebug_session *session)
{
	free(session->prompt_single);
	free(session->prompt_multi);
	free(session);
}

void luadebug_session_enter(struct luadebug_session *session)
{
	char *line, *full_line = NULL;
	bool multiline = false;

	mutex_lock(&active_session_mutex);

	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = "haka";
	rl_attempted_completion_function = complete;
	rl_filename_completion_desired = 0;
	rl_completion_display_matches_hook = rl_display_matches;

	current_session = session;
	LUA_STACK_MARK(session->L);

	session->env_index = capture_env(session->L, 1);
	assert(session->env_index != -1);

	while ((line = readline(multiline ? session->prompt_multi : session->prompt_single))) {
		int status = -1;
		char *current_line;

		if (multiline) {
			assert(full_line);

			full_line = realloc(full_line, strlen(full_line) + strlen(line) + 1);
			strcpy(full_line + strlen(full_line), line);

			current_line = full_line;
		}
		else {
			add_history(line);
			current_line = line;
		}

		if (!multiline) {
			char *return_line = malloc(7 + strlen(current_line) + 1);
			strcpy(return_line, "return ");
			strcpy(return_line+7, current_line);

			status = luaL_loadbuffer(session->L, return_line, strlen(return_line), "stdin");
			if (status) {
				lua_pop(session->L, 1);
				status = -1;
			}

			free(return_line);
		}

		if (status == -1) {
			status = luaL_loadbuffer(session->L, current_line, strlen(current_line), "stdin");
		}

		multiline = false;

		/* Change the chunk environment */
		lua_pushvalue(session->L, session->env_index);
		lua_setfenv(session->L, -2);

		if (status == LUA_ERRSYNTAX) {
			const char *message;
			const int k = sizeof(EOF_MARKER) / sizeof(char) - 1;
			size_t n;

			message = lua_tolstring(session->L, -1, &n);

			if (n > k &&
				!strncmp(message + n - k, EOF_MARKER, k) &&
				strlen(line) > 0) {
				multiline = true;
			} else {
				printf(RED "%s" CLEAR "\n", lua_tostring(session->L, -1));
				rl_on_new_line();
			}

			lua_pop(session->L, 1);
		}
		else if (status == LUA_ERRMEM) {
			printf(RED "%s" CLEAR "\n", lua_tostring(session->L, -1));
			rl_on_new_line();

			lua_pop(session->L, 1);
		}
		else {
			execute_print(session->L);
			lua_pop(session->L, 1);
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

	printf("\n");
	rl_on_new_line();

	lua_pop(session->L, 1);
	LUA_STACK_CHECK(session->L, 0);

	current_session = NULL;

	mutex_unlock(&active_session_mutex);
}

void luadebug_session_setprompts(struct luadebug_session *session, const char *single, const char *multi)
{
	free(session->prompt_single);
	free(session->prompt_multi);

	session->prompt_single = strdup(single);
	session->prompt_multi = strdup(multi);
}
