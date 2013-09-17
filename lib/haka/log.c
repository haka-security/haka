
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


struct installed_module {
	struct list              list;
	struct log_module       *module;
	struct log_module_state *state;
};

static struct installed_module *log_modules = NULL;
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
	bool ret;

	ret = local_storage_init(&local_message_key, message_delete);
	assert(ret);

	ret = mutex_init(&stdout_mutex, true);
	assert(ret);

	stdout_use_colors = colors_supported(fileno(stdout));

	message_is_valid = true;
}

FINI static void _message_fini()
{
	remove_all_log_modules();

	{
		wchar_t *buffer = local_storage_get(&local_message_key);
		if (buffer) {
			message_delete(buffer);
		}
	}

	message_is_valid = false;

	{
		bool ret;

		ret = mutex_destroy(&stdout_mutex);
		assert(ret);

		ret = local_storage_destroy(&local_message_key);
		assert(ret);
	}
}

static wchar_t *message_context()
{
	wchar_t *buffer = (wchar_t *)local_storage_get(&local_message_key);
	if (!buffer) {
		buffer = malloc(sizeof(wchar_t) * MESSAGE_BUFSIZE);
		assert(buffer);

		local_storage_set(&local_message_key, buffer);
	}
	return buffer;
}

bool add_log_module(struct log_module *module, struct log_module_state *state)
{
	struct installed_module *log_module;

	assert(module);

	log_module = malloc(sizeof(struct installed_module));
	if (!log_module) {
		error(L"memory error");
		return false;
	}

	log_module->module = module;
	module_addref(&module->module);
	log_module->state = state;

	rwlock_writelock(&log_module_lock);
	list_insert_before(log_module, log_modules, &log_modules, NULL);
	rwlock_unlock(&log_module_lock);

	return true;
}

static void cleanup_log_module(struct installed_module *log_module, struct installed_module **head)
{
	rwlock_writelock(&log_module_lock);
	list_remove(log_module, head, NULL);
	rwlock_unlock(&log_module_lock);

	log_module->module->cleanup_state(log_module->state);
	module_release(&log_module->module->module);
	free(log_module);
}

bool remove_log_module(struct log_module *module, struct log_module_state *state)
{
	struct installed_module *module_to_release = NULL;
	struct installed_module *iter = log_modules;

	assert(module);

	{
		rwlock_readlock(&log_module_lock);

		for (iter=log_modules; iter; iter = list_next(iter)) {
			if (iter->module == module &&
				iter->state == state) {
				module_to_release = iter;
				break;
			}
		}

		rwlock_unlock(&log_module_lock);
	}

	if (!iter) {
		error(L"Log modules is not registered");
		return false;
	}

	if (module_to_release) {
		cleanup_log_module(module_to_release, &log_modules);
	}

	return true;
}

void remove_all_log_modules()
{
	struct installed_module *modules = NULL;

	{
		rwlock_writelock(&log_module_lock);
		modules = log_modules;
		log_modules = NULL;
		rwlock_unlock(&log_module_lock);
	}


	while (modules) {
		struct installed_module *current = modules;
		modules = list_next(modules);

		cleanup_log_module(current, NULL);
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

void enable_stdout_logging(bool enable)
{
	stdout_enable = enable;
}

static int stdout_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	const char *level_str = level_to_str(lvl);
	const int level_size = strlen(level_str);
	const int module_size = wcslen(module);
	FILE *fd = (lvl == HAKA_LOG_FATAL) ? stderr : stdout;

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

	return 0;
}

void message(log_level level, const wchar_t *module, const wchar_t *message)
{
	const log_level max_level = getlevel(module);
	if (level <= max_level) {
		struct installed_module *iter;

		rwlock_readlock(&log_module_lock);
		for (iter=log_modules; iter; iter = list_next(iter)) {
			iter->module->message(iter->state, level, module, message);
		}
		rwlock_unlock(&log_module_lock);

		if (stdout_enable) {
			stdout_message(level, module, message);
		}
	}
}

void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)
{
	const log_level max_level = getlevel(module);
	if (level <= max_level) {
		wchar_t *message_buffer = message_context();
		if (message_buffer) {
			va_list ap;
			va_start(ap, fmt);
			vswprintf(message_buffer, MESSAGE_BUFSIZE, fmt, ap);
			message_buffer[MESSAGE_BUFSIZE-1] = 0;
			message(level, module, message_buffer);
			va_end(ap);
		}
	}
}

struct module_level {
	wchar_t              *module;
	log_level            level;
	struct module_level *next;
};

static struct module_level *module_level = NULL;
static log_level default_level = HAKA_LOG_INFO;

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
	if (!module) {
		default_level = level;
	}
	else {
		struct module_level *module_level = get_module_level(module, true);
		if (module_level) {
			module_level->level = level;
		}
	}
}

log_level getlevel(const wchar_t *module)
{
	if (module) {
		struct module_level *module_level = get_module_level(module, false);
		if (module_level) {
			return module_level->level;
		}
	}
	return default_level;
}
