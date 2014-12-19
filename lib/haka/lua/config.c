/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/lua/config.h>

struct haka_lua_config haka_lua_config = {
	.lua_debugger    = false,
	.ccomp = {
		.debug   = false,
		.graph   = false,
	}
};
