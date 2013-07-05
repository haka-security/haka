
#ifndef _LUADEBUG_INTERACT_H
#define _LUADEBUG_INTERACT_H

struct lua_State;
struct luadebug_session;

struct luadebug_session *luadebug_session_create(struct lua_State *L);
void luadebug_session_cleanup(struct luadebug_session *session);
void luadebug_session_enter(struct luadebug_session *session);
void luadebug_session_setprompts(struct luadebug_session *session, const char *single, const char *multi);

#endif /* _LUADEBUG_INTERACT_H */
