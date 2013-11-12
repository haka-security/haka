
#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H

#include <wchar.h>
#include <haka/thread.h>
#include <haka/version.h>
#include <haka/parameters.h>


struct module {
	void          *handle;
	atomic_t       ref;

	enum {
		MODULE_UNKNOWN,
		MODULE_PACKET,
		MODULE_LOG,
		MODULE_ALERT,
		MODULE_EXTENSION,
	} type;

	const wchar_t *name;
	const wchar_t *description;
	const wchar_t *author;
	int            api_version;

	int          (*init)(struct parameters *args);
	void         (*cleanup)();
};

struct module *module_load(const char *module_name, struct parameters *args);
void           module_addref(struct module *module);
void           module_release(struct module *module);
void           module_set_path(const char *path);
void           module_add_path(const char *path);
const char    *module_get_path();

#endif /* _HAKA_MODULE_H */
