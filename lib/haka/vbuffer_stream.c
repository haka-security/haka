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
		struct vbuffer_stream_chunk *chunk = list2_get(iter, struct vbuffer_stream_chunk, list);
		iter = list2_erase(iter);
		_vbuffer_stream_free_chunk(chunk);
	}
}

static void _vbuffer_stream_cleanup(struct vbuffer_stream *stream)
{
	list2_iter iter = list2_begin(vbuffer_chunk_list(&stream->data));
	list2_iter iter_read_chunk = list2_begin(&stream->read_chunks);
	const list2_iter end = list2_end(vbuffer_chunk_list(&stream->data));
	const list2_iter end_read_chunk = list2_end(&stream->read_chunks);

	while (iter != end && iter_read_chunk != end_read_chunk) {
		struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk, list);
		struct vbuffer_stream_chunk *read_chunk = list2_get(iter_read_chunk, struct vbuffer_stream_chunk, list);

		if (chunk->data->ops == &vbuffer_data_ctl_mark_ops)
			return;

		if (chunk->data->ops == &vbuffer_data_ctl_push_ops) {
			assert(chunk->data == &read_chunk->ctl_data->super.super);

			iter_read_chunk = list2_erase(iter_read_chunk);
			_vbuffer_stream_free_chunk(read_chunk);
		}

		iter = list2_erase(iter);
		vbuffer_chunk_clear(chunk);
	}
}

bool vbuffer_stream_init(struct vbuffer_stream *stream)
{
	stream->lua_object = lua_object_init;
	vbuffer_create_empty(&stream->data);
	stream->data.chunks->flags.eof = false;
	list2_init(&stream->chunks);
	list2_init(&stream->read_chunks);
	return true;
}

void vbuffer_stream_clear(struct vbuffer_stream *stream)
{
	_vbuffer_stream_free_chunks(&stream->chunks);
	_vbuffer_stream_free_chunks(&stream->read_chunks);

	vbuffer_release(&stream->data);
	lua_object_release(stream, &stream->lua_object);
}

bool vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *data)
{
	struct vbuffer_chunk *ctl, *end;
	struct vbuffer_stream_chunk *chunk;

	if (stream->data.chunks->flags.eof) {
		error(L"stream marked as finished");
		return false;
	}

	chunk = malloc(sizeof(struct vbuffer_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	list2_elem_init(&chunk->list);

	chunk->ctl_data = vbuffer_data_ctl_push(stream, chunk);
	if (!chunk->ctl_data) {
		return false;
	}

	ctl = vbuffer_chunk_insert_ctl(vbuffer_chunk_end(data), &chunk->ctl_data->super.super);
	if (!ctl) {
		free(chunk);
		return false;
	}

	end = vbuffer_chunk_end(&stream->data);
	list2_insert_list(&end->list, &vbuffer_chunk_begin(data)->list, &vbuffer_chunk_end(data)->list);

	list2_insert(list2_end(&stream->chunks), &chunk->list);

	vbuffer_clear(data);

	chunk->ctl_iter = vbuffer_iterator_init;
	vbuffer_iterator_update(&chunk->ctl_iter, ctl, 0);
	return true;
}

void vbuffer_stream_finish(struct vbuffer_stream *stream)
{
	assert(stream);
	stream->data.chunks->flags.eof = true;
}

bool vbuffer_stream_pop(struct vbuffer_stream *stream, struct vbuffer *buffer)
{
	struct vbuffer_stream_chunk *current = list2_first(&stream->chunks, struct vbuffer_stream_chunk, list);
	struct vbuffer_stream_chunk *read_last;
	struct vbuffer_chunk *begin, *iter, *start_of_keep, *end;
	bool keep_for_read = false;

	if (!current) {
		return false;
	}

	_vbuffer_stream_cleanup(stream);

	read_last = list2_last(&stream->read_chunks, struct vbuffer_stream_chunk, list);

	if (read_last) {
		begin = vbuffer_chunk_next(read_last->ctl_iter.chunk);
	}
	else {
		begin = vbuffer_chunk_begin(&stream->data);
	}

	iter = begin;

	/* Check if the data can be pop */
	while (true) {
		assert(!iter->flags.end);

		// Until end of pushed chunk
		if (iter->data == &current->ctl_data->super.super) {
			break;
		}

		if (iter->flags.ctl) {
#ifdef HAKA_DEBUG
			/* A push ctl node must be from another stream */
			if (iter->data->ops == &vbuffer_data_ctl_push_ops) {
				struct vbuffer_data_ctl_push *ctl_data = (struct vbuffer_data_ctl_push *)iter->data;
				assert(ctl_data->stream != stream);
			}
#endif

			if (iter->data->ops == &vbuffer_data_ctl_select_ops) {
				return false;
			}

			if (iter->data->ops == &vbuffer_data_ctl_mark_ops) {
				struct vbuffer_data_ctl_mark *ctl_mark_data = (struct vbuffer_data_ctl_mark *)iter->data;
				if (!ctl_mark_data->readonly) {
					/* Read/write mark found, nothing to pop */
					return false;
				}
				if (!keep_for_read) {
					keep_for_read = true;
					start_of_keep = iter;
				}
			}
		}

		iter = vbuffer_chunk_next(iter);
	}

	assert(iter == current->ctl_iter.chunk);

	/* Init output buffer */
	vbuffer_create_empty(buffer);
	vbuffer_setwritable(buffer, iter->flags.writable);
	end = vbuffer_chunk_end(buffer);

	/* Set output buffer */
	if (keep_for_read) {
		/* Extract chunks from begin to start_of_keep */
		list2_insert_list(&end->list, &begin->list, &start_of_keep->list);

		/* Clone from start_of_keep until push_ctl without marks */
		struct vbuffer_chunk *chunk;


		for (chunk = start_of_keep; chunk != current->ctl_iter.chunk; chunk = vbuffer_chunk_next(chunk)) {
			if (chunk->flags.ctl) {
				continue;
			}

			struct vbuffer_chunk *clone = vbuffer_chunk_clone(chunk, false);
			list2_insert(&end->list, &clone->list);

			chunk->flags.writable = false;
			end->flags.writable = clone->flags.writable;
		}
	} else {
		/* Extract buffer data */
		list2_insert_list(&end->list, &begin->list, list2_next(&iter->list));

		/* Remove push ctl node */
		vbuffer_chunk_remove_ctl(iter);
	}

	/* Update stream->chunks and stream->read_chunks */
	list2_erase(&current->list);
	if (keep_for_read) {
		/* Move current to read_chunks */
		list2_insert(list2_end(&stream->read_chunks), &current->list);
	} else {
		/* Destroy chunk */
		_vbuffer_stream_free_chunk(current);
	}


	return true;
}

struct vbuffer *vbuffer_stream_data(struct vbuffer_stream *stream)
{
	return &stream->data;
}

void vbuffer_stream_current(struct vbuffer_stream *stream, struct vbuffer_iterator *position)
{
	list2_iter iter, end = list2_end(&stream->chunks);
	struct vbuffer_stream_chunk *current;

	if (list2_empty(&stream->read_chunks)) {
		vbuffer_begin(&stream->data, position);
	} else {
		current = list2_last(&stream->read_chunks, struct vbuffer_stream_chunk, list);
		vbuffer_iterator_copy(&current->ctl_iter, position);
	}

	iter = list2_prev(end);
	if (iter == end) return;

	iter = list2_prev(iter);
	if (iter == end) return;

	current = list2_get(iter, struct vbuffer_stream_chunk, list);
	vbuffer_iterator_copy(&current->ctl_iter, position);
}
