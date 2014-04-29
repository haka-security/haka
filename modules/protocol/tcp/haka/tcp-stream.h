/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_PROTO_TCP_STREAM_H
#define _HAKA_PROTO_TCP_STREAM_H

#include <haka/types.h>
#include <haka/container/list2.h>
#include <haka/vbuffer_stream.h>
#include <haka/tcp.h>

/**
 * TCP stream structure.
 */
struct tcp_stream {
	struct lua_object         lua_object;
	uint32                    start_seq;
	uint64                    last_seq;
	uint64                    last_sent_seq;
	int64                     first_offset_seq;
	uint64                    sent_offset_seq;
	struct list2              current;
	struct list2              queued;
	struct list2              sent;
	struct vbuffer_stream     stream;
};

/**
 * Create a new tcp stream.
 */
bool        tcp_stream_create(struct tcp_stream *stream);
void        tcp_stream_clear(struct tcp_stream *stream);

/**
 * Initialize the stream sequence number. This function must be called before
 * starting pushing packet into the stream.
 */
void        tcp_stream_init(struct tcp_stream *stream, uint32 seq);

/**
 * Push data into a tcp stream.
 *
 * \return `true` if successful, `false` otherwise (see clear_error()
 * to get more details about the error).
 */
bool        tcp_stream_push(struct tcp_stream *stream, struct tcp *tcp, struct vbuffer_iterator *current);

/**
 * Pop data from a tcp stream.
 *
 * \return A tcp packet if available. This function will pop all packets that
 * have data before the current position in the stream.
 */
struct tcp *tcp_stream_pop(struct tcp_stream *stream);

/**
 * Offset the ack number of the packet.
 */
void        tcp_stream_ack(struct tcp_stream *stream, struct tcp *tcp);

/**
 * Offset the seq number of the packet.
 */
void        tcp_stream_seq(struct tcp_stream *stream, struct tcp *tcp);

/**
 * Get last sequence number for this tcp stream.
 */
uint32      tcp_stream_lastseq(struct tcp_stream *stream);

#endif /* _HAKA_PROTO_TCP_STREAM_H */
