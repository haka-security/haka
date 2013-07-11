
#include "complete.h"

#define _GNU_SOURCE

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


char *complete_addchar(const char *str, char c)
{
	const size_t len = strlen(str);
	char *ret = malloc(len+2);
	if (!ret) {
		return NULL;
	}

	strcpy(ret, str);
	ret[len] = c;
	ret[len+1] = 0;
	return ret;
}

char *complete_keyword(struct luadebug_complete *context, const char *keywords[],
		const char *text, int state)
{
	int text_len;
	if (state == 0) {
		context->keyword = keywords - 1;
	}

	text_len = strlen(text);

	for (++context->keyword; *context->keyword; ++context->keyword) {
		const int len = strlen(*context->keyword);
		if (len >= text_len && !strncmp(*context->keyword, text, text_len)) {
			return complete_addchar(*context->keyword, ' ');
		}
	}

	return NULL;
}

char *complete_callback_lua_keyword(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	static const char *keywords[] = {
		"and", "break", "do", "else", "elseif", "end", "false", "for",
		"function", "if", "in", "local", "nil", "not", "or",
		"repeat", "return", "then", "true", "until", "while", NULL
	};

	return complete_keyword(context, keywords, text, state);
}

bool complete_push_table_context(struct lua_State *L, struct luadebug_complete *context,
		const char *text)
{
	const char *c;

	for (c = text + strlen(text) - 1;
		 c >= text && *c != '.' && *c != ':' && *c != '[';
		 --c);

	if (c > text) {
		char *buffer;
		int type;
		size_t table_size;

		context->token = c + 1;
		context->operator = *c;

		table_size = context->token - text - 1;

		buffer = malloc(7 + table_size + 1);
		if (!buffer) {
			error(L"memory error");
			return false;
		}

		strcpy(buffer, "return ");
		strncat(buffer+7, text, table_size);

		if (luaL_loadstring(L, buffer)) {
			free(buffer);
			return false;
		}

		free(buffer);
		buffer = NULL;

		if (context->stack_env >= 0) {
			/* Change the chunk environment */
			lua_pushvalue(L, context->stack_env);
			lua_setfenv(L, -2);
		}

		if (lua_pcall(L, 0, 1, 0)) {
			return false;
		}

		type = lua_type(L, -1);
		if (type != LUA_TUSERDATA && type != LUA_TTABLE) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

char *complete_table(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state, bool (*hidden)(const char *text))
{
	if (state == 0) {
		lua_getglobal(L, "pairs");
		lua_insert(L, -2);
		if(lua_type(L, -2) != LUA_TFUNCTION ||
		   lua_pcall(L, 1, 3, 0)) {
			return NULL;
		}
	}

	while (true) {
		lua_pushvalue(L, -3);
		lua_insert(L, -3);
		lua_pushvalue(L, -2);
		lua_insert(L, -4);

		if (lua_pcall(L, 2, 2, 0)) {
			continue;
		}
		else {
			size_t l, m;
			int nospace, type;

			if (lua_isnil(L, -2)) {
				return NULL;
			}

			type = lua_type(L, -1);
			nospace = (type == LUA_TTABLE || type == LUA_TUSERDATA || type == LUA_TFUNCTION);
			lua_pop(L, 1);

			type = lua_type(L, -1);
			if (type == LUA_TSTRING || (context->operator == '[' && type == LUA_TNUMBER)) {
				char *candidate = NULL;

				if (type == LUA_TNUMBER) {
					lua_Number n;
					int i;

					n = lua_tonumber(L, -1);
					i = (int)n;
					if (n == (lua_Number)i) {
						l = asprintf(&candidate, "%d]", i);
					}
					else {
						continue;
					}
				}
				else {
					candidate = strdup(lua_tolstring(L, -1, &l));

					if (hidden && hidden(candidate) && !hidden(context->token)) {
						continue;
					}

					if (context->operator == '[') {
						char q = context->token[0];
						if (q != '"' && q != '\'') {
							q = '"';
						}

						l = asprintf(&candidate, "%c%s%c]", q, candidate, q);
					}
				}

				m = strlen(context->token);

				if (l >= m && !strncmp(context->token, candidate, m)) {
					char *match;

					if (context->token > text) {
						match = malloc((context->token - text) + l + 1);
						strncpy(match, text, context->token - text);
						strcpy(match + (context->token - text), candidate);
					} else {
						match = strdup(candidate);
					}

					free(candidate);

					if (!nospace) {
						char *ret = complete_addchar(match, ' ');
						free(match);
						return ret;
					}
					else {
						return match;
					}
				}

				free(candidate);
			}
		}
	}

	return NULL;
}

bool complete_underscore_hidden(const char *text)
{
	return text[0] == '_';
}

char *complete_generator(struct lua_State *L, struct luadebug_complete *context,
		const complete_callback callbacks[], const char *text, int state)
{
	char *match = NULL;
	const complete_callback *iter = NULL;
	int i;

	if (state == 0) {
		context->state = 0;
		context->stack_top = lua_gettop(L);
	}

	for (iter = callbacks, i=0; *iter; ++iter, ++i) {
		if (context->state == i) {
			match = (*iter)(L, context, text, state);
			if (!match) {
				++context->state;
				state = 0;
				lua_settop(L, context->stack_top);
			}
			else {
				return match;
			}
		}
	}

	lua_settop(L, context->stack_top);
	return NULL;
}

char *complete_callback_table(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	if (state == 0) {
		if (!complete_push_table_context(L, context, text)) {
			return NULL;
		}
	}

	return complete_table(L, context, text, state, &complete_underscore_hidden);
}

static char *complete_callback_swig(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state, const char *table)
{
	if (state == 0) {
		if (!complete_push_table_context(L, context, text)) {
			return NULL;
		}

		lua_getmetatable(L, -1);
		lua_getfield(L, -1, table);
	}

	return complete_table(L, context, text, state, &complete_underscore_hidden);
}

char *complete_callback_swig_get(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	return complete_callback_swig(L, context, text, state, ".get");
}

char *complete_callback_swig_fn(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	return complete_callback_swig(L, context, text, state, ".fn");
}

char *complete_callback_global(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	if (state == 0) {
		context->operator = 0;
		context->token = text;

		lua_pushvalue(L, LUA_GLOBALSINDEX);
	}

	return complete_table(L, context, text, state, &complete_underscore_hidden);
}

char *complete_callback_fenv(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	if (state == 0) {
		context->operator = 0;
		context->token = text;

		lua_pushvalue(L, context->stack_env);
	}

	return complete_table(L, context, text, state, &complete_underscore_hidden);
}
