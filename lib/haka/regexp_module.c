/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/error.h>
#include <haka/regexp_module.h>

struct regexp_module *regexp_module_load(const char *module_name, struct parameters *args) {
	struct module *module = module_load(module_name, args);
	if (module == NULL || module->type != MODULE_REGEXP) {
		if (module != NULL) module_release(module);
		error(L"Module %s is not of type MODULE_REGEXP", module_name);
		return NULL;
	}

	return (struct regexp_module *)module;
}
