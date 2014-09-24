/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>

#include <haka/error.h>
#include <haka/alert.h>
#include <haka/alert_module.h>
#include <haka/version.h>
#include <haka/lua/state.h>
#include <haka/luadebug/debugger.h>
#include <haka/luadebug/interactive.h>
#include <haka/luadebug/user.h>
#include <haka/container/vector.h>

#include "app.h"
#include "thread.h"
#include "ctl.h"
#include "config.h"


#define HAKA_CONFIG "/etc/haka/haka.conf"


static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options]\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:               Display this information\n");
	fprintf(stdout, "\t--version:               Display version information\n");
	fprintf(stdout, "\t-c,--config <conf>:      Load a specific configuration file\n"
			"\t                           (default: " HAKA_CONFIG ")\n");
	fprintf(stdout, "\t-r,--rule <rule>:        Override the rule configuration file\n");
	fprintf(stdout, "\t-d,--debug:              Display debug output\n");
	fprintf(stdout, "\t--opt <section>:<key>[=<value>]:\n");
	fprintf(stdout, "\t                          Override configuration parameter\n");
	fprintf(stdout, "\t-l,--loglevel <level>:    Set the log level\n");
	fprintf(stdout, "\t                            (debug, info, warning, error or fatal)\n");
	fprintf(stdout, "\t--debug-lua:              Activate lua debugging (and keep haka in foreground)\n");
	fprintf(stdout, "\t--dump-dissector-graph:   Dump dissector internals (grammar and state machine) in file <name>.dot\n");
	fprintf(stdout, "\t--no-daemon:              Do no run in the background\n");
	fprintf(stdout, "\t--pid-file <pid-file>     Full path to pid file\n"
			"\t                            (default: " HAKA_PID_FILE ")\n");
	fprintf(stdout, "\t--ctl-file <ctl-file>     Full path to socket control file\n"
			"\t                            (default: " HAKA_CTL_SOCKET_FILE ")\n");
}

static bool  daemonize = true;
static char *config = NULL;
static bool  lua_debugger = false;
static bool  dissector_graph = false;
static char *pid_file_path = NULL;
static char *ctl_file_path = NULL;

struct config_override {
	char *key;
	char *value;
};

void free_config_override(void *_elem) {
	struct config_override *elem = (struct config_override *)_elem;
	free(elem->key);
	free(elem->value);
}

static struct vector config_overrides = VECTOR_INIT(struct config_override, free_config_override);

