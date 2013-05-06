%module module
%include "args.i"

%{
#include <haka/module.h>
#include <haka/config.h>


#define load_args(name, ARGC, ARGV) \
	module_load(name, ARGC, ARGV)

#define load_no_args(name) \
	module_load(name, 0, NULL)

char *prefix = HAKA_MODULE_PREFIX;
char *suffix = HAKA_MODULE_SUFFIX;

%}

%include haka/swig.i

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

%rename(load) load_args;
%rename(load) load_no_args;

%newobject load_no_args;
struct module *load_no_args(const char *name);

%newobject load_args;
struct module *load_args(const char *name, int ARGC, char **ARGV);

%rename(path) module_get_path;
const char *module_get_path();

%rename(setPath) module_set_path;
void module_set_path(const char *path);

%immutable;
char *prefix;
char *suffix;

%luacode {
	package.cpath = package.cpath .. ";" .. string.gsub(module.path(), '*', module.prefix .. '?' .. module.suffix)
	package.path = package.path .. ";" .. string.gsub(module.path(), '*', '?.lua')
}
