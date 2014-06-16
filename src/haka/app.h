/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _APP_H
#define _APP_H

#include <haka/packet_module.h>
#include <haka/log_module.h>
#include "thread.h"


void basic_clean_exit();
void clean_exit();
void initialize();
void prepare(int threadcount, bool attach_debugger, bool grammar_debug);
void start();
struct thread_pool *get_thread_pool();

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

void dump_stat(FILE *file);

bool setup_loglevel(char *level);

#endif /* _APP_H */

