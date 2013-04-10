
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>

#include <haka/packet_module.h>
#include "app.h"
#include "lua/state.h"


static lua_state *global_lua_state;

/* Clean up lua state and loaded modules */
static void clean_exit()
{
	set_filter(NULL, NULL);

	if (global_lua_state) {
		cleanup_state(global_lua_state);
		global_lua_state = NULL;
	}

	set_packet_module(NULL);
	set_log_module(NULL);
}

static void fatal_error_signal(int sig)
{
	static volatile sig_atomic_t fatal_error_in_progress = 0;

	if (fatal_error_in_progress)
		raise(sig);
	fatal_error_in_progress = 1;

	/* Cleanup */
	message(HAKA_LOG_FATAL, L"core", L"fatal signal received");
	clean_exit();

	signal(sig, SIG_DFL);
	raise(sig);
}

int main(int argc, char *argv[])
{
	/* Set locale */
	setlocale(LC_ALL, "");

	/* Install signal handler */
	signal(SIGTERM, fatal_error_signal);
	signal(SIGINT, fatal_error_signal);
	signal(SIGQUIT, fatal_error_signal);
	signal(SIGHUP, fatal_error_signal);

	/* Check arguments */
	if (argc < 2) {
		fprintf(stderr, "usage: %s script_file [...]\n", argv[0]);
		clean_exit();
		return 2;
	}

	/* Init lua vm */
	global_lua_state = init_state();

	/* Execute configuration file */
	if (run_file(global_lua_state, argv[1], argc-2, argv+2)) {
		message(HAKA_LOG_FATAL, L"core", L"configuration error");
		clean_exit();
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
			clean_exit();
			return 1;
		}
	}

	/* Main loop */
	{
		struct packet *pkt = NULL;
		int error = 0;

		while ((error = packet_module->receive(&pkt)) == 0) {
			/* The packet can be NULL in case of failure in packet receive */
			if (pkt) {
				filter_result result = call_filter(global_lua_state, pkt);
				packet_module->verdict(pkt, result);
				pkt = NULL;
			}
		}
	}

	clean_exit();
	return 0;
}

