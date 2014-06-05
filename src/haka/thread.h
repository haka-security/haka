/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _THREAD_H
#define _THREAD_H

#include <haka/packet_module.h>


struct thread_pool;

struct thread_pool *thread_pool_create(int count, struct packet_module *packet_module,
		bool attach_debugger, bool grammar_debug);
int  thread_pool_count(struct thread_pool *pool);
void thread_pool_cleanup(struct thread_pool *pool);
void thread_pool_wait(struct thread_pool *pool);
void thread_pool_cancel(struct thread_pool *pool);
void thread_pool_start(struct thread_pool *pool);
void thread_pool_attachdebugger(struct thread_pool *pool);
bool thread_pool_issingle(struct thread_pool *pool);
struct engine_thread *thread_pool_thread(struct thread_pool *pool, int index);
void thread_pool_dump_stat(struct thread_pool *pool, FILE *file);

#endif /* _THREAD_H */

