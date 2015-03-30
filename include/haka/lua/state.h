/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_LUA_STATE_H
#define HAKA_LUA_STATE_H

#include <haka/types.h>
#include <wchar.h>

struct lua_State;
struct lua_Debug;

struct lua_state {
	struct lua_State    *L;
};

typedef int  (*lua_function)(struct lua_State *L);
typedef void (*lua_hook)(struct lua_State *L, struct lua_Debug *ar);

struct lua_state *lua_state_init();
void lua_state_close(struct lua_state *state);
bool lua_state_require(struct lua_state *state, const char *module);
bool lua_state_isvalid(struct lua_state *state);
bool lua_state_interrupt(struct lua_state *state, lua_function func, void *data, void (*destroy)(void *));
bool lua_state_runinterrupt(struct lua_state *state);
bool lua_state_setdebugger_hook(struct lua_state *state, lua_hook hook);
bool lua_state_run_file(struct lua_state *L, const char *filename, int argc, char *argv[]);
void lua_state_trigger_haka_event(struct lua_state *state, const char *event);

int lua_state_error_formater(struct lua_State *L);
void lua_state_print_error(struct lua_State *L, const char *msg);
struct lua_state *lua_state_get(struct lua_State *L);

extern void (*lua_state_error_hook)(struct lua_State *L);

#if HAKA_LUA52
void lua_getfenv(struct lua_State *L, int index);
int  lua_setfenv(struct lua_State *L, int index);
#endif

#endif /* HAKA_LUA_STATE_H */
