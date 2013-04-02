
#include <stdlib.h>
#include <stdio.h>

#include <lua.h>

#include <haka/packet_module.h>
#include "app.h"
#include "lua/state.h"


int main(int argc, char *argv[])
{
	/* Init lua vm */
	lua_State *lua_state = init_state();

	/* Execute configuration file */
	if (luaL_loadfile(lua_state, argv[1])) {
		print_error("configuration failed", lua_state);
		return 1;
	}

	if (lua_pcall(lua_state, 0, 0, 0)) {
		print_error("configuration failed", lua_state);
		return 1;
	}

	/* Check module status */
	struct packet_module *packet_module = get_packet_module();
	struct log_module *log_module = get_log_module();

	{
		int err = 0;

		if (!packet_module) {
			fprintf(stderr, "error: no packet module found\n");
			err = 1;
		}

		if (!has_filter()) {
			fprintf(stderr, "error: no filter function set\n");
			err = 1;
		}

		if (!log_module) {
			fprintf(stderr, "warning: no log module found\n");
		}

		if (err) {
			return 1;
		}
	}

	/* Main loop */
	{
		struct packet *pkt = NULL;
		int error = 0;

		while ((error = packet_module->receive(&pkt)) == 0) {
			filter_result result = call_filter(lua_state, pkt);
			packet_module->verdict(pkt, result);
		}
	}

	return 0;
}

