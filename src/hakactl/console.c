/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

#include <haka/colors.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/system.h>
#include <haka/lua/state.h>
#include <haka/lua/marshal.h>
#include <haka/luadebug/interactive.h>
#include <haka/luadebug/user.h>

#include "commands.h"
#include "ctl_comm.h"

#define HAKA_CONSOLE "/share/haka/console"


bool initialize_console(struct lua_state *state)
{
	const char *haka_path_s = haka_path();
	char *console_path;
	size_t size;
	DIR *dir;
	struct dirent entry, *result = NULL;

	/* Load all console utilities */
	lua_newtable(state->L);
	lua_setglobal(state->L, "console");
	lua_getglobal(state->L, "console");

	/* <haka_path_s><HAKA_CONSOLE/\0 */
	size = strlen(haka_path_s) + strlen(HAKA_CONSOLE) + 1;

	console_path = malloc(size);
	if (!console_path) {
		error("memory error");
		return false;
	}

	snprintf(console_path, size, "%s%s", haka_path_s, HAKA_CONSOLE);

	dir = opendir(console_path);
	if (!dir) {
		error("cannot open console script folder: %s", console_path);
		return false;
	}
	else {
		while (!readdir_r(dir, &entry, &result) && result) {
			const size_t len = strlen(entry.d_name);
			char fullfilename[PATH_MAX];
			const int level = lua_gettop(state->L);

			snprintf(fullfilename, sizeof(fullfilename), "%s/%s",
					console_path, entry.d_name);

			if (len > 4 && strcmp(entry.d_name + (len - 4), ".lua") == 0) {
				entry.d_name[len - 4] = 0;
			}
			else if (len > 3
			         && strcmp(entry.d_name + (len - 3), ".bc") == 0) {
				entry.d_name[len - 3] = 0;
			}
			else {
				continue;
			}

			LOG_DEBUG("hakactl", "loading console script '%s'", entry.d_name);

			if (luaL_dofile(state->L, fullfilename)) {
				const char *msg = lua_tostring(state->L, -1);
				LOG_ERROR("hakactl", "cannot open console script '%s': %s",
				         entry.d_name, msg);
				lua_pop(state->L, 1);
			}
			else {
				const int res = lua_gettop(state->L) - level;
				if (res == 1) {
					lua_setfield(state->L, -2, entry.d_name);
				}
				else {
					lua_pop(state->L, res);
				}
			}
		}
	}

	return true;
}

int console_fd = -1;
extern int luaopen_hakactl(lua_State *L);

static int run_console(int fd, int argc, char *argv[])
{
	struct lua_state *state;
	struct luadebug_user *user;

	state = lua_state_init();
	if (!state) {
		LOG_FATAL("hakactl", "%s", clear_error());
		return COMMAND_FAILED;
	}

	console_fd = fd;

	lua_pushcfunction(state->L, luaopen_hakactl);
	lua_call(state->L, 0, 1);
	lua_setglobal(state->L, "hakactl");

	if (!initialize_console(state)) {
		LOG_FATAL("hakactl", "%s", clear_error());
		lua_state_close(state);
		return COMMAND_FAILED;
	}

	user = luadebug_user_readline();
	if (!user) {
		LOG_FATAL("hakactl", "%s", clear_error());
		lua_state_close(state);
		return COMMAND_FAILED;
	}

	/* Load Lua console utilities */
	luadebug_interactive_enter(state->L, ">  ", ">> ", NULL, -1, user);
	luadebug_user_release(&user);

	lua_state_close(state);

	return COMMAND_SUCCESS;
}

struct command command_console = {
	"console",
	"console:            Connect to haka and execute commands",
	0,
	run_console
};
