/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lauxlib.h>

#include <haka/luabinding.h>

#include "lua/raw.h"

LUA_BIND_INIT(raw)
{
	LUA_LOAD(raw, L);

	return 1;
}
