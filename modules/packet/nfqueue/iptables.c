/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iptables.h"
#include "config.h"

#include <haka/log.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define IPTABLES_SAVE         "/sbin/iptables-save"
#define IPTABLES_RESTORE      "/sbin/iptables-restore"


int apply_iptables(const char *table, const char *conf, bool noflush)
{
	int pipefd[2];
	pid_t child_pid;
	int error = 0;

	if (pipe(pipefd) < 0) {
		return errno;
	}

	child_pid = fork();
	if (child_pid < 0) {
		error = errno;
		close(pipefd[0]);
		close(pipefd[1]);
		return error;
	}

	if (child_pid == 0) {
		/* child */
		close(pipefd[1]);

		if (dup2(pipefd[0], STDIN_FILENO) == -1) {
			exit(1);
		}
		close(pipefd[0]);

		if (noflush) {
			execl(IPTABLES_RESTORE, "iptables-restore", "-T", table, "-n", NULL);
		}
		else {
			execl(IPTABLES_RESTORE, "iptables-restore", "-T", table, NULL);
		}

		/* an error occured */
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot execute 'iptables-restore'");
		exit(1);
	}
	else {
		/* parent */
		int status;
		pid_t ret;

		close(pipefd[0]);

		status = write(pipefd[1], conf, strlen(conf));
		error = errno;
		close(pipefd[1]);

		if (status < 0) {
			return error;
		}

		/* wait for the child to finish */
		do {
			ret = waitpid(child_pid, &status, 0);
			if (ret == -1) {
				return errno;
			}
		}
		while (!WIFEXITED(status) && !WIFSIGNALED(status));

		return WEXITSTATUS(status);
	}
}

static bool check_haka_target(const char *str, const char *name)
{
	const size_t len = strlen(name);

	if (strncmp(str, name, len) == 0) {
		if (str[len] == ' ' ||
			str[len] == '\0' ||
			str[len] == '\n') {
			return true;
		}
	}

	return false;
}

int save_iptables(const char *table, char **conf, bool all_targets)
{
	int pipefd[2];
	pid_t child_pid;
	int error = 0;

	assert(conf);

	if (pipe(pipefd) < 0) {
		return errno;
	}

	child_pid = fork();
	if (child_pid < 0) {
		error = errno;
		close(pipefd[0]);
		close(pipefd[1]);
		return error;
	}

	if (child_pid == 0) {
		/* child */
		close(pipefd[0]);

		if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
			exit(1);
		}
		close(pipefd[1]);

		execl(IPTABLES_SAVE, "iptables-save", "-t", table, NULL);

		/* an error occured */
		message(HAKA_LOG_ERROR, MODULE_NAME, L"cannot execute 'iptables-save'");
		exit(1);
	}
	else {
		/* parent */
		int status;
		pid_t ret;
		char *buffer = NULL;
		size_t buffer_size = 0, read_size = 0;
		ssize_t line_size;
		FILE *input = fdopen(pipefd[0], "r");
		bool filtering = false;

		if (!input) {
			return errno;
		}

		close(pipefd[1]);

		while ((line_size = getline(&buffer, &buffer_size, input)) >= 0) {
			if (!all_targets && filtering) {
				/*
				 * If all_targets is true, we only want to include the rule in the
				 * targets HAKA_TARGET_PRE and HAKA_TARGET_OUT. So we need to filter
				 * each line to only include those but being careful not to add the
				 * line from another target that jump to one of the haka target
				 * (ie. -A PREROUTING -j haka-pre)
				 */
				char *pattern, *current = buffer;
				bool skip = true;

				while (true) {
					if ((pattern = strstr(current, "-j " HAKA_TARGET))) {
						pattern += 3; /* Skip '-j ' */

						if (check_haka_target(pattern, HAKA_TARGET_PRE) ||
							check_haka_target(pattern, HAKA_TARGET_OUT)) {
							break;
						}

						current = pattern+1;
					}
					else if ((pattern = strstr(current, HAKA_TARGET))) {
						if (check_haka_target(pattern, HAKA_TARGET_PRE) ||
							check_haka_target(pattern, HAKA_TARGET_OUT)) {
							skip = false;
							break;
						}

						current = pattern+1;
					}
					else if (strncmp(current, "COMMIT", strlen("COMMIT")) == 0) {
						skip = false;
						filtering = false;
						break;
					}
					else {
						break;
					}
				}

				if (skip) {
					continue;
				}
			}

			if (buffer[0] == '*') {
				filtering = true;
			}

			*conf = realloc(*conf, read_size + line_size + 1);
			memcpy(*conf + read_size, buffer, line_size + 1);
			read_size += line_size;
		}

		free(buffer);
		fclose(input);

		if (error) {
			free(*conf);
			*conf = NULL;
			return error;
		}

		/* wait for the child to finish */
		do {
			ret = waitpid(child_pid, &status, 0);
			if (ret == -1) {
				free(*conf);
				*conf = NULL;
				return errno;
			}
		}
		while (!WIFEXITED(status) && !WIFSIGNALED(status));

		if (WEXITSTATUS(status) != 0) {
			free(*conf);
			*conf = NULL;
			return 1;
		}

		return 0;
	}
}
