
#ifndef _HAKA_LUA_LUA_H
#define _HAKA_LUA_LUA_H

#include <assert.h>


#ifndef NDEBUG

#define LUA_STACK_MARK(L) \
	const int __stackmark = lua_gettop((L))

#define LUA_STACK_CHECK(L, offset) \
	assert(lua_gettop((L)) == __stackmark+(offset))

#else

#define LUA_STACK_MARK(L)
#define LUA_STACK_CHECK(L, offset)

#endif

#endif /* _HAKA_LUA_LUA_H */
