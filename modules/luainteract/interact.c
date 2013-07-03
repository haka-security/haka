
#define _GNU_SOURCE

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/lua/state.h>

#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <assert.h>

#include "interact.h"


struct luainteract_session
{
	lua_State       *L;
	char            *prompt_single;
	char            *prompt_multi;
	lua_Debug        debug;
	int              env_index;

	/* Completion support */
	int              stack_top;
	int              compl_state;
	const char     **compl_keyword;
	const char      *compl_token;
	int              compl_operator;
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


static local_storage_t current_session_key;
static mutex_t active_session_mutex = MUTEX_INIT;

INIT static void _luainteract_init()
{
	UNUSED const bool ret = local_storage_init(&current_session_key, NULL);
	assert(ret);
}

FINI static void _luainteract_fini()
{
	UNUSED const bool ret = local_storage_destroy(&current_session_key);
	assert(ret);
}

static void luainteract_setcurrent(struct luainteract_session *session)
{
	local_storage_set(&current_session_key, session);
}

static struct luainteract_session *luainteract_getcurrent()
{
	struct luainteract_session *ret = (struct luainteract_session *)local_storage_get(&current_session_key);
	assert(ret);
	return ret;
}

static void display_matches(char **matches, int num_matches, int max_length)
{
	printf(BOLD);
	rl_display_match_list(matches, num_matches, max_length);
	printf(CLEAR);
	rl_on_new_line();
}

static char *lua_keyword_completions(const char *text, int state)
{
	static const char *keywords[] = {
		"and", "break", "do", "else", "elseif", "end", "false", "for",
		"function", "if", "in", "local", "nil", "not", "or",
		"repeat", "return", "then", "true", "until", "while", NULL
	};

	int text_len;
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		session->compl_keyword = keywords - 1;
	}

	text_len = strlen(text);

	for (++session->compl_keyword; *session->compl_keyword; ++session->compl_keyword) {
		const int len = strlen(*session->compl_keyword);
		if (len >= text_len && !strncmp(*session->compl_keyword, text, text_len)) {
			return strdup(*session->compl_keyword);
		}
	}

	return NULL;
}

static char *table_key_search(struct luainteract_session *session, const char *text, bool skip_internal)
{
	while (true) {
		lua_pushvalue(session->L, -3);
		lua_insert(session->L, -3);
		lua_pushvalue(session->L, -2);
		lua_insert(session->L, -4);

		if (lua_pcall(session->L, 2, 2, 0)) {
			continue;
		}
		else {
			size_t l, m;
			int nospace, type;

			if (lua_isnil(session->L, -2)) {
				return NULL;
			}

			type = lua_type(session->L, -1);
			nospace = (type == LUA_TTABLE || type == LUA_TUSERDATA ||
					type == LUA_TFUNCTION);
			lua_pop(session->L, 1);

			type = lua_type(session->L, -1);
			if (type == LUA_TSTRING || (session->compl_operator == '[' && type == LUA_TNUMBER)) {
				char *candidate = NULL;

				if (type == LUA_TNUMBER) {
					lua_Number n;
					int i;

					n = lua_tonumber(session->L, -1);
					i = (int)n;
					if (n == (lua_Number)i) {
						l = asprintf(&candidate, "%d]", i);
					}
					else {
						continue;
					}
				}
				else {
					candidate = strdup(lua_tolstring(session->L, -1, &l));

					if (skip_internal && candidate[0] == '_') {
						continue;
					}

					if (session->compl_operator == '[') {
						char q = session->compl_token[0];
						if (q != '"' && q != '\'') {
							q = '"';
						}

						l = asprintf(&candidate, "%c%s%c]", q, candidate, q);
					}
				}

				m = strlen(session->compl_token);

				if (l >= m && !strncmp(session->compl_token, candidate, m)) {
					char *match;

					if (session->compl_token > text) {
						match = malloc((session->compl_token - text) + l + 1);
						strncpy(match, text, session->compl_token - text);
						strcpy(match + (session->compl_token - text), candidate);
					} else {
						match = strdup(candidate);
					}

					rl_completion_suppress_append = nospace;

					free(candidate);
					return match;
				}

				free(candidate);
			}
		}
	}

	return NULL;
}

static bool extract_table_context(struct luainteract_session *session, const char *text)
{
	const char *c;

	for (c = text + strlen(text) - 1;
		 c >= text && *c != '.' && *c != ':' && *c != '[';
		 --c);

	if (c > text) {
		char *buffer;
		int type;
		size_t table_size;

		session->compl_token = c + 1;
		session->compl_operator = *c;

		table_size = session->compl_token - text - 1;

		buffer = malloc(7 + table_size + 1);
		if (!buffer) {
			error(L"memory error");
			return false;
		}

		strcpy(buffer, "return ");
		strncpy(buffer+7, text, table_size);
		buffer[7+table_size] = 0;

		if (luaL_loadstring(session->L, buffer)) {
			free(buffer);
			return false;
		}

		free(buffer);
		buffer = NULL;

		/* Change the chunk environment */
		lua_pushvalue(session->L, session->env_index);
		lua_setfenv(session->L, -2);

		if (lua_pcall(session->L, 0, 1, 0)) {
			return false;
		}

		type = lua_type(session->L, -1);
		if (type != LUA_TUSERDATA && type != LUA_TTABLE) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

static bool load_pairs(struct luainteract_session *session)
{
	lua_getglobal(session->L, "pairs");
	lua_insert(session->L, -2);
	if(lua_type(session->L, -2) != LUA_TFUNCTION ||
	   lua_pcall(session->L, 1, 3, 0)) {
		return false;
	}

	return true;
}

static char *table_completions(const char *text, int state)
{
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		if (!extract_table_context(session, text)) {
			return NULL;
		}

		if (!load_pairs(session)) {
			return NULL;
		}
	}

	return table_key_search(session, text, true);
}

static char *swig_get_completions(const char *text, int state)
{
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		if (!extract_table_context(session, text)) {
			return NULL;
		}

		lua_getmetatable(session->L, -1);
		lua_getfield(session->L, -1, ".get");

		if (!load_pairs(session)) {
			return NULL;
		}
	}

	return table_key_search(session, text, true);
}


