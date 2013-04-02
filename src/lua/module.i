%module module
%{
#include <haka/module.h>
#include "../module.h"

struct module *load(const char *name) {
    return module_load(name);
}
%}

%nodefaultctor;

struct module {
    %immutable;
    const char *name;
    const char *author;
    const char *description;
};

struct module *load(const char *name);

