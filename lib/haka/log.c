
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>
#include <haka/colors.h>
#include <haka/log_module.h>
#include <haka/container/list.h>


static struct logger *loggers = NULL;
static rwlock_t log_module_lock = RWLOCK_INIT;
static local_storage_t local_message_key;
static bool message_is_valid = false;

static bool stdout_enable = true;
static mutex_t stdout_mutex;
static bool stdout_use_colors = 0;
static int stdout_module_size = 0;

#define MODULE_COLOR   CYAN

static const char *level_color[HAKA_LOG_LEVEL_LAST] = {
	BOLD RED,    // LOG_FATAL
	BOLD RED,    // LOG_ERROR
	BOLD YELLOW, // LOG_WARNING
	BOLD,        // LOG_INFO
	CLEAR,       // LOG_DEBUG
};

static const char *message_color[HAKA_LOG_LEVEL_LAST] = {
	CLEAR,       // LOG_FATAL
	CLEAR,       // LOG_ERROR
	CLEAR,       // LOG_WARNING
	CLEAR,       // LOG_INFO
	CLEAR,       // LOG_DEBUG
};

#define MESSAGE_BUFSIZE   2048

static void message_delete(void *value)
{
	free(value);
}

INIT static void _message_init()
{
	UNUSED bool ret;

	ret = local_storage_init(&local_message_key, message_delete);
	assert(ret);

	ret = mutex_init(&stdout_mutex, true);
	assert(ret);

	stdout_use_colors = colors_supported(fileno(stdout));

	message_is_valid = true;
}

FINI static void _message_fini()
{
	remove_all_logger();

	{
		wchar_t *buffer = local_storage_get(&local_message_key);
		if (buffer) {
			message_delete(buffer);
		}
	}

	message_is_valid = false;

	{
		UNUSED bool ret;

		ret = mutex_destroy(&stdout_mutex);
		assert(ret);

		ret = local_storage_destroy(&local_message_key);
		assert(ret);
	}
}

struct message_context_t {
	bool       doing_message;
	wchar_t    buffer[MESSAGE_BUFSIZE];
};

static struct message_context_t *message_context()
{
	struct message_context_t *context = (struct message_context_t *)local_storage_get(&local_message_key);
	if (!context) {
		context = malloc(sizeof(struct message_context_t));
		assert(context);

		context->doing_message = false;

		local_storage_set(&local_message_key, context);
	}
	return context;
}

bool add_logger(struct logger *logger)
{
	assert(logger);

	rwlock_writelock(&log_module_lock);
	list_insert_before(logger, loggers, &loggers, NULL);
	rwlock_unlock(&log_module_lock);

	return true;
}

static void cleanup_logger(struct logger *logger, struct logger **head)
{
	rwlock_writelock(&log_module_lock);
	list_remove(logger, head, NULL);
	rwlock_unlock(&log_module_lock);

	logger->destroy(logger);
}

bool remove_logger(struct logger *logger)
{
	struct logger *module_to_release = NULL;
	struct logger *iter;

	assert(logger);

	{
		rwlock_readlock(&log_module_lock);

		for (iter=loggers; iter; iter = list_next(iter)) {
			if (iter == logger) {
				module_to_release = iter;
				break;
			}
		}

		rwlock_unlock(&log_module_lock);
	}

	if (!iter) {
		error(L"Log module is not registered");
		return false;
	}

	if (module_to_release) {
		cleanup_logger(logger, &loggers);
	}

	return true;
}

void remove_all_logger()
{
	struct logger *logger = NULL;

	{
		rwlock_writelock(&log_module_lock);
		logger = loggers;
		loggers = NULL;
		rwlock_unlock(&log_module_lock);
	}


	while (logger) {
		struct logger *current = logger;
		logger = list_next(logger);

		cleanup_logger(current, NULL);
	}
}

static const char *str_level[HAKA_LOG_LEVEL_LAST] = {
	"fatal",
	"error",
	"warn",
	"info",
	"debug",
};

const char *level_to_str(log_level level)
{
	assert(level >= 0 && level < HAKA_LOG_LEVEL_LAST);
	return str_level[level];
}

log_level str_to_level(const char *str)
{
	int level = 0;
	while ((level < HAKA_LOG_LEVEL_LAST) && (strcmp(str_level[level], str) != 0)) {
		level++;
	}
	return level;
}

