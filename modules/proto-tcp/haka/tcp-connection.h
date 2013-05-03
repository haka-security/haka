#include <haka/types.h>
#include <haka/ipv4.h>
#include <haka/tcp.h>

/**
 * TCP connection structure.
 * @ingroup TCP
 */
struct tcp_connection {
	ipv4addr srcip;
	ipv4addr dstip;
	uint16   srcport;
	uint16   dstport;
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
 * @return The TCP connection or NULL if not found.
 */
struct tcp_connection *tcp_connection_get(const struct tcp *tcp);

/**
 * Close the TCP connection.
 */
void tcp_connection_close(struct tcp_connection *tcp_conn);

#define TCP_CONN_GET_FIELD(type, field) \
	static inline type tcp_connection_get_##field(const struct tcp_connection *tcp_conn) { return tcp_conn->field; }

TCP_CONN_GET_FIELD(uint16, srcport);
TCP_CONN_GET_FIELD(uint16, dstport);
TCP_CONN_GET_FIELD(ipv4addr, srcip);
TCP_CONN_GET_FIELD(ipv4addr, dstip);
