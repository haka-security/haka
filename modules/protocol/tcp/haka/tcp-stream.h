/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_PROTO_TCP_STREAM_H
#define _HAKA_PROTO_TCP_STREAM_H

#include <haka/types.h>
#include <haka/stream.h>
#include <haka/tcp.h>

struct stream *tcp_stream_create();
bool tcp_stream_push(struct stream *stream, struct tcp *tcp);
struct tcp *tcp_stream_pop(struct stream *stream);
void tcp_stream_init(struct stream *stream, uint32 seq);
void tcp_stream_ack(struct stream *stream, struct tcp *tcp);
void tcp_stream_seq(struct stream *stream, struct tcp *tcp);
uint32 tcp_stream_lastseq(struct stream *stream);

#endif /* _HAKA_PROTO_TCP_STREAM_H */
