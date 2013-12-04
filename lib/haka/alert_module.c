/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>

#include <haka/alert_module.h>
#include <haka/error.h>


static void alert_module_destroy(struct alerter *alerter)
{
	struct alerter_module *alerter_module = (struct alerter_module *)alerter;
	struct module *module = &alerter_module->module->module;

	alerter_module->module->cleanup_alerter(alerter_module);
	module_release(module);
}

struct alerter *alert_module_alerter(struct module *module, struct parameters *args)
{
	struct alert_module *alert_module;

	if (module->type != MODULE_ALERT) {
		error(L"invalid module type: not an alert module");
		return NULL;
	}

	alert_module = (struct alert_module *)module;

	struct alerter_module *alerter = alert_module->init_alerter(args);
	if (!alerter) {
		assert(check_error());
		return NULL;
	}

	list_init(&alerter->alerter);

	alerter->module = alert_module;
	module_addref(module);
	alerter->alerter.destroy = alert_module_destroy;
	alerter->alerter.mark_for_remove = false;

	return &alerter->alerter;
}
