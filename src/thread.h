
#ifndef _THREAD_H
#define _THREAD_H

#include <haka/packet_module.h>
#include "lua/state.h"


struct thread_state;

struct thread_state *init_thread_state(struct packet_module *packet_module, int thread_id);
void cleanup_thread_state(struct packet_module *packet_module, struct thread_state *state);
void start_thread(struct thread_state *state);
void start_single(struct thread_state *state);
void wait_threads();

#endif /* _THREAD_H */

