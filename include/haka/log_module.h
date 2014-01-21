/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LOG_MODULE_H
#define _HAKA_LOG_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/log.h>


struct logger_module {
	struct logger       logger;
	struct log_module  *module;
};

struct log_module {
	struct module    module;

	struct logger_module *(*init_logger)(struct parameters *args);
	void                  (*cleanup_logger)(struct logger_module *logger);
};

struct logger *log_module_logger(struct module *module, struct parameters *args);

#endif /* _HAKA_LOG_MODULE_H */
