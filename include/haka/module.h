/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Generic module.
 */

#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H

#include <wchar.h>
#include <haka/thread.h>
#include <haka/version.h>
#include <haka/parameters.h>


/**
 * Module type.
 */
enum module_type {
	MODULE_UNKNOWN,   /**< Invalid module type. */
	MODULE_PACKET,    /**< Packet capture module. */
	MODULE_LOG,       /**< Logging module type. */
	MODULE_ALERT,     /**< Alert backend module type. */
	MODULE_REGEXP,    /**< Regular expression module type. */
	MODULE_EXTENSION, /**< Abstract extension module type. */
};

/**
 * Module declaration object.
 */
struct module {
	void          *handle;      /**< \private */
	atomic_t       ref;         /**< \private */

	enum module_type type;      /**< Module type */

	const char    *name;        /**< Module name. */
	const char    *description; /**< Module description. */
	const char    *author;      /**< Module author. */
	int            api_version; /**< API version (use HAKA_API_VERSION). */

	/**
	 * Initialize the module. This function is called by the application.
	 *
	 * \returns Non zero in case of an error. In this case the module will be
	 * unloaded but cleanup is not going to be called.
	 */
	int          (*init)(struct parameters *args);

	/**
	 * Cleanup the module. This function is called by the application when the
	 * module is unloaded.
	 */
	void         (*cleanup)();
};

/**
 * Load a module given its name. It is not needed to call module_addref() on the result
 * as this is done before returning.
 *
 * \returns The loaded module structure or NULL in case of an error.
 */
struct module *module_load(const char *module_name, struct parameters *args);

/**
 * Keep the module. Must match with a call to module_release()
 * otherwise the module will not be able to be removed correctly
 * when unused.
 */
void           module_addref(struct module *module);

/**
 * Decrement the module ref count and unload the module if needed.
 */
void           module_release(struct module *module);

/**
 * Set the path used to load haka modules. This path must be in the form:
 * `path/to/modules/\star;another/path/\star`
 */
void           module_set_path(const char *path, bool c);

/**
 * Set the module path using the env variable HAKA_PATH.
 */
bool           module_set_default_path();

/**
 * Add a path to the Lua module search path.
 */
void           module_add_path(const char *path, bool c);

/**
 * Get the modules search path.
 */
const char    *module_get_path(bool c);

#endif /* _HAKA_MODULE_H */
