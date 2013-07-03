
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
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <assert.h>

#include "session.h"
#include "complete.h"


struct luainteract_session
{
	lua_State                   *L;
	char                        *prompt_single;
	char                        *prompt_multi;
	int                          env_index;

	struct luainteract_complete  complete;
};


#define CLEAR   "\e[0m"
#define BOLD    "\e[1m"
#define BLACK   "\e[30m"
#define RED     "\e[31m"
#define GREEN   "\e[32m"
#define YELLOW  "\e[33m"
#define BLUE    "\e[34m"
#define MAGENTA "\e[35m"
#define CYAN    "\e[36m"
#define WHITE   "\e[37m"

#define EOF_MARKER   "'<eof>'"


static mutex_t active_session_mutex = MUTEX_INIT;

static void display_matches(char **matches, int num_matches, int max_length)
{
	printf(BOLD);
	rl_display_match_list(matches, num_matches, max_length);
	printf(CLEAR);
	rl_on_new_line();
}

static const complete_callback completions[] = {
	&complete_callback_lua_keyword,
	&complete_callback_global,
	&complete_callback_fenv,
	&complete_callback_table,
	&complete_callback_swig_get,
	&complete_callback_swig_fn,
	NULL
};

static struct luainteract_session *current_session;

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

struct luainteract_session *luainteract_session_create(struct lua_State *L)
{
	struct luainteract_session *ret = malloc(sizeof(struct luainteract_session));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->L = L;
	ret->prompt_single = NULL;
	ret->prompt_multi = NULL;

	luainteract_session_setprompts(ret, GREEN BOLD ">  " CLEAR, GREEN BOLD ">> " CLEAR);

	return ret;
}

void luainteract_session_cleanup(struct luainteract_session *session)
{
	free(session->prompt_single);
	free(session->prompt_multi);
	free(session);
}

static int luap_call(lua_State *L, int n) {
	int status;
	int h;

	h = lua_gettop(L) - n;
	lua_pushcfunction(L, lua_state_error_formater);
	lua_insert(L, h);

	status = lua_pcall(L, n, LUA_MULTRET, h);
	if (status) {
		printf(RED "%s\n" CLEAR, lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	lua_remove(L, h);

	return status;
}

static int execute(lua_State *L)
{
	int i, g, h, status;

	g = lua_gettop(L);
	status = luap_call(L, 0);
	h = lua_gettop(L) - g + 1;

	for (i = h ; i > 0 ; i -= 1) {
		lua_getglobal(L, "luainteract");
		lua_getfield(L, -1, "pprint");
		lua_pushvalue(L, -i-2);
		lua_pushnumber(L, h-i+1);
		lua_call(L, 2, 0);
		lua_pop(L, 1);
	}

	lua_settop(L, g - 1);
	return status;
}

void luainteract_session_enter(struct luainteract_session *session)
{
	char *line, *full_line = NULL;
	bool multiline = false;
	lua_Debug lua_stack;

	mutex_lock(&active_session_mutex);

	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = "Haka";
	rl_attempted_completion_function = complete;
	rl_filename_completion_desired = 0;
	rl_completion_display_matches_hook = display_matches;

	current_session = session;
	LUA_STACK_MARK(session->L);

	/* Build a local environment that contains the parent context
	 * and the local variables */
	{
		int i;
		const char *local;

		lua_getstack(session->L, 1, &lua_stack);
		lua_getinfo(session->L, "f", &lua_stack);

		lua_newtable(session->L);

		lua_newtable(session->L);
		lua_getfenv(session->L, -3);
		lua_setfield(session->L, -2, "__index");
		lua_setmetatable(session->L, -2);

		for (i=1; (local = lua_getlocal(session->L, &lua_stack, i)); ++i) {
			lua_setfield(session->L, -2, local);
		}

		for (i=1; (local = lua_getupvalue(session->L, -2, i)); ++i) {
			lua_setfield(session->L, -2, local);
		}

		session->env_index = lua_gettop(session->L);
	}

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
			execute(session->L);
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

	lua_pop(session->L, 2);
	LUA_STACK_CHECK(session->L, 0);

	current_session = NULL;

	mutex_unlock(&active_session_mutex);
}

void luainteract_session_setprompts(struct luainteract_session *session, const char *single, const char *multi)
{
	free(session->prompt_single);
	free(session->prompt_multi);

	session->prompt_single = strdup(single);
	session->prompt_multi = strdup(multi);
}
