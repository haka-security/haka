
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <haka/module.h>
#include <haka/log.h>
#include <haka/error.h>


struct module *module_load(const char *module_name, int argc, char *argv[])
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
	module_handle = dlopen(full_module_name, RTLD_NOW);

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

	if (atomic_get(&module->ref) == 0) {
		/* Initialize the module */
		messagef(HAKA_LOG_INFO, L"core", L"load module '%s'\n\t%ls, %ls", full_module_name,
		         module->name, module->author);

		if (module->init(argc, argv) || check_error()) {
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

static char *modules_path;

void module_set_path(const char *path)
{
	if (!strchr(path, '*')) {
		error(L"invalid module path");
		return;
	}

	free(modules_path);
	modules_path = NULL;

	modules_path = strdup(path);
}

const char *module_get_path()
{
	return modules_path;
}
