%module module
%include "args.i"

%{
#include <haka/module.h>

static struct module *load_impl(lua_State *L, const char *name, int ARGC, char **ARGV) {
	char *err;
	struct module *ret = module_load(name, &err, ARGC, ARGV);
	if (!ret) {
		if (err) {
			lua_pushfstring(L, "%s", err);
			free(err);
		}
		else {
			lua_pushfstring(L, "unknown error");
		}
		lua_error(L);
		return NULL;
	}
	else {
		return ret;
	}
}

#define load1(name, ARGC, ARGV) \
	load_impl(L, name, ARGC, ARGV); if (!result) SWIG_fail

#define load2(name) \
	load_impl(L, name, 0, NULL); if (!result) SWIG_fail

%}

%nodefaultctor;

struct module {
	%immutable;
	const wchar_t *name;
	const wchar_t *author;
	const wchar_t *description;

	%extend {
		~module()
		{
			module_release($self);
		}
	}
};

%rename(load) load1;
%rename(load) load2;

%newobject load2;
struct module *load2(const char *name);

%newobject load1;
struct module *load1(const char *name, int ARGC, char **ARGV);
