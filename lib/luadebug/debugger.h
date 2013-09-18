
#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <haka/types.h>
#include <luadebug/user.h>

#include "complete.h"

struct lua_State;
struct luadebug_debugger;

struct luadebug_debugger *luadebug_debugger_create(struct lua_State *L, bool break_immediatly);
void luadebug_debugger_cleanup(struct luadebug_debugger *session);

#endif /* _DEBUGGER_H */
