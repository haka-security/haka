/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LUADEBUG_UTILS_H
#define LUADEBUG_UTILS_H

#include <haka/colors.h>

struct lua_State;
struct luadebug_user;

void pprint(struct lua_State *L, struct luadebug_user *user, int index, bool full, const char *hide);
int execute_print(struct lua_State *L, struct luadebug_user *user, bool full, const char *hide);
int capture_env(struct lua_State *L, int frame);

#endif /* LUADEBUG_UTILS_H */
