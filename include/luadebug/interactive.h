/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _LUADEBUG_INTERACTIVE_H
#define _LUADEBUG_INTERACTIVE_H

struct lua_State;
struct luadebug_user;

void luadebug_interactive_user(struct luadebug_user *user);
void luadebug_interactive_enter(struct lua_State *L, const char *single, const char *multi,
		const char *msg);

#endif /* _LUADEBUG_INTERACTIVE_H */
