/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>

#include <haka/log_module.h>
#include <haka/error.h>


static void log_module_destroy(struct logger *logger)
{
	struct logger_module *logger_module = (struct logger_module *)logger;
	struct module *module = &logger_module->module->module;

	logger_module->module->cleanup_logger(logger_module);
	module_release(module);
}

struct logger *log_module_logger(struct module *module, struct parameters *args)
{
	struct log_module *log_module;

	if (module->type != MODULE_LOG) {
		error(L"invalid module type: not a log module");
		return NULL;
	}

	log_module = (struct log_module *)module;

	struct logger_module *logger = log_module->init_logger(args);
	if (!logger) {
		assert(check_error());
		return NULL;
	}

	list_init(&logger->logger);

	logger->module = log_module;
	module_addref(module);
	logger->logger.destroy = log_module_destroy;
	logger->logger.mark_for_remove = false;

	return &logger->logger;
}
