
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

struct thread_states {
	int                   count;
	struct thread_state **states;
};
static struct thread_states thread_states;


/* Clean up lua state and loaded modules */
static void clean_exit()
{
	struct packet_module *packet_module = get_packet_module();

	set_configuration_script(NULL);

	if (thread_states.count > 0 && packet_module) {
		int i;
		for (i=0; i<thread_states.count; ++i) {
			if (thread_states.states[i]) {
				cleanup_thread_state(packet_module, thread_states.states[i]);
				thread_states.states[i] = NULL;
			}
		}
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

	/* Get the program directory */
	{
		char szTmp[32];
		int bytes;
		char *sep;

		sprintf(szTmp, "/proc/%d/exe", getpid());
		bytes = readlink(szTmp, directory, sizeof(directory)-1);
		if (bytes < 0) {
			message(HAKA_LOG_FATAL, L"core", L"failed to retrieve application directory");
			clean_exit();
			return 3;
		}
		directory[bytes] = '\0';

		sep = strrchr(directory, '/');
		if (!sep) {
			message(HAKA_LOG_FATAL, L"core", L"failed to retrieve application directory");
			clean_exit();
			return 3;
		}
		*sep = '\0';
	}

	/* Default module path */
	{
		static const char append[] = "/modules/*";
		char *path = malloc(strlen(directory) + strlen(append) + 1);
		if (!path) {
			clean_exit();
			return 1;
		}

		strcpy(path, directory);
		strcpy(path + strlen(directory), append);
		module_set_path(path);
		free(path);
	}

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
		int i;

		thread_states.count = thread_get_packet_capture_cpu_count();
		if (!packet_module->multi_threaded())
			thread_states.count = 1;

		assert(thread_states.count > 0);

		thread_states.states = malloc(sizeof(struct packet_module_state *) * thread_states.count);
		if (!thread_states.states) {
			message(HAKA_LOG_FATAL, L"core", L"memory error");
			clean_exit();
			return 1;
		}

		memset(thread_states.states, 0, sizeof(struct packet_module_state *) * thread_states.count);

		for (i=0; i<thread_states.count; ++i) {
			thread_states.states[i] = init_thread_state(packet_module, i);
			if (!thread_states.states[i]) {
				clean_exit();
				return 1;
			}
		}

		if (thread_states.count > 1) {
			messagef(HAKA_LOG_INFO, L"core", L"starting multi-threaded processing on %i threads", thread_states.count);

			for (i=0; i<thread_states.count; ++i) {
				start_thread(thread_states.states[i]);
				if (check_error()) {
					message(HAKA_LOG_FATAL, L"core", clear_error());
					clean_exit();
					return 1;
				}
			}

			wait_threads();
		}
		else {
			message(HAKA_LOG_INFO, L"core", L"starting single threaded processing");
			start_single(thread_states.states[0]);
		}
	}

	clean_exit();
	return 0;
}
