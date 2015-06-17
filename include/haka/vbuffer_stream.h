/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Stream based on vbuffer.
 */

#ifndef HAKA_VBUFFER_STREAM_H
#define HAKA_VBUFFER_STREAM_H

#include <haka/vbuffer.h>


struct vbuffer_stream_chunk;

/**
 * Stream based on vbuffer.
 * This object will create a view of the data as a continuous
 * buffer thanks to vbuffer.
 */
struct vbuffer_stream {
	struct lua_object            lua_object;  /**< \private */
	struct vbuffer               data;        /**< \private */
	struct list2                 chunks;      /**< \private */
	struct list2                 read_chunks; /**< \private */
	void                       (*userdata_cleanup)(void *); /**< \private */
};

/**
 * Initialize a new stream.
 */
bool            vbuffer_stream_init(struct vbuffer_stream *stream, void (*userdata_cleanup)(void *));

/**
 * Clear a stream.
 */
void            vbuffer_stream_clear(struct vbuffer_stream *stream);

/**
 * Push some data in the stream. Fill the iterator `current` with the position right
 * before the data just added.
 */
bool            vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *buffer, void *userdata, struct vbuffer_iterator *current);

/**
 * Finish the stream and mark the end as the eof. It is not possible to push any more data
 * after calling this function.
 */
void            vbuffer_stream_finish(struct vbuffer_stream *stream);

/**
 * Check if the stream has been finished.
 */
bool            vbuffer_stream_isfinished(struct vbuffer_stream *stream);

/**
 * Get some data out of the stream. The data are only extracted if no mark have been inserted
 * in the buffer. Otherwise, the mark will need to be removed before calling this function
 * again.
 */
bool            vbuffer_stream_pop(struct vbuffer_stream *stream, struct vbuffer *buffer, void **userdata);

/**
 * Get the buffer inside the stream.
 */
struct vbuffer *vbuffer_stream_data(struct vbuffer_stream *stream);

#endif /* HAKA_VBUFFER_STREAM_H */
