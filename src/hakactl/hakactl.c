/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <haka/config.h>
#include <haka/version.h>
#include <haka/types.h>
#include <haka/colors.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/module.h>
#include <haka/luadebug/user.h>

#include "config.h"
#include "ctl_comm.h"
#include "commands.h"


enum {
	ERROR_INVALID_OPTIONS = 100,
	ERROR_INVALID_COMMAND = 101,
	ERROR_CTL_SOCKET      = 102
};

static struct command* commands[] = {
	&command_status,
	&command_stop,
	&command_stats,
	&command_logs,
	&command_loglevel,
	&command_debug,
	&command_interactive,
	NULL
};

static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] <command> [arguments]\n", program);
}

static void help(const char *program)
{
	struct command **iter = commands;

	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:          Display this information\n");
	fprintf(stdout, "\t--version:          Display version information\n");

	fprintf(stdout, "\nCommands:\n");
	while (*iter) {
		fprintf(stdout, "\t%s\n", (*iter)->help);
		++iter;
	}
}

static int parse_cmdline(int *argc, char ***argv)
{
	int c;
	int index = 0;
	static struct option long_options[] = {
		{ "version",      no_argument,       0, 'v' },
		{ "help",         no_argument,       0, 'h' },
		{ 0,              0,                 0, 0 }
	};

	while ((c = getopt_long(*argc, *argv, "h", long_options, &index)) != -1) {
		switch (c) {
		case 'h':
			help((*argv)[0]);
			return 0;

		case 'v':
			printf("version %s, arch %s, %s\n", HAKA_VERSION, HAKA_ARCH, HAKA_LUA);
			printf("API version %d\n", HAKA_API_VERSION);
			return 0;

		default:
			usage(stderr, (*argv)[0]);
			return ERROR_INVALID_OPTIONS;
		}
	}

	if (*argc == optind) {
		usage(stderr, (*argv)[0]);
		return ERROR_INVALID_OPTIONS;
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

static void clean_exit()
{
}

static int ctl_open_socket()
{
	int fd;
	struct sockaddr_un addr;
	socklen_t len;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, HAKA_CTL_SOCKET_FILE);

	len = strlen(addr.sun_path) + sizeof(addr.sun_family);
	if (connect(fd, (struct sockaddr *)&addr, len)) {
		return -1;
	}

	return fd;
}

bool use_colors;

int main(int argc, char *argv[])
{
	int ret, fd;
	const char *program = argv[0];
	struct command *command = NULL;

	use_colors = colors_supported(fileno(stdout));

	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		clean_exit();
		return ret;
	}

	if (!module_set_default_path()) {
		fprintf(stderr, "%ls\n", clear_error());
		clean_exit();
		exit(1);
	}

	/* Find commands */
	{
		struct command **iter = commands;
		while (*iter) {
			if (strcasecmp(argv[0], (*iter)->name) == 0) {
				if (argc-1 != (*iter)->nb_args) {
					usage(stderr, program);
					return ERROR_INVALID_COMMAND;
				}

				command = *iter;
				break;
			}
			++iter;
		}
	}

	if (!command) {
		fprintf(stderr, "invalid command '%s', see help for the list of available command\n", argv[0]);
		return ERROR_INVALID_COMMAND;
	}

	/* Connecting to haka */
	if (strcasecmp(argv[0], "status") == 0) {
		printf("[....] haka status");
	}
	else {
		printf("[....] connecting to haka");
	}

	fd = ctl_open_socket();
	if (fd < 0) {
		printf(": cannot connect to haka socket");
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return ERROR_CTL_SOCKET;
	}

	/* Status command */
	if (strcasecmp(argv[0], "STATUS") == 0) {
		fflush(stdout);
		ctl_send_chars(fd, "STATUS", -1);

		if (ctl_expect_chars(fd, "OK")) {
			printf(": haka is running");
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		}
		else {
			printf(": haka not responding");
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}

		return COMMAND_SUCCESS;
	}
	else {
		printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
	}

	/* Other commands */
	assert(command);
	return command->run(fd, argc-1, argv+1);
}
