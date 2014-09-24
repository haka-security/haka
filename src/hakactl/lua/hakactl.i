/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module hakactl

%{
#include <haka/config.h>
#include <haka/colors.h>
#include <haka/engine.h>
#include <haka/lua/marshal.h>
#include "ctl_comm.h"
%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%nodefaultctor;
%nodefaultdtor;

%native(_remote_launch) int _remote_launch(lua_State *L);

%{

	extern int console_fd;

	static int _remote_launch(lua_State *L)
	{
		const int n = lua_gettop(L);
		int thread_id;
		int index;
		char *code;
		int answer;
		size_t codesize;

		if (n != 2) {
			lua_pushstring(L, "incorrect argument");
			lua_error(L);
			return 0;
		}

		thread_id = lua_tonumber(L, 1);

		code = lua_marshal(L, 2, &codesize);
		if (!code) {
			lua_pushstring(L, clear_error());
			lua_error(L);
			return 0;
		}

		/* Send the remote command */
		if (!ctl_send_chars(console_fd, "EXECUTE", -1)) {
			lua_pushstring(L, clear_error());
			lua_error(L);
			return 0;
		}

		if (!ctl_send_int(console_fd, thread_id) ||
		    !ctl_send_chars(console_fd, code, codesize)) {
			lua_pushstring(L, clear_error());
			lua_error(L);
			return 0;
		}

		free(code);
		code = NULL;

		/* Wait for the result */
		lua_newtable(L);

		index = 1;

		while (true) {
			answer = ctl_recv_status(console_fd);
			if (answer == -1) {
				lua_pushstring(L, clear_error());
				lua_error(L);
				return 0;
			}

			if (answer == 1) {
				lua_pushnumber(L, index++);

				code = ctl_recv_chars(console_fd, &codesize);
				if (!code) {
					lua_pushstring(L, clear_error());
					lua_error(L);
					return 0;
				}

				if (!lua_unmarshal(L, code, codesize)) {
					free(code);
					lua_pushstring(L, clear_error());
					lua_error(L);
					return 0;
				}

				lua_settable(L, -3);

				free(code);
			}
			else if (answer == 0) {
				break;
			}
			else {
				lua_pushstring(L, "invalid server answer");
				lua_error(L);
				return 0;
			}
		}

		return 1;
	}

%}

%luacode{

	local this = unpack({...})

	function this.remote(thread, func)
		if thread == 'all' then thread = -1
		elseif thread == 'any' then thread = -2 end

		return this._remote_launch(thread, func)
	end

	haka.mode = 'console'
}
