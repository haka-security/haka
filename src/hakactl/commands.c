/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <haka/colors.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/luadebug/user.h>

#include "commands.h"
#include "ctl_comm.h"


static int check_status(int fd, const char *format, ...)
{
	if (ctl_recv_status(fd) == -1) {
		const char *err = clear_error();
		if (!err) err = "failed!";

		printf(": %s%s%s", c(RED, use_colors), err, c(CLEAR, use_colors));
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}
	else {
		if (format) {
			va_list ap;

			va_start(ap, format);
			vprintf(format, ap);
			va_end(ap);
		}
		printf("\r[ %sok%s ]\n", c(GREEN, use_colors), c(CLEAR, use_colors));
		return COMMAND_SUCCESS;
	}
}


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

	if (!ctl_send_chars(fd, "STOP", -1)) {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}

	return check_status(fd, NULL);
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
	char *module, *msg;

	level = ctl_recv_int(fd);
	if (check_error()) {
		return false;
	}

	module = ctl_recv_chars(fd, NULL);
	if (!module) {
		return false;
	}

	msg = ctl_recv_chars(fd, NULL);
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

	if (!ctl_send_chars(fd, "LOGS", -1)) {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}

	if (check_status(fd, NULL) == COMMAND_SUCCESS) {
		fflush(stdout);

		while (display_log_line(fd));

		return COMMAND_SUCCESS;
	}
	else {
		return COMMAND_FAILED;
	}
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
	printf("[....] changing log level");
	fflush(stdout);

	if ((!ctl_send_chars(fd,"LOGLEVEL", -1)) || (!ctl_send_chars(fd, argv[0], -1))) {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}

	return check_status(fd, ": log level set to %s", argv[0]);
}

struct command command_loglevel = {
	"loglevel",
	"loglevel <level>:   Change haka logging level",
	1,
	run_loglevel
};


/*
 * debug
 */

static int run_remote(int fd, const char *command)
{
	fflush(stdout);

	if (!ctl_send_chars(fd, command, -1)) {
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
		return COMMAND_FAILED;
	}

	struct luadebug_user *readline_user = luadebug_user_readline();
	if (!readline_user) {
		printf(": %s", clear_error());
		printf("\r[%sFAIL%s]\n", c(RED, use_colors), c(CLEAR, use_colors));
	}

	if (check_status(fd, NULL) == COMMAND_SUCCESS) {
		luadebug_user_remote_server(fd, readline_user);
		if (check_error()) {
			messagef(HAKA_LOG_FATAL, "debug", "%s", clear_error());
			return COMMAND_FAILED;
		}
		return COMMAND_SUCCESS;
	}
	else {
		return COMMAND_FAILED;
	}
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
