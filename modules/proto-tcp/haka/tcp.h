/**
 * file tcp.h
 * @brief  TCP Dissector API
 * @author Mehdi Talbi
 *
 * The file contains the TCP disscetor API functions and structures definition
 */

/**
 * @defgroup TCP TCP
 * @brief TCP dissector API functions and structures.
 * @ingroup ExternProtocolModule
 */

#ifndef _HAKA_PROTO_TCP_IPV4_H
#define _HAKA_PROTO_TCP_IPV4_H

#include <haka/types.h>
#include <haka/ipv4.h>

#define SWAP_TO_TCP(type, x)            SWAP_TO_BE(type, x)
#define SWAP_FROM_TCP(type, x)          SWAP_FROM_BE(type, x)
#define TCP_GET_BIT(type, v, i)         GET_BIT(SWAP_FROM_BE(type, v), i)
#define TCP_SET_BIT(type, v, i, x)      SWAP_TO_BE(type, SET_BIT(SWAP_FROM_BE(type, v), i, x))
#define TCP_GET_BITS(type, v, r)        GET_BITS(SWAP_FROM_BE(type, v), r)
#define TCP_SET_BITS(type, v, r, x)     SWAP_TO_BE(type, SET_BITS(SWAP_FROM_BE(type, v), r, x))

#define TCP_FLAGS_BITS                  0, 8
#define TCP_FLAGS_START                 13

struct tcp_header {
	uint16    srcport;
	uint16    dstport;
	uint32	  seq;
	uint32	  ack_seq;
#ifdef HAKA_LITTLEENDIAN
	uint16	  res:4,
			  hdr_len:4,
			  fin:1,
		  	  syn:1,
			  rst:1,
			  psh:1,
		      ack:1,
			  urg:1,
			  ecn:1,
			  cwr:1;
#else
	unint16	  hdr_len:4,
			  res:4,
			  cwr:1,
			  ece:1,
			  urg:1,
			  ack:1,
			  psh:1,
			  rst:1,
			  syn:1,
			  fin:1;
#endif
	uint16	  window_size;
	uint16	  checksum;
	uint16	  urgent_pointer;
};

struct tcp_pseudo_header {
	ipv4addr       src;
	ipv4addr       dst;
	uint8          reserved;
	uint8          proto;
	uint16         len;
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

void tcp_pre_modify(struct tcp *packet);

void tcp_pre_modify_header(struct tcp *packet);

void tcp_compute_checksum(struct tcp *packet);

bool tcp_verify_checksum(const struct tcp *packet);

uint16 tcp_get_length(struct ipv4 *packet);

#define TCP_GETSET_FIELD(type, field) \
        static inline type tcp_get_##field(const struct tcp *tcp) { return SWAP_FROM_TCP(type, tcp->header->field); } \
        static inline void tcp_set_##field(struct tcp *tcp, type v) { tcp_pre_modify_header(tcp); tcp->header->field = SWAP_TO_TCP(type, v); }

TCP_GETSET_FIELD(uint16, srcport);
TCP_GETSET_FIELD(uint16, dstport);
TCP_GETSET_FIELD(uint32, seq);
TCP_GETSET_FIELD(uint32, ack_seq);
TCP_GETSET_FIELD(uint8, hdr_len);
TCP_GETSET_FIELD(uint8, res);
TCP_GETSET_FIELD(uint16, window_size);
TCP_GETSET_FIELD(uint16, checksum);
TCP_GETSET_FIELD(uint16, urgent_pointer);

static inline uint16 tcp_get_flags(const struct tcp *tcp)
{
	return TCP_GET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS);
}

static inline void tcp_set_flags(struct tcp *tcp, uint8 v)
{
	tcp_pre_modify_header(tcp);
	tcp->header->fin = TCP_SET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS, v);
}


#define TCP_GETSET_FLAG(name) \
        static inline bool tcp_get_flags_##name(const struct tcp *tcp) { return tcp->header->name; } \
        static inline void tcp_set_flags_##name(struct tcp *tcp, bool value) { tcp_pre_modify_header(tcp); tcp->header->name = value; }

TCP_GETSET_FLAG(fin);
TCP_GETSET_FLAG(syn);
TCP_GETSET_FLAG(rst);
TCP_GETSET_FLAG(psh);
TCP_GETSET_FLAG(ack);
TCP_GETSET_FLAG(urg);
TCP_GETSET_FLAG(ecn);
TCP_GETSET_FLAG(cwr);

#endif /* _HAKA_PROTO_TCP_IPV4_H */