static void add_override(const char *key, const char *value)
{
	struct config_override *override = vector_push(&config_overrides, struct config_override);
	if (!override) {
		message(HAKA_LOG_FATAL, "core", "memory error");
		exit(2);
	}

	override->key = strdup(key);
	override->value = strdup(value);
	if (!override->key || !override->value) {
		message(HAKA_LOG_FATAL, "core", "memory error");
		clean_exit();
		exit(2);
	}
}

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;

	static struct option long_options[] = {
		{ "version",              no_argument,       0, 'v' },
		{ "help",                 no_argument,       0, 'h' },
		{ "config",               required_argument, 0, 'c' },
		{ "debug",                no_argument,       0, 'd' },
		{ "loglevel",             required_argument, 0, 'l' },
		{ "debug-lua",            no_argument,       0, 'L' },
		{ "dump-dissector-graph", no_argument,       0, 'G' },
		{ "no-daemon",            no_argument,       0, 'D' },
		{ "opt",                  required_argument, 0, 'o' },
		{ "rule",                 required_argument, 0, 'r' },
		{ "pid-file",             required_argument, 0, 'P' },
		{ "ctl-file",             required_argument, 0, 'S' },
		{ 0,                      0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "dl:hc:r:P:S:", long_options, &index)) != -1) {
		switch (c) {
		case 'd':
			add_override("log:level", "debug");
			break;

		case 'l':
			add_override("log:level", optarg);
			break;

		case 'h':
			help((*argv)[0]);
			return 0;

		case 'v':
			printf("version %s, arch %s, %s\n", HAKA_VERSION, HAKA_ARCH, HAKA_LUA);
			printf("API version %d\n", HAKA_API_VERSION);
			return 0;

		case 'D':
			daemonize = false;
			break;

		case 'c':
			config = strdup(optarg);
			if (!config) {
				message(HAKA_LOG_FATAL, "core", "memory error");
				clean_exit();
				exit(2);
			}
			break;

		case 'r':
			add_override("general:configuration", optarg);
			break;

		case 'L':
			lua_debugger = true;
			daemonize = false;
			break;

		case 'G':
			dissector_graph = true;
			break;

		case 'o':
			{
				char *value = strchr(optarg, '=');
				if (value) {
					*value = '\0';
					++value;
				}
				add_override(optarg, value);
			}
			break;

		case 'P':
			pid_file_path = strdup(optarg);
			if (!pid_file_path) {
				message(HAKA_LOG_FATAL, "core", "memory error");
				clean_exit();
				exit(2);
			}
			break;

		case 'S':
			ctl_file_path = strdup(optarg);
			if (!ctl_file_path) {
				message(HAKA_LOG_FATAL, "core", "memory error");
				clean_exit();
				exit(2);
			}
			break;

		default:
			usage(stderr, (*argv)[0]);
			return 2;
		}
	}

	if (optind != *argc) {
		usage(stderr, (*argv)[0]);
		return 2;
	}

	if (!config) {
		config = strdup(HAKA_CONFIG);
		if (!config) {
			message(HAKA_LOG_FATAL, "core", "memory error");
			clean_exit();
			exit(2);
		}
	}

	if (!pid_file_path) {
		pid_file_path = strdup(HAKA_PID_FILE);
		if (!pid_file_path) {
			message(HAKA_LOG_FATAL, "core", "memory error");
			clean_exit();
			exit(2);
		}
	}

	if (!ctl_file_path) {
		ctl_file_path = strdup(HAKA_CTL_SOCKET_FILE);
		if (!ctl_file_path) {
			message(HAKA_LOG_FATAL, "core", "memory error");
			clean_exit();
			exit(2);
		}
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

int read_configuration(const char *file)
{
	struct parameters *config = parameters_open(file);
	if (check_error()) {
		message(HAKA_LOG_FATAL, "core", clear_error());
		return 2;
	}

	/* Apply configuration overrides */
	{
		int i;
		const int size = vector_count(&config_overrides);
		for (i=0; i<size; ++i) {
			struct config_override *override = vector_get(&config_overrides, struct config_override, i);
			if (override->value) {
				parameters_set_string(config, override->key, override->value);
			}
			else {
				parameters_set_boolean(config, override->key, true);
			}
		}

		vector_destroy(&config_overrides);
	}

	/* Thread count */
	{
		const int thread_count = parameters_get_integer(config, "general:thread", -1);
		if (thread_count > 0) {
			thread_set_packet_capture_cpu_count(thread_count);
		}
	}

	/* Log level */
	{
		const char *_level = parameters_get_string(config, "log:level", "info");
		if (_level) {
			char *level = strdup(_level);
			if (!level) {
				message(HAKA_LOG_FATAL, "core", "memory error");
				clean_exit();
				exit(1);
			}

			if (!setup_loglevel(level)) {
				message(HAKA_LOG_FATAL, "core", clear_error());
				clean_exit();
				exit(1);
			}

			free(level);
		}

	}

	/* Logging module */
	{
		const char *module;

		parameters_open_section(config, "log");
		module = parameters_get_string(config, "module", NULL);
		if (module) {
			struct logger *logger;

			struct module *logger_module = module_load(module, config);
			if (!logger_module) {
				messagef(HAKA_LOG_FATAL, "core", "cannot load logging module: %s", clear_error());
				clean_exit();
				return 1;
			}

			logger = log_module_logger(logger_module, config);
			if (!logger) {
				messagef(HAKA_LOG_FATAL, "core", "cannot initialize logging module: %s", clear_error());
				module_release(logger_module);
				clean_exit();
				return 1;
			}

			if (!add_logger(logger)) {
				messagef(HAKA_LOG_FATAL, "core", "cannot install logging module: %s", clear_error());
				logger->destroy(logger);
				module_release(logger_module);
				clean_exit();
				return 1;
			}

			module_release(logger_module);
		}
	}

	/* Alert module */
	{
		const char *module;

		parameters_open_section(config, "alert");
		module = parameters_get_string(config, "module", NULL);
		if (module) {
			struct alerter *alerter;

			struct module *alerter_module = module_load(module, config);
			if (!alerter_module) {
				messagef(HAKA_LOG_FATAL, "core", "cannot load alert module: %s", clear_error());
				clean_exit();
				return 1;
			}

			alerter = alert_module_alerter(alerter_module, config);
			if (!alerter) {
				messagef(HAKA_LOG_FATAL, "core", "cannot initialize alert module: %s", clear_error());
				module_release(alerter_module);
				clean_exit();
				return 1;
			}

			if (!add_alerter(alerter)) {
				messagef(HAKA_LOG_FATAL, "core", "cannot install alert module: %s", clear_error());
				alerter->destroy(alerter);
				module_release(alerter_module);
				clean_exit();
				return 1;
			}

			module_release(alerter_module);
		}

		if (!daemonize && parameters_get_boolean(config, "alert_on_stdout", true)) {
			/* Also print alert to stdout */
			/* Load file alerter */
			struct module *module = NULL;
			struct alerter *alerter = NULL;
			struct parameters *args = parameters_create();

			parameters_set_string(args, "format", "pretty");

			module = module_load("alert/file", NULL);
			if (!module) {
				messagef(HAKA_LOG_FATAL, "core", "cannot load default alert module: %s", clear_error());
				clean_exit();
				return 1;
			}

			alerter = alert_module_alerter(module, args);
			add_alerter(alerter);
			module_release(module);
		}
	}

	/* Packet module */
	{
		const char *module;

		parameters_open_section(config, "packet");
		module = parameters_get_string(config, "module", NULL);
		if (module) {
			struct module *packet = module_load(module, config);
			if (!packet) {
				messagef(HAKA_LOG_FATAL, "core", "cannot load packet module: %s", clear_error());
				clean_exit();
				return 1;
			}

			set_packet_module(packet);
			module_release(packet);
		}
		else {
			message(HAKA_LOG_FATAL, "core", "no packet module specified");
			clean_exit();
			return 1;
		}
	}

	/* Other options */
	parameters_open_section(config, "general");

	{
		const char *configuration = parameters_get_string(config, "configuration", NULL);
		if (!configuration) {
			message(HAKA_LOG_FATAL, "core", "no configuration specified");
			clean_exit();
			return 1;
		}
		else {
			set_configuration_script(configuration);
		}
	}

	parameters_free(config);

	return -1;
}

static bool haka_started = false;

void clean_exit()
{
	stop_ctl_server();

	if (haka_started) {
		unlink(pid_file_path);
	}

	vector_destroy(&config_overrides);

	basic_clean_exit();
}

bool check_running_haka()
{
	pid_t pid;
	FILE *pid_file = fopen(pid_file_path, "r");
	if (!pid_file) {
		return false;
	}

	if (fscanf(pid_file, "%i", &pid) != 1) {
		message(HAKA_LOG_WARNING, "core", "malformed pid file");
		return false;
	}

	if (kill(pid, 0) || errno == ESRCH) {
		return false;
	}

	message(HAKA_LOG_FATAL, "core", "an instance of haka is already running");
	return true;
}

static void terminate()
{
	_exit(2);
}

static void terminate_ok()
{
	_exit(0);
}

int main(int argc, char *argv[])
{
	int ret;
	FILE *pid_file = NULL;
	pid_t parent = getpid();

	initialize();

	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		free(config);
		free(pid_file_path);
		free(ctl_file_path);
		clean_exit();
		return ret;
	}

	if (check_running_haka()) {
		clean_exit();
		return 5;
	}

	ret = read_configuration(config);
	free(config);
	if (ret >= 0) {
		clean_exit();
		return ret;
	}

	haka_started = true;

	ret = prepare_ctl_server(ctl_file_path);
	free(ctl_file_path);
	if (!ret) {
		clean_exit();
		return 2;
	}

	{
		struct luadebug_user *user = luadebug_user_readline();
		if (!user) {
			message(HAKA_LOG_FATAL, "core", "cannot create readline handler");
			clean_exit();
			return 2;
		}

		luadebug_debugger_user(user);
		luadebug_interactive_user(user);
		luadebug_user_release(&user);
	}

	if (daemonize) {
		pid_t child;

		child = fork();
		if (child == -1) {
			message(HAKA_LOG_FATAL, "core", "failed to daemonize");
			clean_exit();
			return 1;
		}

		if (child != 0) {
			int status;

			signal(SIGTERM, terminate);
			signal(SIGINT, terminate);
			signal(SIGQUIT, terminate);
			signal(SIGHUP, terminate_ok);

			wait(&status);
			_exit(2);
		}
	}

	prepare(-1, lua_debugger, dissector_graph);

	pid_file = fopen(pid_file_path, "w");
	if (!pid_file) {
		message(HAKA_LOG_FATAL, "core", "cannot create pid file");
		clean_exit();
		return 1;
	}

	if (!start_ctl_server()) {
		clean_exit();
		return 2;
	}

	{
		const pid_t pid = getpid();
		fprintf(pid_file, "%i\n", pid);
		fclose(pid_file);
		pid_file = NULL;
	}

	if (daemonize) {
		luadebug_debugger_user(NULL);
		luadebug_interactive_user(NULL);

		message(HAKA_LOG_INFO, "core", "switch to background");

		{
			const int nullfd = open("/dev/null", O_RDWR);
			if (nullfd == -1) {
				message(HAKA_LOG_FATAL, "core", "failed to daemonize");
				fclose(pid_file);
				clean_exit();
				return 1;
			}

			dup2(nullfd, STDOUT_FILENO);
			dup2(nullfd, STDERR_FILENO);
			dup2(nullfd, STDIN_FILENO);

			enable_stdout_logging(false);
		}

		kill(parent, SIGHUP);
	}

	start();

	message(HAKA_LOG_INFO, "core", "stopping haka");

	clean_exit();
	return 0;
}
