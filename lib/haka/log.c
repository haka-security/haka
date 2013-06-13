
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/error.h>
#include <haka/log_module.h>


static struct log_module *log_module = NULL;

int set_log_module(struct module *module)
{
	struct log_module *prev_log_module = log_module;

	if (module && module->type != MODULE_LOG) {
		error(L"'%ls' is not a log module", module->name);
		return 1;
	}

	if (module) {
		log_module = (struct log_module *)module;
		module_addref(&log_module->module);
	}
	else {
		log_module = NULL;
	}

	if (prev_log_module) {
		module_release(&prev_log_module->module);
	}

	return 0;
}

int has_log_module()
{
	return log_module != NULL;
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

void message(log_level level, const wchar_t *module, const wchar_t *message)
{
	const log_level max_level = getlevel(module);
	if (level <= max_level) {
		if (log_module) {
			log_module->message(level, module, message);
		}
		else {
			fwprintf(stdout, L"%s: %ls: %ls\n", level_to_str(level), module, message);
		}
	}
}

#define MESSAGE_BUFSIZE   1024

void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)
{
	const log_level max_level = getlevel(module);
	if (level <= max_level) {
		wchar_t message_buffer[MESSAGE_BUFSIZE];

		va_list ap;
		va_start(ap, fmt);
		vswprintf(message_buffer, MESSAGE_BUFSIZE, fmt, ap);
		message(level, module, message_buffer);
		va_end(ap);
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
