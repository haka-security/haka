
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <haka/packet_module.h>
#include "app.h"
#include "lua/state.h"


int main(int argc, char *argv[])
{
	/* Set locale */
	setlocale(LC_ALL, "");

	/* Check arguments */
	if (argc < 2) {
		fprintf(stderr, "usage: %s script_file [...]\n", argv[0]);
		return 2;
	}

	/* Init lua vm */
	lua_state *lua_state = init_state();

	/* Execute configuration file */
	if (run_file(lua_state, argv[1], argc-2, argv+2)) {
		message(HAKA_LOG_FATAL, L"core", L"configuration error");
		return 1;
	}

	/* Check module status */
	struct packet_module *packet_module = get_packet_module();

	{
		int err = 0;

		if (!packet_module) {
			message(HAKA_LOG_FATAL, L"core", L"no packet module found");
			err = 1;
		}

		if (!has_filter()) {
			message(HAKA_LOG_FATAL, L"core", L"no filter function set");
			err = 1;
		}

		if (!has_log_module()) {
			message(HAKA_LOG_WARNING, L"core", L"no log module found");
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

