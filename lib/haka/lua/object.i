%module object

%{
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/lua/object.h>

#include <assert.h>
#include <stdio.h>
#include <wchar.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


#define OBJECT_TABLE "__haka_objects"


void lua_object_initialize(lua_State *L)
{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
}

void lua_object_init(struct lua_object *obj)
{
	obj->state = NULL;
}

void lua_object_release(void *ptr, struct lua_object *obj)
{
	assert(obj);

	if (obj->state) {
		lua_State *L = obj->state;
		obj->state = NULL;

		lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
		assert(lua_istable(L, -1));

		lua_pushlightuserdata(L, obj);
		lua_gettable(L, -2);
		lua_pushnil(L);

		assert(!lua_isnil(L, -2));
		assert(lua_istable(L, -2));

		while (lua_next(L, -2)) {
			swig_lua_userdata *usr;
			swig_lua_class *clss;

			assert(lua_isuserdata(L, -1));
			usr = (swig_lua_userdata*)lua_touserdata(L, -1);
			assert(usr);
			assert(usr->ptr);

			if (usr->ptr != ptr && usr->own) {
				clss = (swig_lua_class*)usr->type->clientdata;
				if (clss && clss->destructor)
				{
					clss->destructor(usr->ptr);
				}
			}

			usr->ptr = NULL;
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
		lua_pushlightuserdata(L, obj);
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
}

bool lua_object_get(lua_State *L, struct lua_object *obj, swig_type_info *type_info)
{
	assert(obj);

	if (obj->state && obj->state != L) {
		lua_pushstring(L, "invalid lua state (an object is being used by multiple lua state)");
		return false;
	}

	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		lua_pushnil(L);
		return true;
	}
	else {
		lua_pushstring(L, type_info->name);
		lua_gettable(L, -2);
		lua_remove(L, -2);
		lua_remove(L, -2);
		return true;
	}
}

void lua_object_register(lua_State *L, struct lua_object *obj, swig_type_info *type_info, int index)
{
	if (!obj->state) {
		obj->state = L;
	}
	else {
		/* A given lua object can only belong to 1 lua state */
		assert(obj->state == L);
	}

	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_TABLE);
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pushlightuserdata(L, obj);
		lua_newtable(L);
		lua_settable(L, -3);

		lua_pushlightuserdata(L, obj);
		lua_gettable(L, -2);
	}

	lua_pushstring(L, type_info->name);
	lua_pushvalue(L, index-3);
	lua_settable(L, -3);
	lua_pop(L, 2);
}

bool lua_object_push(lua_State *L, void *ptr, struct lua_object *obj, swig_type_info *type_info, int owner)
{
	if (ptr) {
		if (!lua_object_get(L, obj, type_info)) {
			return false;
		}

		if (!lua_isnil(L, -1)) {
			swig_lua_userdata *usr;
			assert(lua_isuserdata(L, -1));
			usr = (swig_lua_userdata *)lua_touserdata(L, -1);
			assert(SWIG_TypeCheckStruct(usr->type, type_info));

			if (owner) {
				usr->own = 1;
			}
		}
		else {
			if (ptr) {
				lua_pop(L, 1);
				SWIG_NewPointerObj(L, ptr, type_info, owner);

				lua_object_register(L, obj, type_info, -1);
			}
		}

		return true;
	}
	else {
		lua_pushnil(L);
		return true;
	}
}

%}
