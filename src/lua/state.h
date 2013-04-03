
#ifndef _STATE_H
#define _STATE_H

#include <wchar.h>


struct lua_State *init_state();
void cleanup_state(struct lua_State *L);
void print_error(const wchar_t *msg, struct lua_State *L);

#endif /* _STATE_H */

