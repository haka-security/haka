
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


static char *modules_path = NULL;

FINI static void _module_cleanup()
{
	free(modules_path);
}

struct module *module_load(const char *module_name, struct parameters *args)
{
	void *module_handle = NULL;
	struct module *module = NULL;
	char *full_module_name;

	assert(module_name);

	full_module_name = malloc(strlen(HAKA_MODULE_PREFIX) + strlen(module_name) + strlen(HAKA_MODULE_SUFFIX) + 1);
	if (!full_module_name) {
		return NULL;
	}

	strcpy(full_module_name, HAKA_MODULE_PREFIX);
	strcpy(full_module_name+strlen(HAKA_MODULE_PREFIX), module_name);
	strcpy(full_module_name+strlen(HAKA_MODULE_PREFIX)+strlen(module_name), HAKA_MODULE_SUFFIX);

	{
		char *current_path = modules_path, *iter;
		char *full_path;

		while ((iter = strchr(current_path, '*')) != NULL) {
			const int len = iter - current_path;

			full_path = malloc(len + strlen(full_module_name) + 1);
			if (!full_path) {
				return NULL;
			}

			strncpy(full_path, current_path, len);
			full_path[len] = 0;
			strcat(full_path, full_module_name);
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
		messagef(HAKA_LOG_INFO, L"core", L"load module '%s'\n\t%ls, %ls", full_module_name,
		         module->name, module->author);

		if (module->init(args) || check_error()) {
			if (check_error())
				error(L"unable to initialize module: %ls", clear_error());
			else
				error(L"unable to initialize module");

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

void module_set_path(const char *path)
{
	if (!strchr(path, '*')) {
		error(L"invalid module path");
		return;
	}

	free(modules_path);
	modules_path = NULL;

	modules_path = strdup(path);
	assert(modules_path);
}

void module_add_path(const char *path)
{
	char *new_modules_path = NULL;
	const int modules_path_len = modules_path ? strlen(modules_path) : 0;

	if (!strchr(path, '*')) {
		error(L"invalid module path");
		return;
	}

	if (modules_path_len > 0) {
		new_modules_path = malloc(modules_path_len + strlen(path) + 2);
		assert(new_modules_path);

		strcpy(new_modules_path, modules_path);
		strcat(new_modules_path, ";");
		strcat(new_modules_path, path);
	}
	else {
		new_modules_path = malloc(strlen(path) + 1);
		assert(new_modules_path);

		strcpy(new_modules_path, path);
	}

	free(modules_path);
	modules_path = new_modules_path;
}

const char *module_get_path()
{
	return modules_path;
}
