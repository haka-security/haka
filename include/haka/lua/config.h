/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_CONFIG_H
#define _HAKA_LUA_CONFIG_H

#include <haka/types.h>

struct haka_lua_config {
	bool  lua_debugger;
	struct {
		bool  debug;
		bool  graph;
	} ccomp;
};

extern struct haka_lua_config haka_lua_config;

#endif /* _HAKA_LUA_CONFIG_H */
