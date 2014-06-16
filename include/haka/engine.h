/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_ENGINE_H
#define _HAKA_ENGINE_H

#include <haka/types.h>
#include <stdlib.h>

enum thread_status {
	THREAD_RUNNING,
	THREAD_WAITING,
	THREAD_DEBUG,
	THREAD_DEFUNC,
	THREAD_STOPPED
};

struct packet_stats {
	size_t       recv_packets;
	size_t       recv_bytes;
	size_t       trans_bytes;
	size_t       trans_packets;
	size_t       drop_packets;
};

struct engine_thread;
struct lua_State;

bool                           engine_prepare(int thread_count);

struct engine_thread          *engine_thread_init(struct lua_State *L, int id);
void                           engine_thread_cleanup(struct engine_thread *thread);
struct engine_thread          *engine_thread_current();
struct engine_thread          *engine_thread_byid(int id);

int                            engine_thread_id(struct engine_thread *thread);
enum thread_status             engine_thread_update_status(struct engine_thread *thread, enum thread_status status);
enum thread_status             engine_thread_status(struct engine_thread *thread);
volatile struct packet_stats  *engine_thread_statistics(struct engine_thread *thread);

bool                           engine_thread_remote_launch(struct engine_thread *thread, void (*callback)(void *), void *data);
int                            engine_thread_lua_remote_launch(struct engine_thread *thread, struct lua_State *L, int index);
char*                          engine_thread_raw_lua_remote_launch(struct engine_thread *thread, const char *code, size_t *size);
void                           engine_thread_check_remote_launch(struct engine_thread *thread);

#endif /* _HAKA_ENGINE_H */
