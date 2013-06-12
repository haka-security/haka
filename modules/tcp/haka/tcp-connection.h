/**
 * file tcp-connection.h
 * @brief  TCP Connection
 * @author Pierre-Sylvain Desse
 *
 * The file contains the TCP connection API functions and structures definition
 */

#include <haka/types.h>
#include <haka/stream.h>
#include <haka/ipv4.h>
#include <haka/tcp.h>
#include <haka/lua/ref.h>


/**
 * TCP connection structure.
 * @ingroup TCP
 */
struct tcp_connection {
	ipv4addr       srcip;
	ipv4addr       dstip;
	uint16         srcport;
	uint16         dstport;
	struct lua_ref lua_table;
	struct stream *stream_input;
	struct stream *stream_output;
};

/**
 * Create a new TCP connection for the given TCP packet.
 * @param tcp TCP packet going from the client to the server.
 * @return The new created TCP connection.
 */
struct tcp_connection *tcp_connection_new(const struct tcp *tcp);

/**
 * Get the TCP connection if any associated with the given TCP packet.
 * @param tcp TCP packet.
 * @param directionç_in Filled with the direction of the given tcp packet. It
 * is set to true if the packet follow the input direction, false otherwise.
 * @return The TCP connection or NULL if not found.
 */
struct tcp_connection *tcp_connection_get(const struct tcp *tcp, bool *direction_in);

/**
 * Get the stream associated with a TCP connection.
 * @param conn TCP connection.
 * @param direction_in Stream direction, input if true, output otherwise.
 * @return The stream for the given direction.
 */
static inline struct stream *tcp_connection_get_stream(struct tcp_connection *conn, bool direction_in)
{
	if (direction_in) return conn->stream_input;
	else return conn->stream_output;
}

/**
 * Close the TCP connection.
 * @param tcp_conn TCP connection.
 */
void tcp_connection_close(struct tcp_connection *tcp_conn);

#define TCP_CONN_GET_FIELD(type, field) \
	INLINE type tcp_connection_get_##field(const struct tcp_connection *tcp_conn) { return tcp_conn->field; }

TCP_CONN_GET_FIELD(uint16, srcport);
TCP_CONN_GET_FIELD(uint16, dstport);
TCP_CONN_GET_FIELD(ipv4addr, srcip);
TCP_CONN_GET_FIELD(ipv4addr, dstip);
