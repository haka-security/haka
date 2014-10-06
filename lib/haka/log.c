/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
#include <haka/container/bitfield.h>


static struct logger *loggers = NULL;
static rwlock_t log_module_lock = RWLOCK_INIT;
static local_storage_t local_message_key;
static bool message_is_valid = false;

static bool stdout_enable = true;
static mutex_t stdout_mutex;
static bool stdout_use_colors = 0;
static int stdout_module_size = 0;

static void cleanup_sections(void);

#define MODULE_COLOR   CYAN

static const char *level_color[HAKA_LOG_LEVEL_LAST] = {
	BOLD RED,    // HAKA_LOG_FATAL
	BOLD RED,    // HAKA_LOG_ERROR
	BOLD YELLOW, // HAKA_LOG_WARNING
	BOLD,        // HAKA_LOG_INFO
	CLEAR,       // HAKA_LOG_DEBUG
	CLEAR,       // HAKA_LOG_TRACE
};

#define MESSAGE_BUFSIZE   3072

static void message_delete(void *value)
{
	free(value);
}

/*
 * This list should be synchronized with the enum
 * in haka/log.h.
 */
struct static_section_entry {
	const char *name;
	int         id;
};

static struct static_section_entry static_log_section[] = {
	{ "core", SECTION_CORE },
	{ "packet", SECTION_PACKET },
	{ "time", SECTION_TIME },
	{ "states", SECTION_STATES },
	{ "remote", SECTION_REMOTE },
	{ "external", SECTION_EXTERNAL },
	{ "lua", SECTION_LUA },
	{ NULL, 0 }
};

INIT static void _message_init()
{
	UNUSED bool ret;
	int i;

	ret = local_storage_init(&local_message_key, message_delete);
	assert(ret);

	ret = mutex_init(&stdout_mutex, true);
	assert(ret);

	stdout_use_colors = colors_supported(fileno(stdout));

	for (i=0; static_log_section[i].name; ++i) {
		UNUSED const int id = register_log_section(static_log_section[i].name);
		assert(id == static_log_section[i].id);
	}

	message_is_valid = true;
}

