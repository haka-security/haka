
#include <haka/types.h>
#include <haka/lua/ref.h>

#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


void lua_ref_init(struct lua_ref *ref)
{
	ref->state = NULL;
	ref->ref = LUA_NOREF;
}

bool lua_ref_isvalid(struct lua_ref *ref)
{
	return (ref->state && ref->ref != LUA_NOREF);
}

void lua_ref_get(struct lua_State *state, struct lua_ref *ref)
{
	lua_ref_clear(ref);

	ref->state = state;
	ref->ref = luaL_ref(ref->state, LUA_REGISTRYINDEX);
}

bool lua_ref_clear(struct lua_ref *ref)
{
	if (lua_ref_isvalid(ref)) {
		luaL_unref(ref->state, LUA_REGISTRYINDEX, ref->ref);
		ref->ref = LUA_NOREF;
		ref->state = NULL;
		return true;
	}
	else {
		return false;
	}
}

void lua_ref_push(struct lua_State *state, struct lua_ref *ref)
{
	assert(lua_ref_isvalid(ref));
	assert(state == ref->state);
	lua_rawgeti(ref->state, LUA_REGISTRYINDEX, ref->ref);
}
