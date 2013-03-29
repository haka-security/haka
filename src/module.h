
#ifndef _MODULE_H
#define _MODULE_H

#include <haka/module.h>


/**
 * Load a module given its name
 */
struct module *load_module(const char *module_name);

/**
 * Unload a module
 */
void           unload_module(struct module *module);

#endif /* _MODULE_H */

