%module module
%include "args.i"

%{
#include <haka/module.h>


#define load1(name, ARGC, ARGV) \
	module_load(name, ARGC, ARGV)

#define load2(name) \
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

%rename(load) load1;
%rename(load) load2;

%newobject load2;
struct module *load2(const char *name);

%newobject load1;
struct module *load1(const char *name, int ARGC, char **ARGV);
