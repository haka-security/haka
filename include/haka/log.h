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
	HAKA_LOG_DEFAULT, /**< Reset module log level to global one. */

	HAKA_LOG_LEVEL_LAST /**< Last log level. For internal use only. */
} log_level;

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
 * Log a message without string formating.
 */
void message(log_level level, const char *module, const char *message);

/**
 * Log a message with string formating.
 */
void messagef(log_level level, const char *module, const char *fmt, ...) FORMAT_PRINTF(3, 4);

/**
 * Set the logging level to display for a given module name. The `module` parameter can be
 * `NULL` in which case it will set the default level.
 */
void setlevel(log_level level, const char *module);

/**
 * Get the logging level for a given module name.
 */
log_level getlevel(const char *module);

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
