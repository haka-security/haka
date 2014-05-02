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
#include <haka/container/list.h>
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
	struct ctl_server_state *server;
	struct list              list;
	int                      fd;
	bool                     thread_created;
	thread_t                 thread;
	void                   (*callback)(void*);
	void                    *data;
};

struct ctl_server_state {
	int                      fd;
	bool                     thread_created;
	thread_t                 thread;
	bool                     exiting;
	bool                     binded;
	mutex_t                  lock;
	struct ctl_client_state *clients;
};

struct ctl_server_state ctl_server = {0};

static bool ctl_client_process_command(struct ctl_client_state *state, const char *command);

static void ctl_client_cleanup(struct ctl_client_state *state)
{
	struct ctl_server_state *server_state = state->server;

	thread_setcancelstate(false);
	mutex_lock(&server_state->lock);

	if (state->fd >= 0) {
		close(state->fd);
		state->fd = -1;
	}

	list_remove(state, &state->server->clients, NULL);
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

static void ctl_client_process(struct ctl_client_state *state)
{
	char *command = ctl_recv_chars(state->fd);
	if (!command) {
		messagef(HAKA_LOG_ERROR, MODULE, L"cannot read from ctl socket: %ls", clear_error());
		return;
	}

	if (ctl_client_process_command(state, command)) {
		ctl_client_cleanup(state);
	}

	free(command);
}

static void ctl_server_cleanup(struct ctl_server_state *state)
{
	void *ret;

	mutex_lock(&state->lock);

	state->exiting = true;

	if (state->thread_created) {
		thread_cancel(state->thread);

		mutex_unlock(&state->lock);
		thread_join(state->thread, &ret);
		mutex_lock(&state->lock);

		while (state->clients) {
			struct ctl_client_state *current = state->clients;
			if (current->thread_created) {
				const thread_t client_thread = current->thread;
				thread_cancel(client_thread);

				mutex_unlock(&state->lock);
				thread_join(client_thread, &ret);
				mutex_lock(&state->lock);
			}

			if (state->clients == current) {
				ctl_client_cleanup(current);
			}
		}

		state->clients = NULL;

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
	}

	mutex_unlock(&state->lock);

	mutex_destroy(&state->lock);
}

static void *ctl_server_coreloop(void *param)
{
	struct ctl_server_state *state = (struct ctl_server_state *)param;
	struct sockaddr_un addr;
	sigset_t set;

	/* Block all signal to let the main thread handle them */
	sigfillset(&set);
	if (!thread_sigmask(SIG_BLOCK, &set, NULL)) {
		message(HAKA_LOG_FATAL, L"core", clear_error());
		return NULL;
	}

	state->thread_created = true;

	while (!state->exiting) {
		socklen_t len = sizeof(addr);
		int fd;

		thread_testcancel();

		fd = accept(state->fd, (struct sockaddr *)&addr, &len);
		if (fd < 0) {
			messagef(HAKA_LOG_DEBUG, MODULE, L"failed to accept ctl connection: %s", errno_error(errno));
			continue;
		}

		{
			struct ctl_client_state *client = NULL;

			thread_setcancelstate(false);

			client = malloc(sizeof(struct ctl_client_state));
			if (!client) {
				thread_setcancelstate(true);
				message(HAKA_LOG_ERROR, MODULE, L"memmory error");
				continue;
			}

			list_init(client);
			client->server = state;
			client->fd = fd;
			client->thread_created = false;
			list_insert_before(client, state->clients, &state->clients, NULL);

			thread_setcancelstate(true);

			ctl_client_process(client);
		}
	}

	return NULL;
}

bool prepare_ctl_server()
{
	struct sockaddr_un addr;
	socklen_t len;
	int err;

	mutex_init(&ctl_server.lock, true);

	/* Create the socket */
	if ((ctl_server.fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		messagef(HAKA_LOG_FATAL, MODULE, L"cannot create ctl server socket: %s", errno_error(errno));
		return false;
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, HAKA_CTL_SOCKET_FILE);

	len = strlen(addr.sun_path) + sizeof(addr.sun_family);

	err = bind(ctl_server.fd, (struct sockaddr *)&addr, len);
	if (err && errno == EADDRINUSE) {
		if (unlink(HAKA_CTL_SOCKET_FILE)) {
			messagef(HAKA_LOG_FATAL, MODULE, L"cannot remove ctl server socket: %s", errno_error(errno));
			ctl_server_cleanup(&ctl_server);
			return false;
		}

		err = bind(ctl_server.fd, (struct sockaddr *)&addr, len);
	}

	if (err) {
		messagef(HAKA_LOG_FATAL, MODULE, L"cannot bind ctl server socket: %s", errno_error(errno));
		ctl_server_cleanup(&ctl_server);
		return false;
	}

	ctl_server.binded = true;

	return true;
}

bool start_ctl_server()
{
	if (listen(ctl_server.fd, MAX_CLIENT_QUEUE)) {
		messagef(HAKA_LOG_FATAL, MODULE, L"failed to listen on ctl socket: %s", errno_error(errno));
		ctl_server_cleanup(&ctl_server);
		return false;
	}

	/* Start the processing thread */
	if (!thread_create(&ctl_server.thread, ctl_server_coreloop, &ctl_server)) {
		messagef(HAKA_LOG_FATAL, MODULE, clear_error());
		ctl_server_cleanup(&ctl_server);
		return false;
	}

	return true;
}

void stop_ctl_server()
{
	ctl_server_cleanup(&ctl_server);
}


/*
 * Command code
 */

int redirect_message(int fd, log_level level, const wchar_t *module, const wchar_t *message)
{
	if (fd > 0) {
		if (!ctl_send_int(fd, level) ||
			!ctl_send_wchars(fd, module) ||
			!ctl_send_wchars(fd, message)) {
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
	if (!redirect_message(alerter->fd, HAKA_LOG_INFO, L"alert", alert_tostring(id, time, alert, "", "\n\t"))) {
		alerter->alerter.mark_for_remove = true;
	}
	return true;
}

bool redirect_alerter_update(struct alerter *_alerter, uint64 id, const struct time *time, const struct alert *alert)
{
	struct redirect_alerter *alerter = (struct redirect_alerter *)_alerter;
	if (!redirect_message(alerter->fd, HAKA_LOG_INFO, L"alert", alert_tostring(id, time, alert, "update ", "\n\t"))) {
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

static bool ctl_client_process_command(struct ctl_client_state *state, const char *command)
{
	if (strcmp(command, "STATUS") == 0) {
		ctl_send_chars(state->fd, "OK");
	}
	else if (strcmp(command, "STOP") == 0) {
		messagef(HAKA_LOG_INFO, MODULE, L"request to stop haka received");
		ctl_send_chars(state->fd, "OK");
		kill(getpid(), SIGTERM);
	}
	else if (strcmp(command, "LOGS") == 0) {
		struct redirect_logger *logger = redirect_logger_create(state->fd);
		struct redirect_alerter *alerter = redirect_alerter_create(state->fd);
		if (!logger || !alerter) {
			ctl_send_chars(state->fd, "ERROR");
			if (logger) logger->logger.destroy(&logger->logger);
			if (alerter) alerter->alerter.destroy(&alerter->alerter);
			return true;
		}

		if (!add_logger(&logger->logger) ||
		    !add_alerter(&alerter->alerter)) {
			ctl_send_chars(state->fd, "ERROR");
			logger->logger.destroy(&logger->logger);
			alerter->alerter.destroy(&alerter->alerter);
			return true;
		}

		ctl_send_chars(state->fd, "OK");
		logger->fd = state->fd;
		alerter->fd = state->fd;
		state->fd = -1;
	}
	else if (strcmp(command, "LOGLEVEL") == 0) {
		char *level = ctl_recv_chars(state->fd);
		if (check_error()) {
			message(HAKA_LOG_ERROR, MODULE, clear_error());
		}
		else {
			messagef(HAKA_LOG_INFO, MODULE, L"setting log level to %s", level);
			setup_loglevel(level);
			ctl_send_chars(state->fd, "OK");
		}
	}
	else if (strcmp(command, "STATS") == 0) {
		FILE *file;
		file = fdopen(state->fd, "a+");
		if (!file) {
			messagef(HAKA_LOG_ERROR, MODULE, L"cannot open socket file: %ls", errno_error(errno));
			ctl_send_chars(state->fd, "ERROR");
		}
		else {
			ctl_send_chars(state->fd, "OK");
			stat_printf(file, "\n");
			dump_stat(file);
			fclose(file);
			state->fd = -1;
		}
	}
	else if (strcmp(command, "DEBUG") == 0) {
		struct luadebug_user *remote_user = luadebug_user_remote(state->fd);
		if (!remote_user) {
			ctl_send_chars(state->fd, "ERROR");
			return true;
		}

		ctl_send_chars(state->fd, "OK");
		state->fd = -1;

		luadebug_debugger_user(remote_user);
		luadebug_user_release(&remote_user);

		thread_pool_attachdebugger(get_thread_pool());
	}
	else if (strcmp(command, "INTERACTIVE") == 0) {
		struct luadebug_user *remote_user = luadebug_user_remote(state->fd);
		if (!remote_user) {
			ctl_send_chars(state->fd, "ERROR");
			return true;
		}

		ctl_send_chars(state->fd, "OK");
		state->fd = -1;

		luadebug_interactive_user(remote_user);
		luadebug_user_release(&remote_user);
	}
	else if (strlen(command) > 0) {
		messagef(HAKA_LOG_ERROR, MODULE, L"invalid ctl command '%s'", command);
		ctl_send_chars(state->fd, "ERROR");
	}

	return true;
}
