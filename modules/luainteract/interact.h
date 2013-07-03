
#ifndef _LUAINTERACT_INTERACT_H
#define _LUAINTERACT_INTERACT_H

struct lua_State;
struct luainteract_session;

struct luainteract_session *luainteract_create(struct lua_State *L);
void luainteract_cleanup(struct luainteract_session *session);
void luainteract_enter(struct luainteract_session *session);
void luainteract_setprompts(struct luainteract_session *session, const char *single, const char *multi);

#endif /* _LUAINTERACT_INTERACT_H */
