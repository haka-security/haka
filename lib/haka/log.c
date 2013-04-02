
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/log_module.h>


static struct log_module *log_module = NULL;

int set_log_module(struct module *module)
{
	if (module) {
		if (module->type == MODULE_LOG) {
			log_module = (struct log_module *)module;
			return 0;
		}
		else
			return 1;
	}
	else {
		log_module = NULL;
		return 0;
	}
}

int has_log_module()
{
	return log_module != NULL;
}

static const char *str_level[] = {
    "fatal",
    "error",
    "warning",
    "info",
    "debug",
};

const char *level_to_str(log_level level)
{
    assert(level >= 0 && level < LOG_LEVEL_LAST);
    return str_level[level];
}

void message(log_level level, const char *module, const char *message)
{
    if (log_module) {
        log_module->message(level, module, message);
    }
    else {
        fprintf(stderr, "%s: %s: %s\n", level_to_str(level), module, message);
    }
}

#define MESSAGE_BUFSIZE   4096
static char message_buffer[MESSAGE_BUFSIZE];

void messagef(log_level level, const char *module, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message_buffer, MESSAGE_BUFSIZE, fmt, ap);
    message(level, module, message_buffer);
    va_end(ap);
}

