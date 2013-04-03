%module module
%{
#include <haka/module.h>
#include "../module.h"

static struct module *load_impl(lua_State *L, const char *name) {
	char *err;
	struct module *ret = module_load(name, &err);
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

#define load(name) \
	load_impl(L, name); if (!result) SWIG_fail
%}

%nodefaultctor;

struct module {
	%immutable;
	const wchar_t *name;
	const wchar_t *author;
	const wchar_t *description;
};

struct module *load(const char *name);

