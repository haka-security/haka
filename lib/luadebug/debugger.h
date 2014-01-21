/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <haka/types.h>
#include <luadebug/user.h>

#include "complete.h"

struct lua_State;
struct luadebug_debugger;

struct luadebug_debugger *luadebug_debugger_create(struct lua_State *L, bool break_immediatly);
void luadebug_debugger_cleanup(struct luadebug_debugger *session);

void lua_pushpdebugger(struct lua_State *L, struct luadebug_debugger *dbg);
struct luadebug_debugger *lua_getpdebugger(struct lua_State *L, int index);

#endif /* _DEBUGGER_H */
