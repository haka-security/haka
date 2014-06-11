/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <haka/error.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/stat.h>
#include <haka/alert.h>
#include <haka/engine.h>
#include <haka/container/list2.h>
#include <haka/luadebug/user.h>
#include <haka/luadebug/debugger.h>
#include <haka/luadebug/interactive.h>

#include "ctl.h"
#include "app.h"
#include "thread.h"
#include "config.h"
#include "ctl_comm.h"


#define MODULE             L"ctl"
#define MAX_CLIENT_QUEUE   10
#define MAX_COMMAND_LEN    1024

struct ctl_server_state;

struct ctl_client_state {
	struct list2_elem        list;
	struct ctl_server_state *server;
	int                      fd;
	bool                     thread_created;
	thread_t                 thread;
	void                   (*callback)(void*);
	void                    *data;
};

struct ctl_server_state {
	int                      fd;
	thread_t                 thread;
	bool                     thread_created:1;
	bool                     binded:1;
	bool                     created:1;
	volatile bool            exiting;
	mutex_t                  lock;
	struct list2             clients;
};

struct ctl_server_state ctl_server = {0};

enum clt_client_rc {
	CTL_CLIENT_OK,
	CTL_CLIENT_DONE,
	CTL_CLIENT_DUP,
	CTL_CLIENT_THREAD,
};

static enum clt_client_rc ctl_client_process_command(struct ctl_client_state *state, const char *command);

static void ctl_client_cleanup(struct ctl_client_state *state)
{
	struct ctl_server_state *server_state = state->server;

	thread_setcancelstate(false);
	mutex_lock(&server_state->lock);

	if (state->fd >= 0) {
		close(state->fd);
		state->fd = -1;
	}

	if (list2_elem_check(&state->list)) {
		list2_erase(&state->list);
	}

	free(state);

	mutex_unlock(&server_state->lock);
	thread_setcancelstate(true);
}

static void *ctl_client_process_thread(void *param)
{
	struct ctl_client_state *state = (struct ctl_client_state *)param;
	sigset_t set;

	/* Block all signal to let the main thread handle them */
	sigfillset(&set);
	if (!thread_sigmask(SIG_BLOCK, &set, NULL)) {
		message(HAKA_LOG_ERROR, MODULE, clear_error());
		ctl_client_cleanup(state);
		return NULL;
	}

	if (!thread_setcanceltype(THREAD_CANCEL_ASYNCHRONOUS)) {
		message(HAKA_LOG_ERROR, MODULE, clear_error());
		ctl_client_cleanup(state);
		return NULL;
	}

	state->thread_created = true;

	state->callback(state->data);

	ctl_client_cleanup(state);
	return NULL;
}

UNUSED static bool ctl_start_client_thread(struct ctl_client_state *state, void (*func)(void*), void *data)
{
	state->callback = func;
	state->data = data;

	if (!thread_create(&state->thread, ctl_client_process_thread, state)) {
		messagef(HAKA_LOG_DEBUG, MODULE, L"failed to create thread: %ls", clear_error(errno));
		return false;
	}

	return true;
}

static enum clt_client_rc ctl_client_process(struct ctl_client_state *state)
{
	enum clt_client_rc rc;
	char *command = ctl_recv_chars(state->fd, NULL);

	if (!command) {
		const wchar_t *error = clear_error();
		if (wcscmp(error, L"end of file") != 0) {
			messagef(HAKA_LOG_ERROR, MODULE, L"cannot read from ctl socket: %ls", error);
		}
		return CTL_CLIENT_DONE;
	}

	rc = ctl_client_process_command(state, command);
	free(command);

	return rc;
}

static void ctl_server_cleanup(struct ctl_server_state *state, bool cancel_thread)
{
	if (state && state->created) {
		void *ret;

		mutex_lock(&state->lock);

		state->created = false;
		state->exiting = true;

		if (state->thread_created) {
			list2_iter iter, end;

			if (cancel_thread) {
				thread_cancel(state->thread);

				mutex_unlock(&state->lock);
				thread_join(state->thread, &ret);
				mutex_lock(&state->lock);
			}

			iter = list2_begin(&state->clients);
			end = list2_end(&state->clients);
			while (iter != end) {
				struct ctl_client_state *client = list2_get(iter, struct ctl_client_state, list);
				if (client->thread_created) {
					const thread_t client_thread = client->thread;
					thread_cancel(client_thread);

					mutex_unlock(&state->lock);
					thread_join(client_thread, &ret);
					mutex_lock(&state->lock);

					iter = list2_next(iter);
				}
				else {
					iter = list2_erase(iter);
					ctl_client_cleanup(client);
				}
			}
		}

		if (state->fd >= 0) {
			close(state->fd);
			state->fd = -1;
		}

		if (state->binded) {
			if (remove(HAKA_CTL_SOCKET_FILE)) {
				messagef(HAKA_LOG_ERROR, MODULE, L"cannot remove socket file: %s", errno_error(errno));
			}

			state->binded = false;
		}

		mutex_unlock(&state->lock);

		mutex_destroy(&state->lock);
	}
}

