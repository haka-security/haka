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

#include <haka/packet_module.h>
#include <haka/thread.h>
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

static void term_error_signal(int sig)
{
	static volatile sig_atomic_t fatal_error_in_progress = 0;

	if (sig == SIGINT) {
		printf("\n");
		if (luadebug_debugger_breakall()) {
			message(HAKA_LOG_FATAL, L"debug", L"break (hit ^C again to kill)");
			return;
		}
		else {
			luadebug_debugger_shutdown();
		}
	}

	if (fatal_error_in_progress)
		raise(sig);
	fatal_error_in_progress = 1;

	if (sig != SIGTERM) {
		messagef(HAKA_LOG_FATAL, L"core", L"fatal signal received (sig=%d)", sig);
	}
	else {
		messagef(HAKA_LOG_INFO, L"core", L"terminate signal received");
	}

	if (thread_states) {
		if (thread_pool_issingle(thread_states)) {
			clean_exit();
			exit(1);
		}
		else {
			thread_pool_cancel(thread_states);
			ret_rc = 1;
		}
	}
	else {
		clean_exit();
		exit(1);
	}
}

static void handle_sighup()
{
	enable_stdout_logging(false);
}

void initialize()
{
	/* Set locale */
	setlocale(LC_ALL, "");

	/* Install signal handler */
	signal(SIGTERM, term_error_signal);
	signal(SIGINT, term_error_signal);
	signal(SIGQUIT, term_error_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, handle_sighup);

	if (!module_set_default_path()) {
		fprintf(stderr, "%ls\n", clear_error());
		clean_exit();
		exit(1);
	}
}

void prepare(int threadcount, bool attach_debugger, bool grammar_debug)
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

	thread_states = thread_pool_create(threadcount, packet_module, attach_debugger, grammar_debug);
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

void dump_stat(FILE *file)
{
	thread_pool_dump_stat(thread_states, file);
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