void enable_stdout_logging(bool enable)
{
	stdout_enable = enable;
}

bool stdout_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	const char *level_str = level_to_str(lvl);
	const int level_size = strlen(level_str);
	const int module_size = wcslen(module);
	FILE *fd = (lvl == HAKA_LOG_FATAL) ? stderr : stdout;

	thread_setcancelstate(false);

	mutex_lock(&stdout_mutex);

	if (module_size > stdout_module_size) {
		stdout_module_size = module_size;
	}

	if (stdout_use_colors) {
		fprintf(fd, "%s%s" CLEAR "%*s " MODULE_COLOR "%ls:" CLEAR "%*s %s%ls\n" CLEAR, level_color[lvl], level_str,
				level_size-5, "", module, stdout_module_size-module_size, "", message_color[lvl], message);
	}
	else
	{
		fprintf(fd, "%s%*s %ls:%*s %ls\n", level_str, level_size-5, "",
				module, stdout_module_size-module_size, "", message);
	}

	mutex_unlock(&stdout_mutex);

	thread_setcancelstate(true);

	return true;
}

void message(log_level level, const wchar_t *module, const wchar_t *message)
{
	struct message_context_t *context = message_context();
	if (context && !context->doing_message) {
		const log_level max_level = getlevel(module);
		if (level <= max_level) {
			bool remove_pass = false;

			context->doing_message = true;

			struct logger *iter;

			rwlock_readlock(&log_module_lock);
			for (iter=loggers; iter; iter = list_next(iter)) {
				iter->message(iter, level, module, message);

				remove_pass |= iter->mark_for_remove;
			}
			rwlock_unlock(&log_module_lock);

			if (stdout_enable) {
				stdout_message(level, module, message);
			}

			if (remove_pass) {
				rwlock_readlock(&log_module_lock);
				for (iter=loggers; iter; iter = list_next(iter)) {
					if (iter->mark_for_remove) {
						rwlock_unlock(&log_module_lock);
						remove_logger(iter);
						rwlock_readlock(&log_module_lock);
					}
				}
				rwlock_unlock(&log_module_lock);
			}

			context->doing_message = false;
		}
	}
}

void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)
{
	const log_level max_level = getlevel(module);
	if (level <= max_level) {
		struct message_context_t *context = message_context();
		if (context && !context->doing_message) {
			va_list ap;
			va_start(ap, fmt);
			vswprintf(context->buffer, MESSAGE_BUFSIZE, fmt, ap);
			context->buffer[MESSAGE_BUFSIZE-1] = 0;
			message(level, module, context->buffer);
			va_end(ap);
		}
	}
}

struct module_level {
	wchar_t             *module;
	log_level            level;
	struct module_level *next;
};

static struct module_level *module_level = NULL;
static log_level default_level = HAKA_LOG_INFO;
static rwlock_t log_level_lock = RWLOCK_INIT;

static struct module_level *get_module_level(const wchar_t *module, bool create)
{
	struct module_level *iter = module_level, *prev = NULL;
	while (iter) {
		if (wcscmp(module, iter->module) == 0) {
			break;
		}

		prev = iter;
		iter = iter->next;
	}

	if (!iter && create) {
		iter = malloc(sizeof(struct module_level));
		if (!iter) {
			error(L"memory error");
			return NULL;
		}

		iter->module = wcsdup(module);
		if (!iter->module) {
			free(iter);
			error(L"memory error");
			return NULL;
		}

		iter->next = NULL;

		if (prev) prev->next = iter;
		else module_level = iter;
	}

	return iter;
}

void setlevel(log_level level, const wchar_t *module)
{
	rwlock_writelock(&log_level_lock);

	if (!module) {
		default_level = level;
	}
	else {
		struct module_level *module_level = get_module_level(module, true);
		if (module_level) {
			module_level->level = level;
		}
	}

	rwlock_unlock(&log_level_lock);
}

log_level getlevel(const wchar_t *module)
{
	log_level level;

	rwlock_readlock(&log_level_lock);

	level = default_level;
	if (module) {
		struct module_level *module_level = get_module_level(module, false);
		if (module_level) {
			level = module_level->level;
		}
	}

	rwlock_unlock(&log_level_lock);

	return level;
}
