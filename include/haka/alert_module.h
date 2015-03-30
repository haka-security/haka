/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Alert module back-end.
 */

#ifndef HAKA_ALERT_MODULE_H
#define HAKA_ALERT_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/alert.h>


/**
 * Alerter helper structure.
 */
struct alerter_module {
	struct alerter        alerter;
	struct alert_module  *module;
};

/**
 * Alert module structure. This module type will allow to create custom
 * alert back-end for Haka to, for instance, put the alert into syslog or
 * append it to a database.
 */
struct alert_module {
	struct module    module; /**< Module structure. */

	/**
	 * Create a new alerter. This function must fill the fields of the struct
	 * alerter_module.
	 */
	struct alerter_module *(*init_alerter)(struct parameters *args);

	/**
	 * Destroy an alerter.
	 */
	void                   (*cleanup_alerter)(struct alerter_module *alerter);
};

/**
 * Create an alerter from the given module.
 */
struct alerter *alert_module_alerter(struct module *module, struct parameters *args);

#endif /* HAKA_ALERT_MODULE_H */
