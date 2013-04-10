
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/log_module.h>


static struct log_module *log_module = NULL;

int set_log_module(struct module *module)
{
	struct log_module *prev_log_module = log_module;

	if (module && module->type != MODULE_LOG) {
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
	if (log_module) {
		log_module->message(level, module, message);
	}
	else {
		fwprintf(stdout, L"%s: %ls: %ls\n", level_to_str(level), module, message);
	}
}

#define MESSAGE_BUFSIZE   1024

void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)
{
	wchar_t message_buffer[MESSAGE_BUFSIZE];

	va_list ap;
	va_start(ap, fmt);
	vswprintf(message_buffer, MESSAGE_BUFSIZE, fmt, ap);
	message(level, module, message_buffer);
	va_end(ap);
}
