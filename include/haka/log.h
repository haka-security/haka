/**
 * @file log.h
 * @brief Logging API
 * @author Pierre-Sylvain Desse
 *
 * The file contains the logging API functions.
 */

/**
 * @defgroup Logging Logging API
 * @brief Logging API functions and structures.
 */

#ifndef _HAKA_LOG_H
#define _HAKA_LOG_H

#include <wchar.h>


/**
 * Enumerate the logging levels.
 * @ingroup Logging
 */
typedef enum {
	LOG_FATAL,   /**< Fatal error */
	LOG_ERROR,   /**< Error */
	LOG_WARNING, /**< Warning */
	LOG_INFO,    /**< Information */
	LOG_DEBUG,   /**< Debugging */

	LOG_LEVEL_LAST
} log_level;

/**
 * Convert a logging level to a human readable string.
 * @param level Logging level to convert.
 * @return String representing the logging level. This string is constant and should
 * not be freed.
 * @ingroup Logging
 */
const char *level_to_str(log_level level);

/**
 * Log a message without string formating.
 * @param level Logging level.
 * @param module Module name to report.
 * @param message Message to report
 * @ingroup Logging
 */
void message(log_level level, const wchar_t *module, const wchar_t *message);

/**
 * Log a message with string formating.
 * @param level Logging level.
 * @param module Module name to report.
 * @param fmt Message format
 * @ingroup Logging
 */
void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...);

#endif /* _HAKA_LOG_H */

