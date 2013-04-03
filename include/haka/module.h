/**
 * @file haka/module.h
 * @brief Module interface
 * @author Pierre-Sylvain Desse
 *
 * The file contains the description of a haka generic module.
 */

/**
 * @defgroup Module Module API
 * @brief Module API functions and structures.
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

#endif /* _HAKA_MODULE_H */
