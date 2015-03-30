/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LUADEBUG_INTERACTIVE_H
#define LUADEBUG_INTERACTIVE_H

struct lua_State;
struct luadebug_user;

void luadebug_interactive_user(struct luadebug_user *user);
void luadebug_interactive_enter(struct lua_State *L, const char *single, const char *multi,
		const char *msg, int env, struct luadebug_user *user);

#endif /* LUADEBUG_INTERACTIVE_H */
