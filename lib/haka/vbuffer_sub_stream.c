/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <haka/vbuffer_sub_stream.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/thread.h>

#include "vbuffer.h"
#include "vbuffer_data.h"


struct vbuffer_sub_stream_chunk {
	struct vbuffer_iterator     select;
};

void vbuffer_sub_stream_chunk_free(void *_data)
{
	struct vbuffer_sub_stream_chunk *data = (struct vbuffer_sub_stream_chunk *)_data;

	vbuffer_restore(&data->select, NULL);
	free(data);
}

bool vbuffer_sub_stream_init(struct vbuffer_sub_stream *stream)
{
	return vbuffer_stream_init(&stream->stream, vbuffer_sub_stream_chunk_free);
}

bool vbuffer_sub_stream_push(struct vbuffer_sub_stream *stream, struct vbuffer_sub *buffer)
{
	struct vbuffer extract;
	struct vbuffer_sub_stream_chunk *chunk = malloc(sizeof(struct vbuffer_sub_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	if (!vbuffer_select(buffer, &extract, &chunk->select)) {
		free(chunk);
		return false;
	}

	return vbuffer_stream_push(&stream->stream, &extract, chunk);
}

bool vbuffer_sub_stream_pop(struct vbuffer_sub_stream *stream, struct vbuffer_sub *sub)
{
	struct vbuffer buffer;
	struct vbuffer_sub_stream_chunk *chunk;
	bool ret = false;

	if (sub) {
		vbuffer_sub_init(sub);
	}

	while (vbuffer_stream_pop(&stream->stream, &buffer, (void **)&chunk)) {
		if (sub) {
			if (!ret && !vbuffer_isempty(&buffer)) {
				vbuffer_begin(&buffer, &sub->begin);
				sub->use_size = false;
			}
			vbuffer_last(&buffer, &sub->end);
		}

		if (!vbuffer_isempty(&buffer)) {
			ret = true;
		}

		vbuffer_restore(&chunk->select, &buffer);
		vbuffer_release(&buffer);
		free(chunk);
	}

	return ret;
}
