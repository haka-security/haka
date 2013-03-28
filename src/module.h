
#include <haka/module.h>


/**
 * Load a module gicen its name
 */
struct module *load_module(const char *module_name);

/**
 * Unload a module
 */
void           unload_module(struct module *module);

