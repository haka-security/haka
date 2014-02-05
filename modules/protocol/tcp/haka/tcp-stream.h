/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_PROTO_TCP_STREAM_H
#define _HAKA_PROTO_TCP_STREAM_H

#include <haka/types.h>
#include <haka/vbuffer.h>
#include <haka/tcp.h>

struct tcp_stream_chunk;

struct tcp_stream {
	struct lua_object         lua_object;
	uint32                    start_seq;
	uint64                    last_seq;
	uint64                    last_sent_seq;
	int64                     first_offset_seq;
	uint64                    sent_offset_seq;
	struct tcp_stream_chunk  *first;
	struct tcp_stream_chunk  *last;
	struct tcp_stream_chunk  *queued_first;
	struct tcp_stream_chunk  *queued_last;
	struct tcp_stream_chunk  *sent_first;
	struct tcp_stream_chunk  *sent_last;
	struct vbuffer_stream    *stream;
};

struct tcp_stream *tcp_stream_create();
void               tcp_stream_free(struct tcp_stream *stream);
void               tcp_stream_init(struct tcp_stream *stream, uint32 seq);
bool               tcp_stream_push(struct tcp_stream *stream, struct tcp *tcp);
struct tcp        *tcp_stream_pop(struct tcp_stream *stream);
void               tcp_stream_ack(struct tcp_stream *stream, struct tcp *tcp);
void               tcp_stream_seq(struct tcp_stream *stream, struct tcp *tcp);
uint32             tcp_stream_lastseq(struct tcp_stream *stream);

#endif /* _HAKA_PROTO_TCP_STREAM_H */
