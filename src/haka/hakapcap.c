/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/alert.h>
#include <haka/alert_module.h>
#include <haka/error.h>
#include <haka/version.h>
#include <haka/parameters.h>
#include <haka/lua/state.h>
#include <haka/luadebug/user.h>
#include <haka/luadebug/debugger.h>
#include <haka/luadebug/interactive.h>

#include "app.h"
#include "thread.h"


static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] <config> <pcapfile>\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:              Display this information\n");
	fprintf(stdout, "\t--version:              Display version information\n");
	fprintf(stdout, "\t-d,--debug:             Display debug output\n");
	fprintf(stdout, "\t-l,--loglevel <level>:  Set the log level\n");
	fprintf(stdout, "\t                          (debug, info, warning, error or fatal)\n");
	fprintf(stdout, "\t-a,--alert-to <file>:   Redirect alerts to given file\n");
	fprintf(stdout, "\t--debug-lua:            Activate lua debugging\n");
	fprintf(stdout, "\t--dump-dissector-graph: Dump dissector internals (grammar and state machine) in file <name>.dot\n");
	fprintf(stdout, "\t--no-pass-through, --pass-through:\n");
	fprintf(stdout, "\t                        Select pass-through mode (default: true)\n");
	fprintf(stdout, "\t-o <output>:            Save result in a pcap file\n");
}

static char *output = NULL, *alert_to = NULL;
static bool pass_through = true;
static bool lua_debugger = false;
static bool dissector_graph = false;

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;

	static struct option long_options[] = {
		{ "version",              no_argument,       0, 'v' },
		{ "help",                 no_argument,       0, 'h' },
		{ "debug",                no_argument,       0, 'd' },
		{ "loglevel",             required_argument, 0, 'l' },
		{ "alert-to",             required_argument, 0, 'a' },
		{ "debug-lua",            no_argument,       0, 'L' },
		{ "dump-dissector-graph", no_argument,       0, 'G' },
		{ "no-pass-through",      no_argument,       0, 'p' },
		{ "pass-through",         no_argument,       0, 'P' },
		{ 0,                      0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "dl:a:ho:", long_options, &index)) != -1) {
		switch (c) {
		case 'd':
			setlevel(HAKA_LOG_DEBUG, NULL);
			break;

		case 'l':
			if (!setup_loglevel(optarg)) {
				message(HAKA_LOG_FATAL, L"core", clear_error());
				clean_exit();
				exit(1);
			}
			break;

		case 'a':
			alert_to = strdup(optarg);
			break;

		case 'h':
			help((*argv)[0]);
			return 0;

		case 'v':
			printf("version %s, arch %s, %s\n", HAKA_VERSION, HAKA_ARCH, HAKA_LUA);
			printf("API version %d\n", HAKA_API_VERSION);
			return 0;

		case 'p':
			pass_through = false;
			break;

		case 'P':
			pass_through = true;
			break;

		case 'o':
			output = strdup(optarg);
			break;

		case 'L':
			lua_debugger = true;
			break;

		case 'G':
			dissector_graph = true;
			break;

		default:
			usage(stderr, (*argv)[0]);
			return 2;
		}
	}

	if (optind != *argc-2) {
		usage(stderr, (*argv)[0]);
		return 2;
	}

	*argc -= optind;
	*argv += optind;

	return -1;
}

void clean_exit()
{
	basic_clean_exit();
}

int main(int argc, char *argv[])
{
	int ret;

	initialize();

	/* Check arguments */
	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		clean_exit();
		free(alert_to);
		free(output);
		return ret;
	}

	/* Select and initialize modules */
	{
		struct module *pcap = NULL;
		struct parameters *args = parameters_create();

		parameters_set_string(args, "file", argv[1]);
		if (output) {
			parameters_set_string(args, "output", output);
		}

		parameters_set_boolean(args, "pass-through", pass_through);

		pcap = module_load("packet/pcap", args);

		free(output);
		output = NULL;
		parameters_free(args);
		args = NULL;

		if (!pcap) {
			messagef(HAKA_LOG_FATAL, L"core", L"cannot load packet module: %ls", clear_error());
			clean_exit();
			return 1;
		}

		set_packet_module(pcap);
		module_release(pcap);
		free(output);
	}

	/* Start alert module */
	{
		/* Also print alert to stdout */
		/* Load file alerter */
		struct module *module = NULL;
		struct alerter *alerter = NULL;
		struct parameters *args = parameters_create();

		parameters_set_string(args, "file", alert_to);
		parameters_set_string(args, "format", "pretty");

		free(alert_to);
		alert_to = NULL;

		module = module_load("alert/file", NULL);
		if (!module) {
			messagef(HAKA_LOG_FATAL, L"core", L"cannot load alert module: %ls", clear_error());
			clean_exit();
			return 1;
		}

		alerter = alert_module_alerter(module, args);
		if (!alerter) {
			messagef(HAKA_LOG_FATAL, L"core", L"cannot load alert module: %ls", clear_error());
			clean_exit();
			return 1;
		}

		add_alerter(alerter);

		parameters_free(args);
		module_release(module);
	}

	/* Select configuration */
	set_configuration_script(argv[0]);

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

	/* Main loop */
	prepare(1, lua_debugger, dissector_graph);
	start();

	clean_exit();
	return 0;
}
