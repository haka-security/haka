/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMMANDS_H
#define COMMANDS_H

enum {
	COMMAND_SUCCESS       = 0,
	COMMAND_FAILED        = 1,
};

struct command {
	char  *name;
	char  *help;
	int    nb_args;
	int  (*run)(int fd, int argc, char *argv[]);
};

extern struct command command_status;
extern struct command command_stop;
extern struct command command_logs;
extern struct command command_loglevel;
extern struct command command_debug;
extern struct command command_interactive;
extern struct command command_console;

extern bool use_colors;

#endif /* COMMANDS_H */
