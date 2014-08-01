/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <wchar.h>

#include <haka/module.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/parameters.h>
#include <haka/system.h>


static char *modules_path = NULL;
static char *modules_cpath = NULL;

FINI static void _module_cleanup()
{
	free(modules_path);
	free(modules_cpath);
}

struct module *module_load(const char *module_name, struct parameters *args)
{
	void *module_handle = NULL;
	struct module *module = NULL;
	char *full_module_name;
	size_t module_name_len;

	assert(module_name);

	module_name_len = strlen(HAKA_MODULE_PREFIX) + strlen(module_name) + strlen(HAKA_MODULE_SUFFIX) + 1;
	full_module_name = malloc(module_name_len);
	if (!full_module_name) {
		return NULL;
	}

	snprintf(full_module_name, module_name_len, "%s%s%s", HAKA_MODULE_PREFIX, module_name, HAKA_MODULE_SUFFIX);
	assert(strlen(full_module_name)+1 == module_name_len);

	{
		char *current_path = modules_cpath, *iter;
		char *full_path;

		while ((iter = strchr(current_path, '*')) != NULL) {
			const int len = iter - current_path;
			const size_t full_path_len = len + strlen(full_module_name) + 1;

			full_path = malloc(full_path_len);
			if (!full_path) {
				return NULL;
			}

			snprintf(full_path, full_path_len, "%.*s%s", len, current_path, full_module_name);
			assert(strlen(full_path)+1 == full_path_len);

			module_handle = dlopen(full_path, RTLD_NOW);

			current_path = iter+1;
			if (*current_path != 0) ++current_path;

			free(full_path);
			full_path = NULL;

			if (module_handle) {
				break;
			}
		}
	}

	if (!module_handle) {
		free(full_module_name);
		error(L"%s", strdup(dlerror()));
		return NULL;
	}

	module = (struct module*)dlsym(module_handle, "HAKA_MODULE");
	if (!module) {
		error(L"%s", strdup(dlerror()));
		dlclose(module);
		free(full_module_name);
		return NULL;
	}

	module->handle = module_handle;

	if (module->api_version != HAKA_API_VERSION) {
		messagef(HAKA_LOG_INFO, L"core", L"%s: invalid API version", full_module_name);
		dlclose(module->handle);
		free(full_module_name);
		return NULL;
	}

	if (atomic_get(&module->ref) == 0) {
		/* Initialize the module */
		if (module->name && module->author) {
			messagef(HAKA_LOG_INFO, L"core", L"load module '%s', %ls, %ls",
			         full_module_name, module->name, module->author);
		}
		else if (module->name || module->author) {
			messagef(HAKA_LOG_INFO, L"core", L"load module '%s', %ls%ls",
			         full_module_name, module->name ? module->name : L"",
			         module->author ? module->author : L"");
		}
		else {
			messagef(HAKA_LOG_INFO, L"core", L"load module '%s'", full_module_name);
		}

		if (module->init(args) || check_error()) {
			if (check_error()) {
				error(L"unable to initialize module: %ls", clear_error());
			} else {
				error(L"unable to initialize module");
			}

			dlclose(module->handle);
			free(full_module_name);
			return NULL;
		}
	}

	free(full_module_name);

	module_addref(module);
	return module;
}

void module_addref(struct module *module)
{
	atomic_inc(&module->ref);
}

void module_release(struct module *module)
{
	if (atomic_dec(&module->ref) == 0) {
		/* Cleanup the module */
		messagef(HAKA_LOG_INFO, L"core", L"unload module '%ls'", module->name);
		module->cleanup();
		dlclose(module->handle);
	}
}

bool module_set_default_path()
{
	static const char *HAKA_CORE_PATH = "/share/haka/core/*";
	static const char *HAKA_MODULE_PATH = "/share/haka/modules/*";
	static const char *HAKA_MODULE_CPATH = "/lib/haka/modules/*";

	size_t path_len;
	char *path;
	const char *haka_path_s = haka_path();

	/* Lua module path */
	{
		/* format <haka_path_s><HAKA_CORE_PATH>;<haka_path_s><HAKA_MODULE_PATH>\0
		 * in a string.
		 */

		path_len = 2*strlen(haka_path_s) + strlen(HAKA_CORE_PATH) + 1 +
				strlen(HAKA_MODULE_PATH) + 1;

		path = malloc(path_len);
		if (!path) {
			error(L"memory error");
			return false;
		}

		snprintf(path, path_len, "%s%s;%s%s", haka_path_s, HAKA_CORE_PATH,
				haka_path_s, HAKA_MODULE_PATH);

		module_set_path(path, false);

		free(path);
	}

	/* C module path */
	{
		/* format <haka_path_s><HAKA_MODULE_CPATH>\0
		 * in a string.
		 */

		path_len = strlen(haka_path_s) + strlen(HAKA_MODULE_CPATH) + 1;

		path = malloc(path_len);
		if (!path) {
			error(L"memory error");
			return false;
		}

		snprintf(path, path_len, "%s%s", haka_path_s, HAKA_MODULE_CPATH);

		module_set_path(path, true);

		free(path);
	}

	return true;
}

void module_set_path(const char *path, bool c)
{
	char **old_path = c ? &modules_cpath : &modules_path;

	if (!strchr(path, '*')) {
		error(L"invalid module path");
		return;
	}

	free(*old_path);
	*old_path = NULL;

	*old_path = strdup(path);
	if (!*old_path) {
		error(L"memory error");
		return;
	}
}

void module_add_path(const char *path, bool c)
{
	char **old_path = c ? &modules_cpath : &modules_path;
	char *new_modules_path = NULL;
	const int modules_path_len = *old_path ? strlen(*old_path) : 0;

	if (!strchr(path, '*')) {
		error(L"invalid module path");
		return;
	}

	if (modules_path_len > 0) {
		new_modules_path = malloc(modules_path_len + strlen(path) + 2);
		if (!new_modules_path) {
			error(L"memory error");
			return;
		}

		strcpy(new_modules_path, *old_path);
		strcat(new_modules_path, ";");
		strcat(new_modules_path, path);
	}
	else {
		new_modules_path = malloc(strlen(path) + 1);
		assert(new_modules_path);

		strcpy(new_modules_path, path);
	}

	free(*old_path);
	*old_path = new_modules_path;
}

const char *module_get_path(bool c)
{
	return c ? modules_cpath : modules_path;
}
