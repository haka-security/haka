
#include <stdio.h>
#include <stdlib.h>

#include <haka/colors.h>
#include <haka/log.h>
#include <haka/error.h>
#include <luadebug/user.h>

#include "commands.h"
#include "ctl_comm.h"


/*
 * status
 */

static int run_status(int fd, int argc, char *argv[])
{
	return COMMAND_SUCCESS;
}

struct command command_status = {
	"status",
	"status:             Show haka status",
	0,
	run_status
};

/*
 * stop
 */

static int run_stop(int fd, int argc, char *argv[])
{
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

	return COMMAND_SUCCESS;
}

struct command command_stop = {
	"stop",
	"stop:               Stop haka",
	0,
	run_stop
};


/*
 * logs
 */

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

	/* Force a fflush in case with are redirected to a file */
	fflush(stdout);

	free(module);
	free(msg);

	return true;
}

static int run_logs(int fd, int argc, char *argv[])
{
	printf("[....] requesting logs");
	fflush(stdout);

	if (!ctl_send_chars(fd, "LOGS")) {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}

	if (ctl_expect_chars(fd, "OK")) {
		printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		fflush(stdout);

		while (display_log_line(fd));
	}
	else {
		printf(": %sfailed!%s", c(RED, use_colors), c(CLEAR, use_colors));
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
	}

	return COMMAND_SUCCESS;
}

struct command command_logs = {
	"logs",
	"logs:               Show haka logs in realtime",
	0,
	run_logs
};


/*
 * log level
 */

static int run_loglevel(int fd, int argc, char *argv[])
{
	log_level level;

	printf("[....] changing log level");
	fflush(stdout);

	level = str_to_level(argv[0]);
	if (level != HAKA_LOG_LEVEL_LAST) {
		if ((!ctl_send_chars(fd,"LOGLEVEL")) || (!ctl_send_int(fd, level))) {
			printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
			return COMMAND_FAILED;
		}
		if (ctl_expect_chars(fd, "OK")) {
			printf(": log level set to %s", argv[0]);
			printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		}
	}
	else {
		int index;

		printf(": invalid log level '%s'", argv[0]);
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));

		printf("       possible log levels are ");
		for (index = 0; index < HAKA_LOG_LEVEL_LAST; index++) {
			if (index != 0) printf(", ");
			printf("%s", level_to_str(index));
		}
		printf("\n");

		return COMMAND_FAILED;
	}

	return COMMAND_SUCCESS;
}

struct command command_loglevel = {
	"loglevel",
	"loglevel <level>:   Change haka logging level",
	1,
	run_loglevel
};

/*
 * stats
 */

static int run_stats(int fd, int argc, char *argv[])
{
	printf("[....] requesting statistics");
	fflush(stdout);

	if (!ctl_send_chars(fd, "STATS")) {
		 printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		 return COMMAND_FAILED;
	}

	if (ctl_expect_chars(fd, "OK")) {
		printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		ctl_output_redirect_chars(fd);
		return COMMAND_SUCCESS;
	}
	else {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}
}

struct command command_stats = {
	"stats",
	"stats:               show statistics",
	0,
	run_stats
};

/*
 * debug
 */

static int run_remote(int fd, const char *command)
{
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

	return COMMAND_SUCCESS;
}

static int run_debug(int fd, int argc, char *argv[])
{
	printf("[....] attaching remote debugger");
	return run_remote(fd, "DEBUG");
}

struct command command_debug = {
	"debug",
	"debug:              Attach a remote lua debugger to haka",
	0,
	run_debug
};


/*
 * interactive
 */

static int run_interactive(int fd, int argc, char *argv[])
{
	printf("[....] attaching interactive session");
	return run_remote(fd, "INTERACTIVE");
}

struct command command_interactive = {
	"interactive",
	"interactive:        Attach an interactive session to haka",
	0,
	run_interactive
};
