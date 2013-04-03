
#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H

#include <wchar.h>


struct module {
	void          *handle;
	int            ref;

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

#endif /* _HAKA_MODULE_H */

