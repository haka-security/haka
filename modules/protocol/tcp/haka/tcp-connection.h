/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/types.h>
#include <haka/ipv4.h>
#include <haka/tcp.h>
#include <haka/lua/ref.h>
#include <haka/lua/object.h>


struct tcp_stream;

struct tcp_connection {
	struct lua_object    lua_object;
	ipv4addr             srcip;
	ipv4addr             dstip;
	uint16               srcport;
	uint16               dstport;
	struct lua_ref       lua_table;
	struct tcp_stream   *stream_input;
	struct tcp_stream   *stream_output;
};

struct tcp_connection *tcp_connection_new(struct tcp *tcp);
struct tcp_connection *tcp_connection_get(struct tcp *tcp, bool *direction_in, bool *dropped);

INLINE struct tcp_stream *tcp_connection_get_stream(struct tcp_connection *conn, bool direction_in)
{
	if (direction_in) return conn->stream_input;
	else return conn->stream_output;
}

void tcp_connection_close(struct tcp_connection *tcp_conn);
void tcp_connection_drop(struct tcp_connection *tcp_conn);


#define TCP_CONN_GET_FIELD(type, field) \
	INLINE type tcp_connection_get_##field(const struct tcp_connection *tcp_conn) { return tcp_conn->field; }

TCP_CONN_GET_FIELD(uint16, srcport);
TCP_CONN_GET_FIELD(uint16, dstport);
TCP_CONN_GET_FIELD(ipv4addr, srcip);
TCP_CONN_GET_FIELD(ipv4addr, dstip);
