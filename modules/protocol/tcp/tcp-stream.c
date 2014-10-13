/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/tcp-stream.h>
#include <haka/tcp.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/container/list.h>
#include <haka/container/vector.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


static REGISTER_LOG_SECTION(tcp);

enum tcp_modif_type {
	TCP_MODIF_INSERT,
	TCP_MODIF_ERASE
};

struct tcp_stream;

struct tcp_stream_chunk_modif {
	enum tcp_modif_type            type;
	uint64                         position;
	size_t                         length;
};

struct tcp_stream_chunk {
	struct list2_elem              list;
	struct tcp                    *tcp;
	uint64                         start_seq;
	uint64                         end_seq;
	int64                          offset_seq;
	struct vector                  modifs;
};

bool tcp_stream_create(struct tcp_stream *stream)
{
	memset(stream, 0, sizeof(struct tcp_stream));

	if (!vbuffer_stream_init(&stream->stream, NULL)) {
		return false;
	}

	stream->lua_object = lua_object_init;
	list2_init(&stream->current);
	list2_init(&stream->queued);
	list2_init(&stream->sent);
	stream->start_seq = (uint32)-1;
	return true;
}

static void tcp_stream_chunk_free(struct tcp_stream_chunk *chunk)
{
	if (chunk->tcp) {
		tcp_release(chunk->tcp);
		chunk->tcp = NULL;
	}

	vector_destroy(&chunk->modifs);
	free(chunk);
}

static void tcp_stream_free_chunks(struct list2 *list)
{
	list2_iter iter = list2_begin(list);
	const list2_iter end = list2_end(list);

	while (iter != end) {
		struct tcp_stream_chunk *chunk = list2_get(iter, struct tcp_stream_chunk, list);
		iter = list2_erase(iter);
		tcp_stream_chunk_free(chunk);
	}
}

void tcp_stream_clear(struct tcp_stream *stream)
{
	tcp_stream_free_chunks(&stream->current);
	tcp_stream_free_chunks(&stream->queued);
	tcp_stream_free_chunks(&stream->sent);

	lua_object_release(stream, &stream->lua_object);
	vbuffer_stream_clear(&stream->stream);
}

void tcp_stream_init(struct tcp_stream *stream, uint32 seq)
{
	stream->start_seq = seq;
	stream->last_seq = 0;
	stream->last_sent_seq = 0;
}

static uint64 tcp_remap_seq(uint32 seq, uint64 ref)
{
	uint64 ret = seq + ((ref>>32)<<32);

	if (ret + (1LL<<31) < ref) {
		ret += (1ULL<<32);
	}

	return ret;
}

bool tcp_stream_push(struct tcp_stream *stream, struct tcp *tcp, struct vbuffer_iterator *current)
{
	struct tcp_stream_chunk *chunk;
	uint64 ref_seq;

	if (stream->start_seq == (size_t)-1) {
		error("uninitialized stream");
		return false;
	}

	chunk = malloc(sizeof(struct tcp_stream_chunk));
	if (!chunk) {
		error("memory error");
		return false;
	}

	chunk->tcp = tcp;
	list2_elem_init(&chunk->list);
	vector_create(&chunk->modifs, struct tcp_stream_chunk_modif, NULL);

	ref_seq = stream->last_sent_seq + stream->start_seq;

	chunk->start_seq = tcp_remap_seq(tcp_get_seq(tcp), ref_seq);
	chunk->start_seq -= stream->start_seq;
	chunk->end_seq = chunk->start_seq + tcp_get_payload_length(tcp);
	chunk->offset_seq = 0;

	if (current) {
		// Default, set current to end of stream
		vbuffer_end(&stream->stream.data, current);
	}

	if (stream->last_seq == chunk->start_seq) {
		list2_insert(list2_end(&stream->current), &chunk->list);
		vbuffer_stream_push(&stream->stream, &tcp->payload, NULL, current);
		stream->last_seq = chunk->end_seq;

		/* Check for queued packets */
		{
			list2_iter iter = list2_begin(&stream->queued);
			const list2_iter end = list2_end(&stream->queued);

			while (iter != end) {
				struct tcp_stream_chunk *qchunk = list2_get(iter, struct tcp_stream_chunk, list);

				if (qchunk->start_seq != stream->last_seq) {
					break;
				}

				iter = list2_erase(iter);

				list2_insert(list2_end(&stream->current), &qchunk->list);
				vbuffer_stream_push(&stream->stream, &qchunk->tcp->payload, NULL, NULL);
				stream->last_seq = qchunk->end_seq;
			}
		}
	}
	else if (chunk->start_seq > stream->last_seq) {
		/* Search for insert point */
		list2_iter iter = list2_begin(&stream->queued);
		const list2_iter end = list2_end(&stream->queued);
		struct tcp_stream_chunk *qchunk = NULL;

		while (iter != end) {
			qchunk = list2_get(iter, struct tcp_stream_chunk, list);
			if (qchunk->start_seq > chunk->start_seq) break;
			iter = list2_next(iter);
		}

		if (iter != end && chunk->end_seq > qchunk->start_seq) {
			LOG_WARNING(tcp, "retransmit packet (ignored)");
			tcp_stream_chunk_free(chunk);
			return false;
		}

		list2_insert(iter, &chunk->list);
	}
	else {
		LOG_WARNING(tcp, "retransmit packet (ignored)");
		tcp_stream_chunk_free(chunk);
		return false;
	}

	return true;
}

