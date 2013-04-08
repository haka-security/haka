
#ifndef _MODULE_H
#define _MODULE_H

#include <haka/module.h>


/**
 * Load a module given its name. It is not needed to call
 * module_addref on the result as this is done before returning.
 */
struct module *module_load(const char *module_name, char **error, int argc, char *argv[]);

/**
 * Keep the module. Must match with a call to module_release
 * otherwise the module will not be able to be removed correctly
 * when unused.
 */
void           module_addref(struct module *module);

/**
 * Release a module.
 */
void           module_release(struct module *module);


#endif /* _MODULE_H */

