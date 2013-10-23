
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
#include <haka/version.h>
#include <haka/lua/state.h>
#include <luadebug/debugger.h>
#include <luadebug/interactive.h>
#include <luadebug/user.h>

#include "app.h"
#include "thread.h"
#include "ctl.h"
#include "lua/state.h"
#include "config.h"


#define HAKA_CONFIG "/etc/haka/haka.conf"


extern void packet_set_mode(enum packet_mode mode);

static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options]\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:          Display this information\n");
	fprintf(stdout, "\t--version:          Display version information\n");
	fprintf(stdout, "\t-c,--config <conf>: Load a specific configuration file\n"
					"\t                      (default: %s/etc/haka/haka.conf)\n", haka_path());
	fprintf(stdout, "\t-d,--debug:         Display debug output\n");
	fprintf(stdout, "\t--luadebug:         Attach lua debugger (and keep haka in foreground)\n");
	fprintf(stdout, "\t--no-daemon:        Do no run in the background\n");
}

static bool daemonize = true;
static char *config = NULL;
static bool lua_debugger = false;

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;

	static struct option long_options[] = {
		{ "version",      no_argument,       0, 'v' },
		{ "help",         no_argument,       0, 'h' },
		{ "config",       required_argument, 0, 'c' },
		{ "debug",        no_argument,       0, 'd' },
		{ "luadebug",     no_argument,       0, 'L' },
		{ "no-daemon",    no_argument,       0, 'D' },
		{ 0,              0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "dhc:", long_options, &index)) != -1) {
		switch (c) {
		case 'd':
			setlevel(HAKA_LOG_DEBUG, NULL);
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
			break;

		case 'L':
			lua_debugger = true;
			daemonize = false;
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
		const char *haka_path_s = haka_path();

		config = malloc(strlen(haka_path_s) + strlen(HAKA_CONFIG) + 1);
		assert(config);
		strcpy(config, haka_path_s);
		strcat(config, HAKA_CONFIG);
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

int read_configuration(const char *file)
{
	struct parameters *config = parameters_open(file);
	if (check_error()) {
		message(HAKA_LOG_FATAL, L"core", clear_error());
		return 2;
	}

	/* Thread count */
	{
		const int thread_count = parameters_get_integer(config, "general:thread", -1);
		if (thread_count > 0) {
			thread_set_packet_capture_cpu_count(thread_count);
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
				message(HAKA_LOG_FATAL, L"core", L"cannot load logging module");
				clean_exit();
				return 1;
			}

			logger = log_module_logger(logger_module, config);
			if (!logger) {
				messagef(HAKA_LOG_FATAL, L"core", L"cannot initialize logging module: %s", clear_error());
				module_release(logger_module);
				clean_exit();
				return 1;
			}

			if (!add_logger(logger)) {
				messagef(HAKA_LOG_FATAL, L"core", L"cannot install logging module: %s", clear_error());
				logger->destroy(logger);
				module_release(logger_module);
				clean_exit();
				return 1;
			}

			module_release(logger_module);
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
				message(HAKA_LOG_FATAL, L"core", L"cannot load packet module");
				clean_exit();
				return 1;
			}

			set_packet_module(packet);
			module_release(packet);
		}
		else {
			message(HAKA_LOG_FATAL, L"core", L"no packet module specified");
			clean_exit();
			return 1;
		}
	}

	/* Other options */
	parameters_open_section(config, "general");

	if (parameters_get_boolean(config, "pass-through", false)) {
		messagef(HAKA_LOG_INFO, L"core", L"setting packet mode to pass-through\n");
		packet_set_mode(MODE_PASSTHROUGH);
	}

	{
		const char *configuration = parameters_get_string(config, "configuration", NULL);
		if (!configuration) {
			message(HAKA_LOG_FATAL, L"core", L"no configuration specified");
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
		unlink(HAKA_PID_FILE);
	}

	basic_clean_exit();
}

bool check_running_haka()
{
	pid_t pid;
	FILE *pid_file = fopen(HAKA_PID_FILE, "r");
	if (!pid_file) {
		return false;
	}

	if (fscanf(pid_file, "%i", &pid) != 1) {
		message(HAKA_LOG_WARNING, L"core", L"malformed pid file");
		return false;
	}

	if (kill(pid, 0) == -1 && errno == ESRCH) {
		return false;
	}

	message(HAKA_LOG_FATAL, L"core", L"an instance of haka is already running");
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

	if (!prepare_ctl_server()) {
		clean_exit();
		return 2;
	}

	{
		struct luadebug_user *user = luadebug_user_readline();
		if (!user) {
			message(HAKA_LOG_FATAL, L"core", L"cannot create readline handler");
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
			message(HAKA_LOG_FATAL, L"core", L"failed to daemonize");
			fclose(pid_file);
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

	prepare(-1, lua_debugger);

	pid_file = fopen(HAKA_PID_FILE, "w");
	if (!pid_file) {
		message(HAKA_LOG_FATAL, L"core", L"cannot create pid file");
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

		message(HAKA_LOG_INFO, L"core", L"switch to background");

		{
			const int nullfd = open("/dev/null", O_RDWR);
			if (nullfd == -1) {
				message(HAKA_LOG_FATAL, L"core", L"failed to daemonize");
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

	clean_exit();
	return 0;
}