struct tcp *tcp_stream_pop(struct tcp_stream *stream)
{
	struct tcp *tcp;
	struct tcp_stream_chunk *chunk;
	struct vbuffer available;

	if (list2_empty(&stream->current)) {
		// TODO: we should generate a new forged packet in this case
		// if the vbuffer_stream contains some data
		return NULL;
	}

	if (!vbuffer_stream_pop(&stream->stream, &available, NULL)) {
		/* No data available yet */
		return NULL;
	}

	chunk = list2_first(&stream->current, struct tcp_stream_chunk, list);
	list2_erase(&chunk->list);

	tcp = chunk->tcp;
	chunk->tcp = NULL;
	assert(tcp);
	assert(!vbuffer_isvalid(&tcp->payload));

	assert(!list2_last(&stream->sent, struct tcp_stream_chunk, list) ||
	       chunk->start_seq == list2_last(&stream->sent, struct tcp_stream_chunk, list)->end_seq);
	list2_insert(list2_end(&stream->sent), &chunk->list);

	chunk->offset_seq = ((int64)vbuffer_size(&available)) - (chunk->end_seq - chunk->start_seq);

	/* TODO: Compute modifications lists if we want to be able to
	 * correctly offset seq number that are not aligned with a packet
	 * boundary */

	/* Update tcp packet */
	vbuffer_swap(&tcp->payload, &available);
	vbuffer_release(&available);

	tcp_stream_seq(stream, tcp);

	stream->last_sent_seq = chunk->end_seq;
	stream->first_offset_seq += chunk->offset_seq;

	if (chunk->offset_seq < 0 && chunk->offset_seq == (chunk->start_seq - chunk->end_seq)) {
		tcp_action_drop(tcp);
	}

	return tcp;
}

void tcp_stream_ack(struct tcp_stream *stream, struct tcp *tcp)
{
	uint64 ack, seq, new_seq;
	struct tcp_stream_chunk *chunk;
	list2_iter iter, end;

	if (tcp->packet == NULL) {
		return;
	}

	ack = tcp_get_ack_seq(tcp);

	iter = list2_begin(&stream->sent);
	end = list2_end(&stream->sent);

	if (iter == end) {
		return;
	}

	chunk = list2_get(iter, struct tcp_stream_chunk, list);
	seq = stream->sent_offset_seq + chunk->start_seq;
	ack = tcp_remap_seq(ack, seq);
	ack -= stream->start_seq;

	new_seq = chunk->start_seq;

	while (iter != end) {
		chunk = list2_get(iter, struct tcp_stream_chunk, list);

		if (seq + (chunk->end_seq - chunk->start_seq) + chunk->offset_seq > ack) {
			break;
		}

		seq += (chunk->end_seq - chunk->start_seq) + chunk->offset_seq;
		new_seq = chunk->end_seq;
		if (ack <= seq) {
			break;
		}

		assert(list2_next(iter) == end ||
		       list2_get(list2_next(iter), struct tcp_stream_chunk, list)->start_seq == chunk->end_seq);

		iter = list2_next(iter);
	}

	/* Find the exact ack position taking into account the modifs
	 * TODO: Not sure this is really needed. */
	/*if (chunk)
	{
		const int count = vector_count(&chunk->modifs);
		int i, offset = 0;

		for (i=0; i<count; ++i) {
			struct tcp_stream_chunk_modif *modif = vector_get(&chunk->modifs, struct tcp_stream_chunk_modif, i);

			if (seq + (modif->position - offset) >= ack) {
				break;
			}
			else {
				new_seq = iter->start_seq + modif->position;
				seq += modif->position - offset;
				offset = modif->position;

				switch (modif->type) {
				case TCP_MODIF_INSERT:
					if (seq + modif->length > ack) {
						seq = ack;
					}
					else {
						seq += modif->length;
					}
					break;

				case TCP_MODIF_ERASE:
					new_seq += modif->length;
					offset += modif->length;
					break;

				default:
					assert(0);
					break;
				}
			}
		}
	}*/

	/* Need to account for offset (case of the FIN for instance) */
	new_seq += ack-seq;
	new_seq += stream->start_seq;

	if (tcp_get_ack_seq(tcp) != new_seq) {
		tcp_set_ack_seq(tcp, new_seq);
	}
}

void tcp_stream_seq(struct tcp_stream *stream, struct tcp *tcp)
{
	const uint32 ack = tcp_get_seq(tcp) + stream->first_offset_seq;
	if (ack != tcp_get_seq(tcp)) {
		tcp_set_seq(tcp, ack);
	}
}

uint32 tcp_stream_lastseq(struct tcp_stream *stream)
{
	return stream->start_seq + stream->last_sent_seq + stream->first_offset_seq;
}
