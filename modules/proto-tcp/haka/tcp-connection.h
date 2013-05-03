#include <haka/types.h>
#include <haka/ipv4.h>
#include <haka/tcp.h>

struct tcp_connection {
	ipv4addr srcip;
    ipv4addr dstip;
    uint16   srcport;
    uint16   dstport;
};

struct tcp_connection *tcp_connection_new(const struct tcp *tcp);

struct tcp_connection *tcp_connection_get(const struct tcp *tcp);

void tcp_connection_close(struct tcp_connection* tcp_conn);

#define TCP_CONN_GET_FIELD(type, field) \
        static inline type tcp_connection_get_##field(const struct tcp_connection *tcp_conn) { return tcp_conn->field; }

TCP_CONN_GET_FIELD(uint16, srcport);
TCP_CONN_GET_FIELD(uint16, dstport);
TCP_CONN_GET_FIELD(ipv4addr, srcip);
TCP_CONN_GET_FIELD(ipv4addr, dstip);


