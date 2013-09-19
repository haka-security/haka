
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
#include <luadebug/user.h>

#include "config.h"
#include "ctl_comm.h"


enum {
	COMMAND_SUCCESS       = 0,
	COMMAND_FAILED        = 1,
	ERROR_INVALID_OPTIONS = 100,
	ERROR_INVALID_COMMAND = 101,
	ERROR_CTL_SOCKET      = 102
};

struct commands {
	char *cmd;
	int  nb_args;
};

static void usage(FILE *output, const char *program)
{
	fprintf(stdout, "Usage: %s [options] <command> [arguments]\n", program);
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
	fprintf(stdout, "\tlogs:               Show haka logs in realtime\n");
	fprintf(stdout, "\tloglevel <level>:   Change haka logging level\n");
	fprintf(stdout, "\tdebug:              Attach a remote lua debugger to haka\n");
	fprintf(stdout, "\tinteractive:        Attach an interactive session to haka\n");
}


static int get_parameters_nb(char *command)
{
	#define NB_COMMAND  6

	static struct commands cmd_options[] = {
		{ "status",		0 },
		{ "stop",		0 },
		{ "logs",		0 },
		{ "loglevel",	1 },
		{ "debug",		0 },
		{ "interactive",0 },
	};

	if (command) {
		int index = 0;
		while(index < NB_COMMAND && strcasecmp(cmd_options[index].cmd, command) != 0)
			index++;
		if (index != NB_COMMAND)
			return cmd_options[index].nb_args;
	}

	return -1;
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

	index = get_parameters_nb(*(argv[0] + 1)) + 1;
	if ((index == 0) || (optind+index != *argc)) {
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

static bool display_log_line(int fd)
{
	log_level level;
	wchar_t *module, *msg;

	level = ctl_recv_int(fd);
	if (check_error()) {
		return false;
	}

	module = ctl_recv_wchars(fd);
	if (!module) {
		return false;
	}

	msg = ctl_recv_wchars(fd);
	if (!msg) {
		return false;
	}

	stdout_message(level, module, msg);

	free(module);
	free(msg);

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

	/* Connecting to haka */
	if (strcasecmp(argv[0], "STATUS") == 0) {
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
		ctl_send_chars(fd, "STATUS");

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
	if (strcasecmp(argv[0], "STOP") == 0) {
		printf("[....] stopping haka");
		fflush(stdout);

		if (!ctl_send_chars(fd, "STOP")) {
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			return COMMAND_FAILED;
		}

		if (ctl_expect_chars(fd, "OK")) {
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		}
		else {
			printf(": %sfailed!%s", c(RED, use_colors), c(CLEAR, use_colors));
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}
	}
	else if (strcasecmp(argv[0], "LOGS") == 0) {
		printf("[....] requesting logs");
		fflush(stdout);

		if (!ctl_send_chars(fd, "LOGS")) {
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			return COMMAND_FAILED;
		}

		if (ctl_expect_chars(fd, "OK")) {
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));

			while (display_log_line(fd));
		}
		else {
			printf(": %sfailed!%s", c(RED, use_colors), c(CLEAR, use_colors));
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}
	}
	else if (strcasecmp(argv[0], "LOGLEVEL") == 0) {
		log_level level;

		printf("[....] changing log level");
		fflush(stdout);

		level = str_to_level(argv[1]);
		if (level != HAKA_LOG_LEVEL_LAST) {
			if ((!ctl_send_chars(fd,"LOGLEVEL")) || (!ctl_send_int(fd, level))) {
				printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
				return COMMAND_FAILED;
			}
			if (ctl_expect_chars(fd, "OK")) {
				printf(": log level set to %s", argv[1]);
				printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
			}
		}
		else {
			int index;

			printf(": invalid log level '%s'", argv[1]);
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));

			printf("       possible log levels are ");
			for (index = 0; index < HAKA_LOG_LEVEL_LAST; index++) {
				if (index != 0) printf(", ");
				printf("%s", level_to_str(index));
			}
			printf("\n");

			return COMMAND_FAILED;
		}
	}
	else if (strcasecmp(argv[0], "DEBUG") == 0 || strcasecmp(argv[0], "INTERACTIVE") == 0) {
		const char *command;

		if (strcasecmp(argv[0], "DEBUG") == 0) {
			printf("[....] attaching remote debugger");
			command = "DEBUG";
		}
		else {
			printf("[....] attaching interactive session");
			command = "INTERACTIVE";
		}

		fflush(stdout);

		if (!ctl_send_chars(fd, command)) {
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			return COMMAND_FAILED;
		}

		if (ctl_expect_chars(fd, "OK")) {
			struct luadebug_user *readline_user = luadebug_user_readline();
			if (!readline_user) {
				printf(": %ls", clear_error());
				printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			}
			else {
				printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));

				luadebug_user_remote_server(fd, readline_user);
				if (check_error()) {
					message(HAKA_LOG_FATAL, L"debug", clear_error());
					return COMMAND_FAILED;
				}
			}
		}
		else {
			printf(": %sfailed!%s", c(RED, use_colors), c(CLEAR, use_colors));
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		}
	}
	else {
		fprintf(stderr, "invalid command '%s', see help for the list of available command\n", argv[0]);
		return ERROR_INVALID_COMMAND;
	}

	return COMMAND_SUCCESS;
}
