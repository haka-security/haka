
#ifndef _LUADEBUG_INTERACTIVE_H
#define _LUADEBUG_INTERACTIVE_H

struct lua_State;
struct luadebug_interactive;

struct luadebug_interactive *luadebug_interactive_create(struct lua_State *L);
void luadebug_interactive_cleanup(struct luadebug_interactive *interactive);
void luadebug_interactive_enter(struct luadebug_interactive *interactive);
void luadebug_interactive_setprompts(struct luadebug_interactive *interactive, const char *single, const char *multi);

#endif /* _LUADEBUG_INTERACTIVE_H */
