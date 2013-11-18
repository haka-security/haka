
#ifndef _LUADEBUG_DEBUGGER_H
#define _LUADEBUG_DEBUGGER_H

#include <haka/types.h>

struct lua_State;
struct luadebug_user;

void luadebug_debugger_user(struct luadebug_user *user);
bool luadebug_debugger_start(struct lua_State *L, bool break_immediatly);
void luadebug_debugger_stop(struct lua_State *L);
bool luadebug_debugger_break(struct lua_State *L);
bool luadebug_debugger_interrupt(struct lua_State *L, const char *reason);
bool luadebug_debugger_breakall();
bool luadebug_debugger_shutdown();

#endif /* _LUADEBUG_DEBUGGER_H */
