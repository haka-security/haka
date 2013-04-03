/**
 * @file log_module.h
 * @brief Log module interface
 * @author Pierre-Sylvain Desse
 *
 * The file contains the description of a haka logging module.
 */

#ifndef _HAKA_LOG_MODULE_H
#define _HAKA_LOG_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/log.h>


/**
 * @brief Logging module structure
 *
 * Logging module description.
 * @ingroup Module
 */
struct log_module {
	struct module    module;

	/**
	 * Messaging function called by the application.
	 * @param level Logging level
	 * @param module Module name to report
	 * @param message Message
	 * @return Non-zero in case of error.
	 */
	int            (*message)(log_level level, const wchar_t *module, const wchar_t *message);
};

#endif /* _HAKA_LOG_MODULE_H */

