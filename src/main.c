
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/version.h>
#include <haka/lua/state.h>
#include <luadebug/debugger.h>

#include "app.h"
#include "thread.h"
#include "lua/state.h"


static struct lua_state *global_lua_state;
static struct thread_pool *thread_states;

extern void packet_set_mode(enum packet_mode mode);


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
		lua_state_close(global_lua_state);
		global_lua_state = NULL;
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

static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] script_file [...]\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:       Display this information\n");
	fprintf(stdout, "\t--version:       Display version information\n");
	fprintf(stdout, "\t-d,--debug:      Display debug output\n");
	fprintf(stdout, "\t--daemon:        Run in the background\n");
	fprintf(stdout, "\t-jN:             Use N threads for packet capture (if supported)\n");
	fprintf(stdout, "\t--pass-through:  Run in pass-through mode\n");
}

static bool daemonize = false;
static bool pass_throught = false;

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;

	static struct option long_options[] = {
		{ "version",      no_argument,       0, 'v' },
		{ "help",         no_argument,       0, 'h' },
		{ "debug",        no_argument,       0, 'd' },
		{ "daemon",       no_argument,       0, 'D' },
		{ "pass-through", no_argument,       0, 'P' },
		{ 0,              0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "dhj:", long_options, &index)) != -1) {
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
			daemonize = true;
			break;

		case 'P':
			pass_throught = true;
			break;

		case 'j':
			{
				const int thread_count = atoi(optarg);
				if (thread_count > 0) {
					thread_set_packet_capture_cpu_count(thread_count);
				}
				else {
					usage(stderr, (*argv)[0]);
					fprintf(stderr, "invalid thread count\n");
					return 2;
				}
			}
			break;

		default:
			usage(stderr, (*argv)[0]);
			return 2;
		}
	}

	if (optind >= *argc) {
		usage(stderr, (*argv)[0]);
		return 2;
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

int main(int argc, char *argv[])
{
	int ret;

	/* Set locale */
	setlocale(LC_ALL, "");

	/* Install signal handler */
	signal(SIGTERM, fatal_error_signal);
	signal(SIGINT, fatal_error_signal);
	signal(SIGQUIT, fatal_error_signal);
	signal(SIGHUP, fatal_error_signal);

	/* Default module path */
	module_set_path(HAKA_PREFIX "/share/haka/core/*;"
			HAKA_PREFIX "/share/haka/modules/*");

	/* Check arguments */
	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		clean_exit();
		return ret;
	}

	/* Init lua vm */
	global_lua_state = haka_init_state();

	/* Execute configuration file */
	if (run_file(global_lua_state->L, argv[0], argc-1, argv+1)) {
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

	if (pass_throught) {
		messagef(HAKA_LOG_INFO, L"core", L"setting packet mode to pass-through\n");
		packet_set_mode(MODE_PASSTHROUGH);
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
			messagef(HAKA_LOG_INFO, L"core", L"starting multi-threaded processing on %i threads\n", count);
		}
		else {
			message(HAKA_LOG_INFO, L"core", L"starting single threaded processing\n");
		}

		/* Deamonize if needed */
		if (daemonize) {
			if (daemon(1, 0)) {
				message(HAKA_LOG_FATAL, L"core", L"failed to daemonize");
				clean_exit();
				return 1;
			}
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
