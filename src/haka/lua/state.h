/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _STATE_H
#define _STATE_H

#include <wchar.h>
#include <haka/lua/state.h>

struct lua_State;

struct lua_state *haka_init_state();
int run_file(struct lua_State *L, const char *filename, int argc, char *argv[]);
int do_file_as_function(struct lua_State *L, const char *filename);

#endif /* _STATE_H */

