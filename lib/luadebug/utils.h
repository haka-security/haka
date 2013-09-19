
#ifndef _LUADEBUG_UTILS_H
#define _LUADEBUG_UTILS_H

#include <haka/colors.h>

struct lua_State;
struct luadebug_user;

void pprint(struct lua_State *L, struct luadebug_user *user, int index, bool full, const char *hide);
int execute_print(struct lua_State *L, struct luadebug_user *user);
int capture_env(struct lua_State *L, int frame);

#endif /* _LUADEBUG_UTILS_H */
