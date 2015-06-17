/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Logging back-end module.
 */

#ifndef HAKA_LOG_MODULE_H
#define HAKA_LOG_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/log.h>


/**
 * Logger helper structure.
 */
struct logger_module {
	struct logger       logger;
	struct log_module  *module;
};

/**
 * Log module structure. This module type will allow to create custom
 * logging back-end for Haka to, for instance, put the log into syslog.
 */
struct log_module {
	struct module    module; /**< Module structure. */

	/**
	 * Create a new logger. This function must fill the fields of the struct
	 * logger_module.
	 */
	struct logger_module *(*init_logger)(struct parameters *args);

	/**
	 * Destroy a logger.
	 */
	void                  (*cleanup_logger)(struct logger_module *logger);
};

/**
 * Create a logger from the given module.
 */
struct logger *log_module_logger(struct module *module, struct parameters *args);

#endif /* HAKA_LOG_MODULE_H */
