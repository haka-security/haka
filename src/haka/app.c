/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <libgen.h>
#include <unistd.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/system.h>
#include <haka/error.h>
#include <haka/alert.h>
#include <haka/luadebug/debugger.h>

#include "app.h"


static struct thread_pool *thread_states;
static char *configuration_file;
static int ret_rc = 0;
extern void packet_set_mode(enum packet_mode mode);


void basic_clean_exit()
{
	set_configuration_script(NULL);

	if (thread_states) {
		thread_pool_cleanup(thread_states);
		thread_states = NULL;
	}

	set_packet_module(NULL);
	remove_all_logger();
}

static void term_error_signal(int sig, siginfo_t *si, void *uc)
{
	static atomic_t fatal_error_count;
	int fatal_error_level;

	if (sig == SIGINT) {
		write(STDERR_FILENO, "\n", 1);

		if (luadebug_debugger_breakall()) {
			static const char *debugger_message = "break (hit ^C again to kill)\n";
			write(STDERR_FILENO, debugger_message, strlen(debugger_message));
			return;
		}
		else {
			luadebug_debugger_shutdown();
		}
	}

	fatal_error_level = atomic_inc(&fatal_error_count);

	if (sig != SIGINT) {
		static const char *message = "terminate signal received\n";
		write(STDERR_FILENO, message, strlen(message));
	}

	if (thread_states) {
		if (!thread_pool_stop(thread_states, fatal_error_level)) {
			fatal_exit(1);
		}
	}
	else {
		fatal_exit(1);
	}
}

static void handle_sighup()
{
	enable_stdout_logging(false);
}

void initialize()
{
	struct sigaction sa;

	/* Set locale */
	setlocale(LC_ALL, "");

	/* Install signal handler */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = term_error_signal;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGTERM, &sa, NULL) ||
	    sigaction(SIGINT, &sa, NULL) ||
	    sigaction(SIGQUIT, &sa, NULL)) {
		messagef(HAKA_LOG_FATAL, L"core", L"%s", errno_error(errno));
		clean_exit();
		exit(1);
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, handle_sighup);

	if (!module_set_default_path()) {
		fprintf(stderr, "%ls\n", clear_error());
		clean_exit();
		exit(1);
	}
}

void prepare(int threadcount, bool attach_debugger, bool grammar_debug, bool state_machine_debug)
{
	struct packet_module *packet_module = get_packet_module();
	assert(packet_module);

	if (threadcount == -1) {
		threadcount = thread_get_packet_capture_cpu_count();
		if (!packet_module->multi_threaded()) {
			threadcount = 1;
		}
	}

	if (packet_module->pass_through()) {
		messagef(HAKA_LOG_INFO, L"core", L"setting packet mode to pass-through\n");
		packet_set_mode(MODE_PASSTHROUGH);
	}

	messagef(HAKA_LOG_INFO, L"core", L"loading rule file '%s'", configuration_file);

	/* Add module path to the configuration folder */
	{
		char *module_path;

		module_path = malloc(strlen(configuration_file) + 3);
		assert(module_path);
		strcpy(module_path, configuration_file);
		dirname(module_path);
		strcat(module_path, "/*");

		module_add_path(module_path);
		if (check_error()) {
			message(HAKA_LOG_FATAL, L"core", clear_error());
			free(module_path);
			clean_exit();
			exit(1);
		}

		free(module_path);
		module_path = NULL;
	}

	thread_states = thread_pool_create(threadcount, packet_module,
			attach_debugger, grammar_debug, state_machine_debug);
	if (!thread_states) {
		assert(check_error());
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

	if (ret_rc) {
		clean_exit();
		exit(ret_rc);
	}
}

struct thread_pool *get_thread_pool()
{
	return thread_states;
}

int set_configuration_script(const char *file)
{
	free(configuration_file);
	configuration_file = NULL;

	if (file) {
		configuration_file = strdup(file);
	}

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

bool setup_loglevel(char *level)
{
	while (true) {
		char *value;
		wchar_t *module=NULL;
		log_level loglevel;

		char *next_level = strchr(level, ',');
		if (next_level) {
			*next_level = '\0';
			++next_level;
		}

		value = strchr(level, '=');
		if (value) {
			*value = '\0';
			++value;

			module = malloc(sizeof(wchar_t)*(strlen(level)+1));
			if (!module) {
				error(L"memory error");
				return false;
			}

			if (mbstowcs(module, level, strlen(level)+1) == -1) {
				error(L"invalid module string");
				return false;
			}
		}
		else {
			value = level;
		}

		loglevel = str_to_level(value);
		if (check_error()) {
			return false;
		}

		setlevel(loglevel, module);

		if (module) free(module);

		if (next_level) level = next_level;
		else break;
	}

	return true;
}
