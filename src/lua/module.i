%module module
%include "args.i"

%{
#include <haka/module.h>


#define load_args(name, ARGC, ARGV) \
	module_load(name, ARGC, ARGV)

#define load_no_args(name) \
	module_load(name, 0, NULL)

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
