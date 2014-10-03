/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Logging API.
 */

#ifndef _HAKA_LOG_H
#define _HAKA_LOG_H

#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/container/list.h>


/**
 * Log level.
 */
typedef enum {
	HAKA_LOG_FATAL, /**< Fatal errors. */
	HAKA_LOG_ERROR, /**< Errors. */
	HAKA_LOG_WARNING, /**< Warning. */
	HAKA_LOG_INFO, /**< Informations. */
	HAKA_LOG_DEBUG, /**< Debugging informations. */
	HAKA_LOG_TRACE, /**< Trace debugging (only available in debug build). */

	HAKA_LOG_DEFAULT, /**< Reset module log level to global one. */
	HAKA_LOG_LEVEL_LAST /**< Last log level. For internal use only. */
} log_level;

#define HAKA_LOG_LEVEL_MAX   6

/**
 * Convert a logging level to a human readable string.
 *
 * \returns A string representing the logging level. This string is constant and should
 * not be freed.
 */
const char *level_to_str(log_level level);

/**
 * Convert a logging level represented by a string to the matching
 * enum value.
 */
log_level str_to_level(const char *str);

/**
 * Log section identifier.
 */
typedef int section_id;

/**
 * Global section id.
 */
enum {
	SECTION_CORE,
	SECTION_PACKET,
	SECTION_TIME,
	SECTION_STATES,
	SECTION_REMOTE,
	SECTION_EXTERNAL,
	SECTION_LUA,
};

/**
 * Invalid section id.
 */
#define INVALID_SECTION_ID -1

/**
 * Register a log section.
 */
section_id register_log_section(const char *name);

#define DECLARE_LOG_SECTION(name) \
	int name##_section;

#define REGISTER_LOG_SECTION(name) \
	int name##_section = SECTION_EXTERNAL; \
	INIT static void __init_log_section_##name() { name##_section = register_log_section(#name); }

/**
 * Search for a log section by name.
 * \returns The section id or INVALID_SECTION_ID if not found.
 */
section_id search_log_section(const char *name);

/**
 * Check if section should be logged.
 */
bool check_section_log_level(section_id section, log_level level);

void _messagef(log_level level, section_id sectio, const char *fmt, ...) FORMAT_PRINTF(3, 4);

#define _LOG(level, section, fmt, ...) \
	do { if (check_section_log_level(section, level)) { \
		_messagef(level, section, fmt, ##__VA_ARGS__); \
	} } while(0)

/**
 * Log a message with string formating in various level.
 */
#define LOG_FATAL(section, fmt, ...)    _LOG(HAKA_LOG_FATAL, section, fmt, ##__VA_ARGS__)
#define LOG_ERROR(section, fmt, ...)    _LOG(HAKA_LOG_ERROR, section, fmt, ##__VA_ARGS__)
#define LOG_WARNING(section, fmt, ...)  _LOG(HAKA_LOG_WARNING, section, fmt, ##__VA_ARGS__)
#define LOG_INFO(section, fmt, ...)     _LOG(HAKA_LOG_INFO, section, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(section, fmt, ...)    _LOG(HAKA_LOG_DEBUG, section, fmt, ##__VA_ARGS__)

#ifdef HAKA_DEBUG
	#define LOG_TRACE(section, fmt, ...) _LOG(HAKA_LOG_TRACE, section, fmt, ##__VA_ARGS__)
#else
	#define LOG_TRACE(section, fmt, ...)
#endif

/**
 * Set the logging level to display for a given section name. The `name` parameter can be
 * `NULL` in which case it will set the default level.
 */
bool setlevel(log_level level, const char *name);

/**
 * Get the logging level for a given section name.
 */
log_level getlevel(const char *name);

/**
 * Change the display of log message on stdout.
 */
void enable_stdout_logging(bool enable);

/**
 * Show a log line on the stdout.
 */
bool stdout_message(log_level lvl, const char *module, const char *message);

/**
 * Logger instance structure.
 */
struct logger {
	struct list   list;
	void        (*destroy)(struct logger *state);
	int         (*message)(struct logger *state, log_level level, const char *module, const char *message);
	bool          mark_for_remove; /**< \private */
};

/**
 * Add a new logger instance in the logger list.
 * Each logger will receive all logging messages that are
 * emitted by the code.
 */
bool add_logger(struct logger *logger);

/**
 * Remove a logger.
 */
bool remove_logger(struct logger *logger);

/**
 * Remove all loggers.
 */
void remove_all_logger();

#endif /* _HAKA_LOG_H */
