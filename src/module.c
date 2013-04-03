
#include "module.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <haka/log.h>


#define MODULE_EXT  ".ho"

struct module *module_load(const char *module_name)
{
	void *module_handle = NULL;
	struct module *module = NULL;
	char *full_module_name;

	full_module_name = malloc(strlen(module_name) + strlen(MODULE_EXT) + 1);
	if (!full_module_name) {
		return NULL;
	}

	strcpy(full_module_name, module_name);
	strcpy(full_module_name+strlen(module_name), MODULE_EXT);
	module_handle = dlopen(full_module_name, RTLD_NOW);

	if (!module_handle) {
		messagef(LOG_ERROR, L"core", L"%s", dlerror());
		free(full_module_name);
		return NULL;
	}

	module = (struct module*)dlsym(module_handle, "HAKA_MODULE");
	if (!module) {
		messagef(LOG_ERROR, L"core", L"%s", dlerror());
		dlclose(module);
		free(full_module_name);
		return NULL;
	}

	module->handle = module_handle;

	if (module->ref++ == 0) {
		/* Initialize the module */
        messagef(LOG_INFO, L"core", L"load module '%s'\n\t%ls, %ls", full_module_name,
                module->name, module->author);

		if (module->init(0, NULL)) {
			messagef(LOG_ERROR, L"core", L"%s: unable to initialize module", full_module_name);
			dlclose(module);
			free(full_module_name);
			return NULL;
		}
	}

	free(full_module_name);
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
		module->cleanup();
	}

	dlclose(module->handle);
}

