
#ifndef _APP_H
#define _APP_H

#include <haka/packet_module.h>
#include <haka/log_module.h>
#include "lua/state.h"
#include "thread.h"


void clean_exit();
void initialize();
void prepare(int threadcount);
void start();

const char *haka_path();

struct module;

extern int set_packet_module(struct module *module);
extern struct packet_module *get_packet_module();
extern int set_log_module(struct module *module);
extern int has_log_module();

int set_configuration_script(const char *file);
const char *get_configuration_script();

extern char directory[1024];

const char *get_app_directory();

#endif /* _APP_H */

