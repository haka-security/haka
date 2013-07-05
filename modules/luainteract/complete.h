
#ifndef _LUAINTERACT_COMPLETE_H
#define _LUAINTERACT_COMPLETE_H

#include <haka/types.h>


struct lua_State;

struct luainteract_complete {
	int              stack_top;
	int              stack_env;
	int              state;
	const char     **keyword;
	int              index;
	const char      *token;
	int              operator;
	bool             space;
};

bool complete_push_table_context(struct lua_State *L, struct luainteract_complete *context,
		const char *text);

char *complete_table(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state, bool (*hidden)(const char *text));

char *complete_keyword(struct luainteract_complete *context, const char *keywords[],
		const char *text, int state);

bool complete_underscore_hidden(const char *text);

typedef char *(*complete_callback)(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);

char *complete_generator(struct lua_State *L, struct luainteract_complete *context,
		const complete_callback callbacks[], const char *text, int state);

char *complete_callback_lua_keyword(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);
char *complete_callback_table(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);
char *complete_callback_swig_get(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);
char *complete_callback_swig_fn(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);
char *complete_callback_global(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);
char *complete_callback_fenv(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state);

#endif /* _LUAINTERACT_COMPLETE_H */
