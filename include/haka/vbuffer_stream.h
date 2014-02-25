/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_STREAM_H
#define _HAKA_VBUFFER_STREAM_H

#include <haka/vbuffer.h>


struct vbuffer_stream_chunk;

struct vbuffer_stream {
	struct lua_object            lua_object;
	struct vbuffer               data;
	struct list2                 chunks;
	struct list2                 read_chunks;
};

bool            vbuffer_stream_init(struct vbuffer_stream *stream);
void            vbuffer_stream_clear(struct vbuffer_stream *stream);
bool            vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *buffer);
void            vbuffer_stream_finish(struct vbuffer_stream *stream);
bool            vbuffer_stream_pop(struct vbuffer_stream *stream, struct vbuffer *buffer);
struct vbuffer *vbuffer_stream_data(struct vbuffer_stream *stream);
void            vbuffer_stream_current(struct vbuffer_stream *stream, struct vbuffer_iterator *position);

#endif /* _HAKA_VBUFFER_STREAM_H */
