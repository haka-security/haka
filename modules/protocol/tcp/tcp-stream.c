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


enum tcp_modif_type {
	TCP_MODIF_INSERT,
	TCP_MODIF_ERASE
};

struct tcp_stream;

struct tcp_stream_chunk_modif {
	enum tcp_modif_type             type;
	uint64                          position;
	size_t                          length;
};

struct tcp_stream_chunk {
	struct list                    list;
	struct tcp                    *tcp;
	uint64                         start_seq;
	uint64                         end_seq;
	int64                          offset_seq;
	struct vector                  modifs;
};

struct tcp_stream *tcp_stream_create()
{
	struct tcp_stream *stream = malloc(sizeof(struct tcp_stream));
	if (!stream) {
		error(L"memory error");
		return NULL;
	}

	memset(stream, 0, sizeof(struct tcp_stream));

	stream->stream = vbuffer_stream();
	if (!stream->stream) {
		free(stream);
		return NULL;
	}

	lua_object_init(&stream->lua_object);
	stream->start_seq = (uint32)-1;

	return stream;
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

static void tcp_stream_free_chunks(struct tcp_stream_chunk *chunks)
{
	while (chunks) {
		struct tcp_stream_chunk *next = list_next(chunks);
		tcp_stream_chunk_free(chunks);
		chunks = next;
	}
}

void tcp_stream_free(struct tcp_stream *stream)
{
	tcp_stream_free_chunks(stream->first);
	tcp_stream_free_chunks(stream->queued_first);
	tcp_stream_free_chunks(stream->sent_first);

	lua_object_release(stream, &stream->lua_object);
	vbuffer_stream_free(stream->stream);
	free(stream);
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

bool tcp_stream_push(struct tcp_stream *stream, struct tcp *tcp)
{
	struct tcp_stream_chunk *chunk;
	uint64 ref_seq;

	if (stream->start_seq == (size_t)-1) {
		error(L"uninitialized stream");
		return false;
	}

	chunk = malloc(sizeof(struct tcp_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	chunk->tcp = tcp;
	list_init(chunk);
	vector_create(&chunk->modifs, struct tcp_stream_chunk_modif, NULL);

	ref_seq = stream->last_sent_seq + stream->start_seq;

	chunk->start_seq = tcp_remap_seq(tcp_get_seq(tcp), ref_seq);
	chunk->start_seq -= stream->start_seq;
	chunk->end_seq = chunk->start_seq + tcp_get_payload_length(tcp);
	chunk->offset_seq = 0;

	if (stream->last_seq == chunk->start_seq) {
		struct tcp_stream_chunk *iter = stream->queued_first;

		list_insert_after(chunk, stream->last, &stream->first, &stream->last);
		vbuffer_stream_push(stream->stream, tcp->payload);
		tcp->payload = NULL;
		stream->last_seq = chunk->end_seq;

		/* Check for queued packets */
		while (iter && iter->start_seq == stream->last_seq) {
			struct tcp_stream_chunk *next = list_next(iter);
			list_remove(iter, &stream->queued_first, &stream->queued_last);

			list_insert_after(iter, stream->last, &stream->first, &stream->last);
			vbuffer_stream_push(stream->stream, iter->tcp->payload);
			iter->tcp->payload = NULL;
			stream->last_seq = chunk->end_seq;

			iter = next;
		}
	}
	else if (chunk->start_seq > stream->last_seq) {
		/* Search for insert point */
		struct tcp_stream_chunk *iter = stream->queued_first;

		while (iter && iter->start_seq <= chunk->start_seq) {
			iter = list_next(iter);
		}

		if (iter && chunk->end_seq > iter->start_seq) {
			message(HAKA_LOG_WARNING, L"tcp-connection", L"retransmit packet (ignored)");
			tcp_stream_chunk_free(chunk);
			return false;
		}

		list_insert_before(chunk, iter, &stream->queued_first, &stream->queued_last);
	}
	else {
		message(HAKA_LOG_WARNING, L"tcp-connection", L"retransmit packet (ignored)");
		tcp_stream_chunk_free(chunk);
		return false;
	}

	return true;
}

struct tcp *tcp_stream_pop(struct tcp_stream *stream)
{
	struct tcp *tcp;
	struct tcp_stream_chunk *chunk;
	struct vbuffer *available;

	if (!stream->first) {
		// TODO: we should generate a new forged packet in this case
		// if the vbuffer_stream contains some data
		return NULL;
	}

	available = vbuffer_stream_pop(stream->stream);
	if (!available) {
		/* No data available yet */
		return NULL;
	}

	chunk = stream->first;
	tcp = chunk->tcp;
	assert(tcp);
	assert(!tcp->payload);

	list_remove(chunk, &stream->first, &stream->last);
	chunk->tcp = NULL;
	assert(!stream->sent_last || chunk->start_seq == stream->sent_last->end_seq);
	list_insert_after(chunk, stream->sent_last, &stream->sent_first, &stream->sent_last);

	chunk->offset_seq = ((int64)vbuffer_size(available)) - (chunk->end_seq - chunk->start_seq);

	/* TODO: Compute modifications lists if we want to be able to
	 * correctly offset seq number that are not aligned with a packet
	 * boundary */

	/* Update tcp packet */
	tcp->payload = available;
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
	uint64 ack = tcp_get_ack_seq(tcp);
	uint64 seq, new_seq;
	struct tcp_stream_chunk *iter;

	iter = stream->sent_first;
	if (!iter) {
		return;
	}

	seq = stream->sent_offset_seq + iter->start_seq;
	ack = tcp_remap_seq(ack, seq);
	ack -= stream->start_seq;

	new_seq = iter->start_seq;

	while (iter) {
		if (seq + (iter->end_seq - iter->start_seq) + iter->offset_seq > ack) {
			break;
		}

		seq += (iter->end_seq - iter->start_seq) + iter->offset_seq;
		new_seq = iter->end_seq;
		if (ack <= seq) {
			break;
		}

		assert(!list_next(iter) || list_next(iter)->start_seq == iter->end_seq);

		iter = list_next(iter);
	}

	/* Find the exact ack position taking into account the modifs
	 * TODO: Not sure this is really needed. */
	/*if (iter)
	{
		const int count = vector_count(&iter->modifs);
		int i, offset = 0;

		for (i=0; i<count; ++i) {
			struct tcp_stream_chunk_modif *modif = vector_get(&iter->modifs, struct tcp_stream_chunk_modif, i);

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
