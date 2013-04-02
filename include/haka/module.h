
#ifndef _HAKA_MODULE_H
#define _HAKA_MODULE_H


struct module {
	void       *handle;
	int         ref;

	enum {
		MODULE_UNKNOWN,
		MODULE_PACKET,
		MODULE_LOG,
		MODULE_EXTENSION,
	} type;

	const char *name;
	const char *description;
	const char *author;

	int       (*init)(int argc, char *argv[]);
	void      (*cleanup)();
};

#endif /* _HAKA_MODULE_H */

