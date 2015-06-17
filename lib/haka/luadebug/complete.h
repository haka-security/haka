/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LUADEBUG_COMPLETE_H
#define LUADEBUG_COMPLETE_H

#include <haka/types.h>


struct lua_State;

struct luadebug_complete {
	int              stack_top;
	int              stack_env;
	int              state;
	const char     **keyword;
	int              index;
	const char      *token;
	int              operator;
};

bool complete_push_table_context(struct lua_State *L, struct luadebug_complete *context,
		const char *text);

char *complete_table(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state, bool (*hidden)(const char *text));

char *complete_keyword(struct luadebug_complete *context, const char *keywords[],
		const char *text, int state);

bool complete_underscore_hidden(const char *text);

typedef char *(*complete_callback)(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);

char *complete_generator(struct lua_State *L, struct luadebug_complete *context,
		const complete_callback callbacks[], const char *text, int state);

char *complete_callback_lua_keyword(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);
char *complete_callback_table(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);
char *complete_callback_swig_get(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);
char *complete_callback_swig_fn(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);
char *complete_callback_global(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);
char *complete_callback_fenv(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state);

char *complete_addchar(const char *str, char c);

#endif /* LUADEBUG_COMPLETE_H */