FINI static void _message_fini()
{
	remove_all_logger();

	{
		void *buffer = local_storage_get(&local_message_key);
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

	cleanup_sections();
}

struct message_context_t {
	bool       doing_message;
	char       buffer[MESSAGE_BUFSIZE];
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
		error("Log module is not registered");
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
	"trace",
	"default",
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

	if (level == HAKA_LOG_LEVEL_LAST) {
		error("invalid logging level: %s", str);
	}

	return level;
}

void enable_stdout_logging(bool enable)
{
	stdout_enable = enable;
}

bool stdout_message(log_level lvl, const char *module, const char *message)
{
	const char *level_str = level_to_str(lvl);
	const int level_size = strlen(level_str);
	const int module_size = strlen(module);
	FILE *fd = (lvl == HAKA_LOG_FATAL) ? stderr : stdout;

	thread_setcancelstate(false);

	mutex_lock(&stdout_mutex);

	if (module_size > stdout_module_size) {
		stdout_module_size = module_size;
	}

	if (stdout_use_colors) {
		fprintf(fd, "%s%s" CLEAR "%*s " MODULE_COLOR "%s:" CLEAR "%*s %s\n" CLEAR, level_color[lvl], level_str,
				level_size-5, "", module, stdout_module_size-module_size, "", message);
	}
	else {
		fprintf(fd, "%s%*s %s:%*s %s\n", level_str, level_size-5, "",
				module, stdout_module_size-module_size, "", message);
	}

	mutex_unlock(&stdout_mutex);

	thread_setcancelstate(true);

	return true;
}

/*
 * Section management
 */

#define MAX_SECTION   256

struct section_info {
	char                *name;
};

static struct section_info sections[MAX_SECTION];
static int sections_count = 0;

static void cleanup_sections(void)
{
	int i;
	for (i=0; i<sections_count; ++i) {
		free(sections[i].name);
		sections[i].name = NULL;
	}
}

section_id register_log_section(const char *name)
{
	int id = search_log_section(name);
	if (id != INVALID_SECTION_ID) {
		return id;
	}

	if (sections_count >= MAX_SECTION) {
		error("too many log section");
		return INVALID_SECTION_ID;
	}

	sections[sections_count].name = strdup(name);

	return sections_count++;
}

section_id search_log_section(const char *name)
{
	int i;

	for (i=0; i<sections_count; ++i) {
		if (strcmp(name, sections[i].name) == 0) {
			return i;
		}
	}

	return INVALID_SECTION_ID;
}

static void message(log_level level, const char *module, const char *message, struct message_context_t *context)
{
	bool remove_pass = false;

	context->doing_message = true;

	struct logger *iter;

	rwlock_readlock(&log_module_lock);
	for (iter = loggers; iter; iter = list_next(iter)) {
		iter->message(iter, level, module, message);

		remove_pass |= iter->mark_for_remove;
	}
	rwlock_unlock(&log_module_lock);

	if (stdout_enable) {
		stdout_message(level, module, message);
	}

	if (remove_pass) {
		rwlock_readlock(&log_module_lock);
		for (iter = loggers; iter; iter = list_next(iter)) {
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

void _messagef(log_level level, section_id section, const char *fmt, ...)
{
	struct message_context_t *context = message_context();
	if (context && !context->doing_message) {
		if (section >= sections_count) {
			error("invalid section");
			return;
		}

		va_list ap;
		va_start(ap, fmt);
		vsnprintf(context->buffer, MESSAGE_BUFSIZE, fmt, ap);
		message(level, sections[section].name, context->buffer, context);
		va_end(ap);
	}
}


/*
 * Section level.
 */

BITFIELD_STATIC(MAX_SECTION, bitfield_section);
static struct bitfield_section section_custom_level = BITFIELD_STATIC_INIT(MAX_SECTION);
static struct bitfield_section section_levels[HAKA_LOG_LEVEL_MAX] = {
	BITFIELD_STATIC_INIT(MAX_SECTION),
	BITFIELD_STATIC_INIT(MAX_SECTION),
	BITFIELD_STATIC_INIT(MAX_SECTION),
	BITFIELD_STATIC_INIT(MAX_SECTION),
	BITFIELD_STATIC_INIT(MAX_SECTION),
	BITFIELD_STATIC_INIT(MAX_SECTION),
};
static mutex_t log_level_lock = MUTEX_INIT;

static log_level default_level = HAKA_LOG_INFO;

bool check_section_log_level(section_id section, log_level level)
{
	assert(section < sections_count);
	assert(level < HAKA_LOG_LEVEL_MAX);

	return bitfield_get(&section_levels[level].bitfield, section);
}

bool setlevel(log_level level, const char *name)
{
	if (name) {
		int i;
		const section_id section = search_log_section(name);
		if (section == INVALID_SECTION_ID) {
			error("invalid section name");
			return false;
		}

		mutex_lock(&log_level_lock);

		if (level == HAKA_LOG_DEFAULT) {
			bitfield_set(&section_custom_level.bitfield, section, false);
			level = default_level;
		}
		else {
			bitfield_set(&section_custom_level.bitfield, section, true);
		}

		assert(level < HAKA_LOG_LEVEL_MAX);

		for (i=0; i<HAKA_LOG_LEVEL_MAX; ++i) {
			bitfield_set(&section_levels[i].bitfield, section, i <= level);
		}
	}
	else {
		int i, j;

		if (level == HAKA_LOG_DEFAULT) {
			error("invalid default log level");
			return false;
		}

		mutex_lock(&log_level_lock);

		for (i=0; i<HAKA_LOG_LEVEL_MAX; ++i) {
			for (j=0; j<MAX_SECTION; ++j) {
				if (!bitfield_get(&section_custom_level.bitfield, j)) {
					bitfield_set(&section_levels[i].bitfield, j, i <= level);
				}
			}
		}
	}

	mutex_unlock(&log_level_lock);

	return true;
}

log_level getlevel(const char *name)
{
	int i;
	const section_id section = search_log_section(name);
	if (section == INVALID_SECTION_ID) {
		error("invalid section name");
		return false;
	}

	for (i=1; i<HAKA_LOG_LEVEL_MAX; ++i) {
		if (!bitfield_get(&section_levels[i].bitfield, section)) {
			return (log_level)i-1;
		}
	}

	return (log_level)i-1;
}
