
#include "module.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MODULE_EXT  ".ho"

struct module *load_module(const char *module_name)
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
	module_handle = dlopen(full_module_name, RTLD_LAZY);

	if (!module_handle) {
		fprintf(stderr, "%s\n", dlerror());
		free(full_module_name);
		return NULL;
	}

	module = (struct module*)dlsym(module_handle, "HAKA_MODULE");
	if (!module) {
		fprintf(stderr, "%s\n", dlerror());
		dlclose(module);
		free(full_module_name);
		return NULL;
	}

	module->handle = module_handle;

	/* Initialize the module */
	if (module->init(0, NULL)) {
		fprintf(stderr, "%s: unable to initialize module\n", full_module_name);
		dlclose(module);
		free(full_module_name);
		return NULL;
	}

	free(full_module_name);
	return module;
}

void unload_module(struct module *module)
{
	/* Cleanup the module */
	module->cleanup();

	dlclose(module->handle);
}

