
#ifndef _LUAINTERACT_DEBUGGER_H
#define _LUAINTERACT_DEBUGGER_H

struct lua_State;
struct luainteract_debugger;

struct luainteract_debugger *luainteract_debugger_create(struct lua_State *L);
void luainteract_debugger_cleanup(struct luainteract_debugger *session);
void luainteract_debugger_break(struct luainteract_debugger *session);

#endif /* _LUAINTERACT_DEBUGGER_H */
