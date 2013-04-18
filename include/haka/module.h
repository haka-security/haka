/**
 * @file haka/module.h
 * @brief Module interface
 * @author Pierre-Sylvain Desse
 *
 * The file contains the description of a haka generic module.
 */

/**
 * @defgroup API API
 * @brief API functions and structures.
 */

/**
 * @defgroup Module Module
 * @brief Module API functions and structures.
 * @ingroup API
 */

/**
 * @defgroup ExternModule External Modules
 * @brief External module documentation.
 */

#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H

#include <wchar.h>


/**
 * @brief Module structure
 *
 * Generic module structure.
 * @ingroup Module
 */
struct module {
	void          *handle;
	int            ref;

	enum {
		MODULE_UNKNOWN,    /**< Unknown module */
		MODULE_PACKET,     /**< Packet module @see packet_module */
		MODULE_LOG,        /**< Logging module @see log_module */
		MODULE_EXTENSION,  /**< Extension module */
	} type;

	const wchar_t *name; /**< Full name */
	const wchar_t *description; /**< Description */
	const wchar_t *author; /**< Author */

	/**
	 * Initialize the module. This function is called by the application.
	 * @return Non zero in case of an error. In this case the module will be
	 * unloaded but cleanup is not going to be called.
	 */
	int          (*init)(int argc, char *argv[]);

	/**
	 * Cleanup the module. This function is called by the application when the
	 * module is unloaded.
	 */
	void         (*cleanup)();
};

/**
 * Load a module given its name. It is not needed to call
 * module_addref on the result as this is done before returning.
 * @param module_name The name of the module to load.
 * @param argc Extra arguments count.
 * @param argv Argument list.
 * @return The loaded module structure or NULL in case of an error.
 * @ingroup Module
 */
struct module *module_load(const char *module_name, int argc, char *argv[]);

/**
 * Keep the module. Must match with a call to module_release
 * otherwise the module will not be able to be removed correctly
 * when unused.
 * @param module Module to keep.
 * @ingroup Module
 */
void           module_addref(struct module *module);

/**
 * Release a module.
 * @param module Module to release.
 * @ingroup Module
 */
void           module_release(struct module *module);


#endif /* _HAKA_MODULE_H */
