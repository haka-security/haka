/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lauxlib.h>

#include <haka/config.h>
#include <haka/luabinding.h>
#include <haka/module.h>
#include <haka/packet.h>
#include <haka/lua/state.h>
#include <haka/log.h>

#include "lua/raw.h"

#ifdef HAKA_FFI

LUA_BIND_INIT(raw)
{
	LUA_LOAD(raw, L);
	lua_call(L, 0, 1);

	return 1;
}

bool packet_new_ffi(struct ffi_object *packet, size_t size)
{
	struct packet *pkt = packet_new(size);
	if (!pkt) {
		return false;
	}
	else {
		packet->ref = &pkt->lua_object.ref;
		packet->ptr = pkt;
		return true;
	}
}

#endif

struct lua_ref *packet_get_luadata(struct packet *pkt)
{
	return &pkt->luadata;
}

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        "Raw",
	description: "Raw packet protocol",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
