
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <signal.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <luadebug/debugger.h>

#include "app.h"


static struct thread_pool *thread_states;
static char *configuration_file;


void clean_exit()
{
	set_configuration_script(NULL);

	if (thread_states) {
		thread_pool_cancel(thread_states);

		thread_pool_cleanup(thread_states);
		thread_states = NULL;
	}

	set_packet_module(NULL);
	set_log_module(NULL);
}


static void fatal_error_signal(int sig)
{
	static volatile sig_atomic_t fatal_error_in_progress = 0;

	printf("\n");

	if (sig != 2 || !luadebug_debugger_breakall()) {
		if (fatal_error_in_progress)
			raise(sig);
		fatal_error_in_progress = 1;

		messagef(HAKA_LOG_FATAL, L"core", L"fatal signal received (sig=%d)", sig);

		clean_exit();
		exit(1);
	}
}

void initialize()
{
		/* Set locale */
	setlocale(LC_ALL, "");

	/* Install signal handler */
	signal(SIGTERM, fatal_error_signal);
	signal(SIGINT, fatal_error_signal);
	signal(SIGQUIT, fatal_error_signal);
	signal(SIGHUP, fatal_error_signal);

	/* Default module path */
	{
		static const char *HAKA_CORE_PATH = "/share/haka/core/*";
		static const char *HAKA_MODULE_PATH = "/share/haka/modules/*";

		size_t path_len;
		char *path;
		char *haka_path = getenv("HAKA_PATH");
		if (haka_path == NULL) {
			haka_path = HAKA_PREFIX;
		}

		path_len = 2*strlen(haka_path) + strlen(HAKA_CORE_PATH) + 1 +
				strlen(HAKA_MODULE_PATH) + 1;

		path = malloc(path_len);
		if (!path) {
			fprintf(stderr, "memory allocation error\n");
			clean_exit();
			exit(1);
		}

		strncpy(path, haka_path, path_len);
		strncat(path, HAKA_CORE_PATH, path_len);
		strncat(path, ";", path_len);
		strncat(path, haka_path, path_len);
		strncat(path, HAKA_MODULE_PATH, path_len);

		module_set_path(path);

		free(path);
	}
}

void check()
{
	int err = 0;

	if (!get_packet_module()) {
		message(HAKA_LOG_FATAL, L"core", L"no packet module found");
		err = 1;
	}

	if (!configuration_file) {
		message(HAKA_LOG_FATAL, L"core", L"no configuration script set");
		err = 1;
	}

	if (!has_log_module()) {
		message(HAKA_LOG_WARNING, L"core", L"no log module found");
	}

	if (err) {
		clean_exit();
		exit(1);
	}
}

void prepare(int threadcount)
{
	struct packet_module *packet_module = get_packet_module();
	assert(packet_module);

	if (threadcount == -1) {
		threadcount = thread_get_packet_capture_cpu_count();
		if (!packet_module->multi_threaded()) {
			threadcount = 1;
		}
	}

	thread_states = thread_pool_create(threadcount, packet_module);
	if (check_error()) {
		message(HAKA_LOG_FATAL, L"core", clear_error());
		clean_exit();
		exit(1);
	}

	if (threadcount > 1) {
		messagef(HAKA_LOG_INFO, L"core", L"starting multi-threaded processing on %i threads\n", threadcount);
	}
	else {
		message(HAKA_LOG_INFO, L"core", L"starting single threaded processing\n");
	}
}

void start()
{
	thread_pool_start(thread_states);
	if (check_error()) {
		message(HAKA_LOG_FATAL, L"core", clear_error());
		clean_exit();
		exit(1);
	}
}

int set_configuration_script(const char *file)
{
	free(configuration_file);
	configuration_file = NULL;

	if (file)
		configuration_file = strdup(file);

	return 0;
}

const char *get_configuration_script()
{
	return configuration_file;
}

char directory[1024];

const char *get_app_directory()
{
	return directory;
}
