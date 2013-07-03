
#ifndef _HAKA_LUA_REF_H
#define _HAKA_LUA_REF_H

#include <haka/types.h>


struct lua_state;
struct lua_State;


/*
 * Lua reference management
 */

struct lua_ref {
	struct lua_state *state;
	int               ref;
};

void lua_ref_init(struct lua_ref *ref);
bool lua_ref_isvalid(struct lua_ref *ref);
void lua_ref_get(struct lua_State *state, struct lua_ref *ref);
bool lua_ref_clear(struct lua_ref *ref);
void lua_ref_push(struct lua_State *state, struct lua_ref *ref);

#endif /* _HAKA_LUA_REF_H */
