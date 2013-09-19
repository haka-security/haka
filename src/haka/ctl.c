
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
#include <haka/container/list.h>

#include "ctl.h"
#include "config.h"


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

	mutex_lock(&server_state->lock);

	if (state->fd > 0) {
		close(state->fd);
		state->fd = -1;
	}

	list_remove(state, &state->server->clients, NULL);
	free(state);

	mutex_unlock(&server_state->lock);
}

static bool ctl_send(int fd, const char *message)
{
	if (send(fd, message, strlen(message)+1, 0) < 0) {
		messagef(HAKA_LOG_ERROR, MODULE, L"cannot write to ctl socket: %s", errno_error(errno));
		return false;
	}
	return true;
}

static bool ctl_wsend(int fd, const wchar_t *message)
{
	if (send(fd, message, (wcslen(message)+1)*sizeof(wchar_t), 0) < 0) {
		messagef(HAKA_LOG_ERROR, MODULE, L"cannot write to ctl socket: %s", errno_error(errno));
		return false;
	}
	return true;
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
		messagef(HAKA_LOG_DEBUG, MODULE, L"failed to create thread: %s", clear_error(errno));
		return false;
	}

	return true;
}

static void ctl_client_process(struct ctl_client_state *state)
{
	char buffer[MAX_COMMAND_LEN];
	int len;

	bzero(buffer, MAX_COMMAND_LEN);
	len = recv(state->fd, buffer, MAX_COMMAND_LEN, 0);
	if (len <= 0) {
		if (len < 0)
			messagef(HAKA_LOG_ERROR, MODULE, L"cannot read from ctl socket: %s", errno_error(errno));
		return;
	}
	buffer[MAX_COMMAND_LEN-1] = 0;
	if (ctl_client_process_command(state, buffer)) {
		ctl_client_cleanup(state);
	}
}

static void ctl_server_cleanup(struct ctl_server_state *state)
{
	void *ret;

	mutex_lock(&state->lock);

	state->exiting = true;
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

	if (state->fd > 0) {
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
	if (bind(ctl_server.fd, (struct sockaddr *)&addr, len)) {
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

struct redirect_logger {
	struct logger    logger;
	int              fd;
};

int redirect_logger_message(struct logger *_logger, log_level level, const wchar_t *module, const wchar_t *message)
{
	struct redirect_logger *logger = (struct redirect_logger *)_logger;

	if (logger->fd > 0) {
		if (send(logger->fd, &level, sizeof(log_level), 0) < 0 ||
			!ctl_wsend(logger->fd, module) ||
			!ctl_wsend(logger->fd, message)) {
			logger->logger.mark_for_remove = true;
		}
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

static bool ctl_client_process_command(struct ctl_client_state *state, const char *command)
{
	if (strcmp(command, "STATUS") == 0) {
		ctl_send(state->fd, "OK");
	}
	else if (strcmp(command, "STOP") == 0) {
		messagef(HAKA_LOG_INFO, MODULE, L"request to stop haka received");
		kill(getpid(), SIGTERM);
		ctl_send(state->fd, "OK");
	}
	else if (strcmp(command, "LOGS") == 0) {
		struct redirect_logger *logger = redirect_logger_create(state->fd);
		if (!logger) {
			ctl_send(state->fd, "ERROR");
			return true;
		}

		if (!add_logger(&logger->logger)) {
			ctl_send(state->fd, "ERROR");
			logger->logger.destroy(&logger->logger);
			return true;
		}

		ctl_send(state->fd, "OK");
		logger->fd = state->fd;
		state->fd = -1;
	}
	else if (strcmp(command, "LOGLEVEL") == 0) {
			const char *ptr = command + 9;
			if (*ptr != '\0') {
				messagef(HAKA_LOG_INFO, MODULE, L"setting log level to %s", ptr);
				log_level level = str_to_level(ptr);
				setlevel(level, NULL);
				ctl_send(state->fd, "OK");
			}
		}

	else if (strlen(command) > 0) {
		messagef(HAKA_LOG_ERROR, MODULE, L"invalid ctl command '%s'", command);
		ctl_send(state->fd, "ERROR");
	}

	return true;
}