static bool ctl_server_init(struct ctl_server_state *state)
{
	struct sockaddr_un addr;
	socklen_t len;
	int err;

	mutex_init(&state->lock, true);

	list2_init(&state->clients);

	/* Create the socket */
	if ((state->fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		messagef(HAKA_LOG_FATAL, MODULE, L"cannot create ctl server socket: %s", errno_error(errno));
		return false;
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, HAKA_CTL_SOCKET_FILE);

	len = strlen(addr.sun_path) + sizeof(addr.sun_family);

	err = bind(state->fd, (struct sockaddr *)&addr, len);
	if (err && errno == EADDRINUSE) {
		if (unlink(HAKA_CTL_SOCKET_FILE)) {
			messagef(HAKA_LOG_FATAL, MODULE, L"cannot remove ctl server socket: %s", errno_error(errno));
			ctl_server_cleanup(&ctl_server, true);
			return false;
		}

		err = bind(state->fd, (struct sockaddr *)&addr, len);
	}

	if (err) {
		messagef(HAKA_LOG_FATAL, MODULE, L"cannot bind ctl server socket: %s", errno_error(errno));
		ctl_server_cleanup(&ctl_server, true);
		return false;
	}

	state->binded = true;
	state->created = true;

	return true;
}

static void *ctl_server_coreloop(void *param)
{
	struct ctl_server_state *state = (struct ctl_server_state *)param;
	struct sockaddr_un addr;
	sigset_t set;
	fd_set listfds, readfds;
	int maxfd, rc;

	/* Block all signal to let the main thread handle them */
	sigfillset(&set);
	if (!thread_sigmask(SIG_BLOCK, &set, NULL)) {
		message(HAKA_LOG_FATAL, L"core", clear_error());
		return NULL;
	}

	state->thread_created = true;

	FD_ZERO(&listfds);
	FD_SET(state->fd, &listfds);
	maxfd = state->fd;

	while (!state->exiting) {
		readfds = listfds;
		rc = select(maxfd+1, &readfds, NULL, NULL, NULL);

		if (rc < 0) {
			messagef(HAKA_LOG_FATAL, MODULE, L"failed to handle ctl connection (closing ctl socket): %s", errno_error(errno));
			break;
		}
		else if (rc > 0) {
			if (FD_ISSET(state->fd, &readfds)) {
				socklen_t len = sizeof(addr);
				int fd;
				struct ctl_client_state *client = NULL;

				thread_testcancel();

				fd = accept(state->fd, (struct sockaddr *)&addr, &len);
				if (fd < 0) {
					messagef(HAKA_LOG_DEBUG, MODULE, L"failed to accept ctl connection: %s", errno_error(errno));
					continue;
				}

				thread_setcancelstate(false);

				client = malloc(sizeof(struct ctl_client_state));
				if (!client) {
					thread_setcancelstate(true);
					message(HAKA_LOG_ERROR, MODULE, L"memmory error");
					continue;
				}

				list2_elem_init(&client->list);
				client->server = state;
				client->fd = fd;
				client->thread_created = false;

				mutex_lock(&state->lock);
				list2_insert(list2_end(&state->clients), &client->list);
				mutex_unlock(&state->lock);

				thread_setcancelstate(true);

				FD_SET(client->fd, &listfds);
				if (client->fd > maxfd) maxfd = client->fd;

				rc--;
			}

			if (rc > 0) {
				list2_iter iter = list2_begin(&state->clients);
				list2_iter end = list2_end(&state->clients);
				while (iter != end) {
					struct ctl_client_state *client = list2_get(iter, struct ctl_client_state, list);

					if (FD_ISSET(client->fd, &readfds)) {
						enum clt_client_rc rc = ctl_client_process(client);

						if (rc == CTL_CLIENT_OK) {
							iter = list2_next(iter);
						}
						else {
							FD_CLR(client->fd, &listfds);

							if (rc == CTL_CLIENT_DUP) client->fd = -1;

							if (rc != CTL_CLIENT_THREAD) {
								iter = list2_erase(iter);
								ctl_client_cleanup(client);
							}
							else {
								iter = list2_next(iter);
							}
						}
					}
					else {
						iter = list2_next(iter);
					}
				}
			}
		}
	}

	ctl_server_cleanup(state, false);

	return NULL;
}

bool prepare_ctl_server()
{
	return ctl_server_init(&ctl_server);
}

bool start_ctl_server()
{
	if (listen(ctl_server.fd, MAX_CLIENT_QUEUE)) {
		messagef(HAKA_LOG_FATAL, MODULE, L"failed to listen on ctl socket: %s", errno_error(errno));
		ctl_server_cleanup(&ctl_server, true);
		return false;
	}

	/* Start the processing thread */
	if (!thread_create(&ctl_server.thread, ctl_server_coreloop, &ctl_server)) {
		messagef(HAKA_LOG_FATAL, MODULE, clear_error());
		ctl_server_cleanup(&ctl_server, true);
		return false;
	}

	return true;
}

void stop_ctl_server()
{
	ctl_server_cleanup(&ctl_server, true);
}


/*
 * Command code
 */

int redirect_message(int fd, log_level level, const wchar_t *module, const wchar_t *message)
{
	if (fd > 0) {
		if (!ctl_send_int(fd, level) ||
			!ctl_send_wchars(fd, module, -1) ||
			!ctl_send_wchars(fd, message, -1)) {
			clear_error();
			return false;
		}
	}
	return true;
}

struct redirect_logger {
	struct logger    logger;
	int              fd;
};

int redirect_logger_message(struct logger *_logger, log_level level, const wchar_t *module, const wchar_t *message)
{
	struct redirect_logger *logger = (struct redirect_logger *)_logger;
	if (!redirect_message(logger->fd, level, module, message)) {
		logger->logger.mark_for_remove = true;
	}
	return 1;
}

void redirect_logger_destroy(struct logger *_logger)
{
	struct redirect_logger *logger = (struct redirect_logger *)_logger;
	if (logger->fd > 0) {
		close(logger->fd);
	}
	free(logger);
}

struct redirect_logger *redirect_logger_create(d)
{
	struct redirect_logger *logger = malloc(sizeof(struct redirect_logger));
	if (!logger) {
		error(L"memory error");
		return NULL;
	}

	list_init(&logger->logger);
	logger->logger.message = redirect_logger_message;
	logger->logger.destroy = redirect_logger_destroy;
	logger->logger.mark_for_remove = false;
	logger->fd = -1;

	return logger;
}

struct redirect_alerter {
	struct alerter   alerter;
	int              fd;
};

bool redirect_alerter_alert(struct alerter *_alerter, uint64 id, const struct time *time, const struct alert *alert)
{
	struct redirect_alerter *alerter = (struct redirect_alerter *)_alerter;
	if (!redirect_message(alerter->fd, HAKA_LOG_INFO, L"alert", alert_tostring(id, time, alert, "", "\n\t", false))) {
		alerter->alerter.mark_for_remove = true;
	}
	return true;
}

bool redirect_alerter_update(struct alerter *_alerter, uint64 id, const struct time *time, const struct alert *alert)
{
	struct redirect_alerter *alerter = (struct redirect_alerter *)_alerter;
	if (!redirect_message(alerter->fd, HAKA_LOG_INFO, L"alert", alert_tostring(id, time, alert, "update ", "\n\t", false))) {
		alerter->alerter.mark_for_remove = true;
	}
	return true;
}

void redirect_alerter_destroy(struct alerter *_alerter)
{
	struct redirect_alerter *alerter = (struct redirect_alerter *)_alerter;
	if (alerter->fd > 0) {
		close(alerter->fd);
	}
	free(alerter);
}

struct redirect_alerter *redirect_alerter_create(d)
{
	struct redirect_alerter *alerter = malloc(sizeof(struct redirect_alerter));
	if (!alerter) {
		error(L"memory error");
		return NULL;
	}

	list_init(&alerter->alerter);
	alerter->alerter.alert = redirect_alerter_alert;
	alerter->alerter.update = redirect_alerter_update;
	alerter->alerter.destroy = redirect_alerter_destroy;
	alerter->alerter.mark_for_remove = false;
	alerter->fd = -1;

	return alerter;
}

static enum clt_client_rc ctl_client_process_command(struct ctl_client_state *state, const char *command)
{
	if (strcmp(command, "STATUS") == 0) {
		ctl_send_status(state->fd, 0, NULL);
		return CTL_CLIENT_OK;
	}
	else if (strcmp(command, "STOP") == 0) {
		messagef(HAKA_LOG_INFO, MODULE, L"request to stop haka received");
		ctl_send_status(state->fd, 0, NULL);
		kill(getpid(), SIGTERM);
		return CTL_CLIENT_DONE;
	}
	else if (strcmp(command, "LOGS") == 0) {
		struct redirect_logger *logger = redirect_logger_create(state->fd);
		struct redirect_alerter *alerter = redirect_alerter_create(state->fd);
		if (!logger || !alerter) {
			ctl_send_status(state->fd, -1, clear_error());
			if (logger) logger->logger.destroy(&logger->logger);
			if (alerter) alerter->alerter.destroy(&alerter->alerter);
			return CTL_CLIENT_OK;
		}

		if (!add_logger(&logger->logger) ||
		    !add_alerter(&alerter->alerter)) {
			ctl_send_status(state->fd, -1, clear_error());
			logger->logger.destroy(&logger->logger);
			alerter->alerter.destroy(&alerter->alerter);
			return CTL_CLIENT_OK;
		}

		ctl_send_status(state->fd, 0, NULL);
		logger->fd = state->fd;
		alerter->fd = state->fd;
		return CTL_CLIENT_DUP;
	}
	else if (strcmp(command, "LOGLEVEL") == 0) {
		char *level = ctl_recv_chars(state->fd, NULL);
		if (check_error()) {
			message(HAKA_LOG_ERROR, MODULE, clear_error());
			return CTL_CLIENT_DONE;
		}

		messagef(HAKA_LOG_INFO, MODULE, L"setting log level to %s", level);

		if (!setup_loglevel(level)) {
			const wchar_t *err = clear_error();
			messagef(HAKA_LOG_ERROR, MODULE, err);
			ctl_send_status(state->fd, -1, err);
		}
		else {
			ctl_send_status(state->fd, 0, NULL);
		}
		return CTL_CLIENT_OK;
	}
	else if (strcmp(command, "STATS") == 0) {
		FILE *file;
		file = fdopen(state->fd, "a+");
		if (!file) {
			messagef(HAKA_LOG_ERROR, MODULE, L"cannot open socket file: %ls", errno_error(errno));
			ctl_send_status(state->fd, -1, L"cannot open socket file");
			return CTL_CLIENT_OK;
		}
		else {
			ctl_send_status(state->fd, 0, NULL);

			stat_printf(file, "\n");
			dump_stat(file);
			fclose(file);
			return CTL_CLIENT_DUP;
		}
	}
	else if (strcmp(command, "DEBUG") == 0) {
		struct luadebug_user *remote_user = luadebug_user_remote(state->fd);
		if (!remote_user) {
			ctl_send_status(state->fd, -1, clear_error());
			return CTL_CLIENT_OK;
		}

		ctl_send_status(state->fd, 0, NULL);

		luadebug_debugger_user(remote_user);
		luadebug_user_release(&remote_user);

		thread_pool_attachdebugger(get_thread_pool());
		return CTL_CLIENT_DUP;
	}
	else if (strcmp(command, "INTERACTIVE") == 0) {
		struct luadebug_user *remote_user = luadebug_user_remote(state->fd);
		if (!remote_user) {
			ctl_send_status(state->fd, -1, clear_error());
			return CTL_CLIENT_OK;
		}

		ctl_send_status(state->fd, 0, NULL);

		luadebug_interactive_user(remote_user);
		luadebug_user_release(&remote_user);
		return CTL_CLIENT_DUP;
	}
	else if (strcmp(command, "EXECUTE") == 0) {
		int thread = ctl_recv_int(state->fd);
		if (check_error()) {
			return CTL_CLIENT_DONE;
		}

		size_t len;
		char *code = ctl_recv_chars(state->fd, &len);
		if (!code) {
			return CTL_CLIENT_DONE;
		}

		/* Run on the first thread when the thread id is 'any' */
		if (thread == -2) thread = 0;

		if (thread == -1) {
			int i;
			struct engine_thread *engine_thread;
			for (i=0;; ++i) {
				char *result;
				size_t size = len;

				engine_thread = engine_thread_byid(i);
				if (!engine_thread) break;

				result = engine_thread_raw_lua_remote_launch(engine_thread, code, &size);
				if (check_error()) {
					ctl_send_status(state->fd, -1, clear_error());
					free(code);
					return CTL_CLIENT_OK;
				}

				if (result) {
					ctl_send_status(state->fd, 1, NULL);
					ctl_send_chars(state->fd, result, size);
					free(result);
				}
			}

			ctl_send_status(state->fd, 0, NULL);
		}
		else {
			struct engine_thread *engine_thread = engine_thread_byid(thread);
			if (!engine_thread) {
				ctl_send_status(state->fd, -1, clear_error());
				free(code);
				return CTL_CLIENT_OK;
			}

			char *result = engine_thread_raw_lua_remote_launch(engine_thread, code, &len);
			if (check_error()) {
				ctl_send_status(state->fd, -1, clear_error());
				free(code);
				return CTL_CLIENT_OK;
			}

			if (result) {
				ctl_send_status(state->fd, 1, NULL);
				ctl_send_chars(state->fd, result, len);
				free(result);
			}

			ctl_send_status(state->fd, 0, NULL);
		}

		free(code);
		return CTL_CLIENT_OK;
	}
	else {
		if (strlen(command) > 0) {
			messagef(HAKA_LOG_ERROR, MODULE, L"invalid ctl command '%s'", command);
			ctl_send_status(state->fd, -1, NULL);
		}
		return CTL_CLIENT_DONE;
	}
}
