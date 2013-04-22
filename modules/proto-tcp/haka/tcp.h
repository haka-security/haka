
#ifndef _HAKA_PROTO_TCP_IPV4_H
#define _HAKA_PROTO_TCP_IPV4_H

#include <haka/types.h>
#include <haka/ipv4.h>


struct tcp_header {
};

struct tcp {
	struct ipv4         *packet;
	struct tcp_header   *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
};


struct tcp *tcp_dissect(struct ipv4 *packet);
void tcp_forge(struct tcp *packet);
void tcp_release(struct tcp *packet);


#endif /* _HAKA_PROTO_TCP_IPV4_H */
