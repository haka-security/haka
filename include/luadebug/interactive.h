
#ifndef _LUADEBUG_INTERACTIVE_H
#define _LUADEBUG_INTERACTIVE_H

struct lua_State;
struct luadebug_user;

void luadebug_interactive_user(struct luadebug_user *user);
void luadebug_interactive_enter(struct lua_State *L, const char *single, const char *multi);

#endif /* _LUADEBUG_INTERACTIVE_H */
