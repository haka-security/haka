/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lauxlib.h>

#include <haka/config.h>
#include <haka/lua/luautils.h>
#include <haka/luabinding.h>
#include <haka/packet.h>
#include <haka/types.h>

#include "packet.h"
#include "thread.h"

#include "lua/packet.h"

#ifdef HAKA_FFI

void packet_receive_wrapper_wrap(void *_state, struct receive_result *res)
{
	struct thread_state *state;
	state = (struct thread_state *)_state;

	if (state == NULL) return;

	packet_receive_wrapper(state, &res->pkt, &res->has_interrupts, &res->stop);
}

LUA_BIND_INIT(packet)
{
	lua_load_packet(L);
	lua_call(L, 0, 1);

	return 1;
}

#else

extern bool lua_pushppacket(lua_State *L, struct packet *pkt);

static int packet_receive_wrapper_wrap(lua_State *L)
{
	struct thread_state *state;
	struct packet *pkt = NULL;
	bool has_interrupts = false;
	bool stop = false;

	assert(lua_islightuserdata(L, -1));
	state = lua_touserdata(L, -1);
	lua_pop(L, 1);

	packet_receive_wrapper(state, &pkt, &has_interrupts, &stop);

	lua_pushppacket(L, pkt);
	lua_pushboolean(L, has_interrupts);
	lua_pushboolean(L, stop);

	return 3;
}

static int packet_drop_wrap(lua_State *L)
{
	struct packet *pkt;

	assert(lua_isuserdata(L, -1));
	pkt = lua_touserdata(L, -1);
	lua_pop(L, 1);

	packet_drop(pkt);

	return 0;
}

LUA_BIND_INIT(packet)
{
	LUA_LOAD(packet, L);

	lua_pushcfunction(L, packet_receive_wrapper_wrap);
	lua_pushcfunction(L, packet_drop_wrap);

	lua_call(L, 2, 1);

	return 1;
}

#endif
