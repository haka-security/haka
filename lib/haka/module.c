
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <haka/module.h>
#include <haka/log.h>


#define MODULE_EXT	".ho"

struct module *module_load(const char *module_name, char **error, int argc, char *argv[])
{
	void *module_handle = NULL;
	struct module *module = NULL;
	char *full_module_name;

	assert(module_name);

	if (error) *error = NULL;

	full_module_name = malloc(strlen(module_name) + strlen(MODULE_EXT) + 1);
	if (!full_module_name) {
		return NULL;
	}

	strcpy(full_module_name, module_name);
	strcpy(full_module_name+strlen(module_name), MODULE_EXT);
	module_handle = dlopen(full_module_name, RTLD_NOW);

	if (!module_handle) {
		free(full_module_name);
		if (error) *error = strdup(dlerror());
		return NULL;
	}

	module = (struct module*)dlsym(module_handle, "HAKA_MODULE");
	if (!module) {
		if (error) *error = strdup(dlerror());
		dlclose(module);
		free(full_module_name);
		return NULL;
	}

	module->handle = module_handle;

	if (module->ref == 0) {
		/* Initialize the module */
		messagef(HAKA_LOG_INFO, L"core", L"load module '%s'\n\t%ls, %ls", full_module_name,
		         module->name, module->author);

		if (module->init(argc, argv)) {
			if (error) {
				*error = strdup("unable to initialize module");
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
	++module->ref;
}

void module_release(struct module *module)
{
	if (--module->ref == 0) {
		/* Cleanup the module */
		messagef(HAKA_LOG_INFO, L"core", L"unload module '%ls'", module->name);
		module->cleanup();
		dlclose(module->handle);
	}
}
