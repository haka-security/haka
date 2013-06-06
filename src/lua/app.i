%module app
%{
#include <stdint.h>
#include <wchar.h>

#include "app.h"
#include "state.h"
#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/config.h>

static int install(const char *type, struct module *module)
{
	if (strcmp(type, "packet") == 0) {
		return set_packet_module(module);
	}
	else if (strcmp(type, "log") == 0) {
		return set_log_module(module);
	}
	else {
		return 2;
	}
}

void load_configuration(const char *file)
{
	set_configuration_script(file);
}

%}

%include haka/swig.si

void install(const char *type, struct module *module);
void load_configuration(const char *file);
void exit(int);

%rename(currentThread) thread_get_id;
int thread_get_id();

%rename(directory) get_app_directory;
const char *get_app_directory();
