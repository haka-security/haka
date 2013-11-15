
#ifndef _THREAD_H
#define _THREAD_H

#include <haka/packet_module.h>
#include "lua/state.h"


struct thread_pool;

struct thread_pool *thread_pool_create(int count, struct packet_module *packet_module, bool attach_debugger);
void thread_pool_cleanup(struct thread_pool *pool);
void thread_pool_wait(struct thread_pool *pool);
void thread_pool_cancel(struct thread_pool *pool);
void thread_pool_start(struct thread_pool *pool);
void thread_pool_attachdebugger(struct thread_pool *pool);
bool thread_pool_issingle(struct thread_pool *pool);

#endif /* _THREAD_H */