static char *swig_fn_completions(const char *text, int state)
{
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		if (!extract_table_context(session, text)) {
			return NULL;
		}

		lua_getmetatable(session->L, -1);
		lua_getfield(session->L, -1, ".fn");

		if (!load_pairs(session)) {
			return NULL;
		}
	}

	return table_key_search(session, text, true);
}

static char *global_completions(const char *text, int state)
{
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		session->compl_operator = 0;
		session->compl_token = text;

		lua_pushvalue(session->L, LUA_GLOBALSINDEX);

		if (!load_pairs(session)) {
			return NULL;
		}
	}

	return table_key_search(session, text, true);
}

static char *local_completions(const char *text, int state)
{
	struct luainteract_session *session = luainteract_getcurrent();

	if (state == 0) {
		session->compl_operator = 0;
		session->compl_token = text;

		lua_pushvalue(session->L, session->env_index);

		if (!load_pairs(session)) {
			return NULL;
		}
	}

	return table_key_search(session, text, true);
}

typedef char *(*compl_callback)(const char *text, int state);

static const compl_callback completions[] = {
	&lua_keyword_completions,
	&global_completions,
	&local_completions,
	&table_completions,
	&swig_get_completions,
	&swig_fn_completions,
	NULL
};

static char *generator(const char *text, int state)
{
	char *match = NULL;
	struct luainteract_session *session = luainteract_getcurrent();
	const compl_callback *iter = NULL;
	int i;

	if (state == 0) {
		session->compl_state = 0;
	}

	for (iter = completions, i=0; *iter; ++iter, ++i) {
		if (session->compl_state == i) {
			match = (*iter)(text, state);
			if (!match) {
				++session->compl_state;
				state = 0;
				lua_settop(session->L, session->stack_top);
			}
			else {
				return match;
			}
		}
	}

	return NULL;
}

static char **complete(const char *text, int start, int end)
{
	char **matches;
	struct luainteract_session *session = luainteract_getcurrent();

	session->stack_top = lua_gettop(session->L);
	rl_completion_suppress_append = 0;
	matches = rl_completion_matches(text, generator);
	lua_settop(session->L, session->stack_top);

	return matches;
}

INIT static void luainteract_init()
{
	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = "Haka";
	rl_attempted_completion_function = complete;
	rl_completion_display_matches_hook = display_matches;
}

struct luainteract_session *luainteract_create(struct lua_State *L)
{
	struct luainteract_session *ret = malloc(sizeof(struct luainteract_session));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->L = L;
	ret->prompt_single = NULL;
	ret->prompt_multi = NULL;

	luainteract_setprompts(ret, ">  ", ">> ");

	return ret;
}

void luainteract_cleanup(struct luainteract_session *session)
{
	free(session->prompt_single);
	free(session->prompt_multi);
	free(session);
}

static int luap_call(lua_State *L, int n) {
	int status;
	int h;

	/* Add the traceback formatter */
	h = lua_gettop(L) - n;
	lua_pushcfunction(L, lua_state_error_formater);
	lua_insert(L, h);

	/* Try to execute the supplied chunk and keep note of any return
	 * values. */
	status = lua_pcall(L, n, LUA_MULTRET, h);

	/* Print all returned values with proper formatting. */
	if (status) {
		printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	lua_remove(L, h);

	return status;
}

static int execute(lua_State *L)
{
	int i, h_0, h, status;

	h_0 = lua_gettop(L);
	status = luap_call(L, 0);
	h = lua_gettop(L) - h_0 + 1;

	for (i = h ; i > 0 ; i -= 1) {
		lua_getglobal(L, "luainteract");
		lua_getfield(L, -1, "pprint");
		lua_pushvalue(L, -i-2);
		lua_pushnumber(L, h-i+1);
		lua_call(L, 2, 0);
		lua_pop(L, 1);
	}

	lua_settop(L, h_0 - 1);

	return status;
}

void luainteract_enter(struct luainteract_session *session)
{
	char *line, *full_line = NULL;
	bool multiline = false;
	lua_Debug lua_stack;

	mutex_lock(&active_session_mutex);

	luainteract_setcurrent(session);

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
			if (full_line) {
				full_line = realloc(full_line, strlen(full_line) + strlen(line) + 1);
				strcpy(full_line + strlen(full_line), line);
			}
			else {
				full_line = strdup(line);
			}

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

		free(line);

		if (!multiline) {
			free(full_line);
			full_line = NULL;
		}

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
	}

	printf("\n");
	rl_on_new_line();

	luainteract_setcurrent(NULL);

	mutex_unlock(&active_session_mutex);
}

void luainteract_setprompts(struct luainteract_session *session, const char *single, const char *multi)
{
	free(session->prompt_single);
	free(session->prompt_multi);

	asprintf(&session->prompt_single, "%s%s%s%s", GREEN, BOLD, single, CLEAR);
	asprintf(&session->prompt_multi, "%s%s%s%s", GREEN, BOLD, multi, CLEAR);
}
