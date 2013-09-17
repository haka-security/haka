
#include <sys/socket.h>
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

#include "config.h"
#include "ctl.h"


enum {
	COMMAND_SUCCESS       = 0,
	COMMAND_FAILED        = 1,
	ERROR_INVALID_OPTIONS = 100,
	ERROR_INVALID_COMMAND = 101,
	ERROR_CTL_SOCKET      = 102
};

static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] <command>\n", program);
}

static void help(const char *program)
{
	usage(stdout, program);

	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h,--help:          Display this information\n");
	fprintf(stdout, "\t--version:          Display version information\n");

	fprintf(stdout, "\nCommands:\n");
	fprintf(stdout, "\tstatus:             Display haka daemon status\n");
	fprintf(stdout, "\tstop:               Stop haka daemon\n");
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

	if (optind+1 != *argc) {
		usage(stderr, (*argv)[0]);
		return ERROR_INVALID_COMMAND;
	}

	*argc -= optind;
	*argv += optind;
	return -1;
}

static void clean_exit()
{
}

static bool display_log_line(int fd)
{
	#define MESSAGE_BUFSIZE   2048

	log_level level;
	wchar_t module[128];
	wchar_t message[MESSAGE_BUFSIZE];

	if (recv(fd, &level, sizeof(log_level), 0) < 0 ||
		!ctl_receive_wchars(fd, module, 128) ||
		!ctl_receive_wchars(fd, message, MESSAGE_BUFSIZE))
	{
		return false;
	}

	stdout_message(level, module, message);
	return true;
}

int main(int argc, char *argv[])
{
	int ret, fd;
	const bool use_colors = colors_supported(fileno(stdout));

	ret = parse_cmdline(&argc, &argv);
	if (ret >= 0) {
		clean_exit();
		return ret;
	}

	fd = ctl_open_socket();
	if (fd < -1) {
		clean_exit();
		return ERROR_CTL_SOCKET;
	}

	/* Commands */
	if (strcasecmp(argv[0], "STATUS") == 0) {
		printf("[....] haka status");

		if (fd < 0) {
			printf(": cannot connect to haka socket");
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			return COMMAND_FAILED;
		}
		else {
			fflush(stdout);
			ctl_send(fd, "STATUS");

			if (ctl_check(fd, "OK")) {
				printf(": haka is running");
				printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
			}
			else {
				printf(": haka not responding");
				printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			}
			return COMMAND_SUCCESS;
		}
	}

	/* Other command that need a valid ctl socket */
	if (fd < 0) {
		fprintf(stderr, "cannot connect ctl socket: %s\n", strerror(errno));
		return ERROR_CTL_SOCKET;
	}

	if (strcasecmp(argv[0], "STOP") == 0) {
		printf("[....] stopping haka");
		fflush(stdout);

		ctl_send(fd, "STOP");
		if (ctl_check(fd, "OK")) {
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		}
		else {
			printf("failed!");
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}
	}
	else if (strcasecmp(argv[0], "LOGS") == 0) {
		printf("[....] connecting to haka");
		fflush(stdout);

		ctl_send(fd, "LOGS");
		if (ctl_check(fd, "OK")) {
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));

			while (display_log_line(fd));
		}
		else {
			printf("failed!");
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}
	}
	else {
		fprintf(stderr, "invalid command '%s', see help for the list of available command\n", argv[0]);
		return ERROR_INVALID_COMMAND;
	}

	return 0;
}
