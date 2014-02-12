/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_REGEXP_MODULE_H
#define _HAKA_REGEXP_MODULE_H

#include <haka/module.h>

struct regexp;

struct regexp_module {
	struct module module;

	struct regexp *(*compile)(const char *pattern);
	void (*free)(struct regexp *regexp);
	int (*exec)(const struct regexp *regexp, const char *buffer, int len);
	int (*match)(const char *pattern, const char *buffer, int len);
	int (*feed)(const struct regexp *regexp, const char *buffer, int len);
};

struct regexp  {
	struct regexp_module *module;
};

struct regexp_module *regexp_module_load(const char *module_name, struct parameters *args);

#endif /* _HAKA_LOG_MODULE_H */
