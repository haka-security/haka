
#include "iptables.h"
#include "config.h"

#include <haka/log.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define IPTABLES_SAVE         "/sbin/iptables-save"
#define IPTABLES_RESTORE      "/sbin/iptables-restore"


int apply_iptables(const char *conf)
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

		execl(IPTABLES_RESTORE, "iptables-restore", NULL);

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

int save_iptables(const char *table, char **conf)
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
		char buffer[1024];
		int read_size = 0;

		close(pipefd[1]);

		while (1) {
			status = read(pipefd[0], buffer, sizeof(buffer));
			if (status < 0) {
				error = errno;
				break;
			}

			*conf = realloc(*conf, read_size + status + 1);
			memcpy(*conf + read_size, buffer, status);
			*(*conf + read_size + status) = 0;
			read_size += status;
			if (status < sizeof(buffer)) {
				break;
			}
		}

		close(pipefd[0]);

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
