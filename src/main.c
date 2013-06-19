
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/error.h>

#include "app.h"
#include "thread.h"
#include "lua/state.h"


static lua_state *global_lua_state;
static struct thread_pool *thread_states;


/* Clean up lua state and loaded modules */
static void clean_exit()
{
	set_configuration_script(NULL);

	if (thread_states) {
		thread_pool_cancel(thread_states);

		thread_pool_cleanup(thread_states);
		thread_states = NULL;
	}

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

	wprintf(L"\n");
	message(HAKA_LOG_FATAL, L"core", L"fatal signal received");

	clean_exit();
	exit(1);
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

	/* Default module path */
	module_set_path(HAKA_PREFIX "/share/haka/core/*;" HAKA_PREFIX "/share/haka/modules/*");

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

		if (!has_configuration_script()) {
			message(HAKA_LOG_FATAL, L"core", L"no configuration script set");
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
		int count = thread_get_packet_capture_cpu_count();
		if (!packet_module->multi_threaded()) {
			count = 1;
		}

		thread_states = thread_pool_create(count, packet_module);
		if (check_error()) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			clean_exit();
			return 1;
		}

		if (count > 1) {
			messagef(HAKA_LOG_INFO, L"core", L"starting multi-threaded processing on %i threads", count);
		}
		else {
			message(HAKA_LOG_INFO, L"core", L"starting single threaded processing");
		}

		thread_pool_start(thread_states);
		if (check_error()) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			clean_exit();
			return 1;
		}
	}

	clean_exit();
	return 0;
}
