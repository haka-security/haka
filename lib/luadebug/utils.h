
#ifndef _LUADEBUG_UTILS_H
#define _LUADEBUG_UTILS_H

#include <haka/colors.h>

struct lua_State;

int execute_print(struct lua_State *L);
int capture_env(struct lua_State *L, int frame);

#endif /* _LUADEBUG_UTILS_H */
