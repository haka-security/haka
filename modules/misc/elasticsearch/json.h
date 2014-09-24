/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <jansson.h>

#include <haka/types.h>

struct lua_State;

json_t *lua2json(struct lua_State *L, int index);
