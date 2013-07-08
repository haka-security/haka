
#ifndef _LUADEBUG_DEBUGGER_H
#define _LUADEBUG_DEBUGGER_H

#include <haka/types.h>


struct lua_State;
struct luadebug_debugger;

struct luadebug_debugger *luadebug_debugger_create(struct lua_State *L);
struct luadebug_debugger *luadebug_debugger_get(struct lua_State *L);
void luadebug_debugger_cleanup(struct luadebug_debugger *session);
void luadebug_debugger_break(struct luadebug_debugger *session);
void luadebug_debugger_interrupt(struct luadebug_debugger *session);
bool luadebug_debugger_breakall();

#endif /* _LUADEBUG_DEBUGGER_H */
