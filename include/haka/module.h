
#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H

#include <wchar.h>
#include <haka/thread.h>

#define MAX_EXTRA_MODULE_PARAMETERS 10

struct module {
	void          *handle;
	atomic_t       ref;

	enum {
		MODULE_UNKNOWN,
		MODULE_PACKET,
		MODULE_LOG,
		MODULE_EXTENSION,
	} type;

	const wchar_t *name;
	const wchar_t *description;
	const wchar_t *author;

	int          (*init)(int argc, char *argv[]);
	void         (*cleanup)();
};

struct module *module_load(const char *module_name,...);
void           module_addref(struct module *module);
void           module_release(struct module *module);
void           module_set_path(const char *path);
const char    *module_get_path();

#endif /* _HAKA_MODULE_H */
