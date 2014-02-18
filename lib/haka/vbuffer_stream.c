/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <haka/vbuffer_stream.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/thread.h>

#include "vbuffer.h"
#include "vbuffer_data.h"


struct vbuffer_stream_chunk {
	struct list2_elem              list;
	struct vbuffer_data_ctl_push  *ctl_data;
	struct vbuffer_iterator        ctl_iter;
};

static void _vbuffer_stream_free_chunk(struct vbuffer_stream_chunk *chunk)
{
	free(chunk);
}

static void _vbuffer_stream_free_chunks(struct list2 *chunks)
{
	list2_iter iter = list2_begin(chunks);
	const list2_iter end = list2_end(chunks);

	while (iter != end) {
		struct vbuffer_stream_chunk *chunk = list2_get(iter, struct vbuffer_stream_chunk);
		iter = list2_erase(iter);
		_vbuffer_stream_free_chunk(chunk);
	}
}

bool vbuffer_stream_init(struct vbuffer_stream *stream)
{
	stream->lua_object = lua_object_init;
	vbuffer_create_empty(&stream->data);
	list2_init(&stream->chunks);
	list2_init(&stream->read_chunks);
	return true;
}

void vbuffer_stream_clear(struct vbuffer_stream *stream)
{
	_vbuffer_stream_free_chunks(&stream->chunks);
	_vbuffer_stream_free_chunks(&stream->read_chunks);

	lua_object_release(stream, &stream->lua_object);
	vbuffer_release(&stream->data);
}

bool vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *data)
{
	struct vbuffer_chunk *ctl;
	struct vbuffer_stream_chunk *chunk = malloc(sizeof(struct vbuffer_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	list2_elem_init(&chunk->list);

	chunk->ctl_data = vbuffer_data_ctl_push(stream, chunk);
	if (!chunk->ctl_data) {
		return false;
	}

	ctl = vbuffer_chunk_create_ctl(&chunk->ctl_data->super.super);
	if (!ctl) {
		free(chunk);
		return false;
	}

	list2_insert(list2_end(&stream->chunks), &chunk->list);

	list2_insert_list(list2_end(&stream->data.chunks), list2_begin(&data->chunks), list2_end(&data->chunks));
	list2_insert(list2_end(&stream->data.chunks), &ctl->list);

	vbuffer_clear(data);
	vbuffer_end(&stream->data, &chunk->ctl_iter);
	return true;
}

bool vbuffer_stream_pop(struct vbuffer_stream *stream, struct vbuffer *buffer)
{
	struct vbuffer_stream_chunk *current = list2_first(&stream->chunks, struct vbuffer_stream_chunk);
	struct vbuffer_stream_chunk *read_last = list2_last(&stream->read_chunks, struct vbuffer_stream_chunk);
	list2_iter begin, iter;
	bool keep_for_read = false;

	if (!current) {
		return false;
	}

	if (read_last) {
		begin = list2_next(&read_last->ctl_iter.chunk->list);
	}
	else {
		begin = list2_begin(&stream->data.chunks);
	}

	iter = begin;

	/* Check if the data can be pop */
	while (true) {
		struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);
		assert(!list2_atend(iter));

		if (chunk->data == &current->ctl_data->super.super) {
			break;
		}

		if (chunk->flags.ctl) {
#ifdef HAKA_DEBUG
			/* A push ctl node must be from another stream */
			if (chunk->data->ops == &vbuffer_data_ctl_push_ops) {
				struct vbuffer_data_ctl_push *ctl_data = (struct vbuffer_data_ctl_push *)chunk->data;
				assert(ctl_data->stream != stream);
			}
#endif

			if (chunk->data->ops == &vbuffer_data_ctl_select_ops) {
				return false;
			}

			if (chunk->data->ops == &vbuffer_data_ctl_mark_ops) {
				/* Mark found, nothing to pop */
				return false;
			}
		}

		iter = list2_next(iter);
	}

	if (keep_for_read) {
		//TODO
		return false;
	}

	/* Extract buffer data */
	vbuffer_create_empty(buffer);
	list2_insert_list(list2_end(&buffer->chunks), begin, iter);

	/* Remove push ctl node */
	assert(iter == &current->ctl_iter.chunk->list);
	vbuffer_chunk_clear(current->ctl_iter.chunk);

	/* Destroy chunk */
	list2_erase(&current->list);
	_vbuffer_stream_free_chunk(current);

	return true;
}

struct vbuffer *vbuffer_stream_data(struct vbuffer_stream *stream)
{
	return &stream->data;
}

bool vbuffer_stream_current(struct vbuffer_stream *stream, struct vbuffer_iterator *position)
{
	list2_iter iter, end = list2_end(&stream->chunks);
	struct vbuffer_stream_chunk *current;

	if (!vbuffer_begin(&stream->data, position)) return false;

	iter = list2_prev(end);
	if (iter == end) return true;

	iter = list2_prev(iter);
	if (iter == end) return true;

	current = list2_get(iter, struct vbuffer_stream_chunk);
	vbuffer_iterator_copy(position, &current->ctl_iter);
	return true;
}
