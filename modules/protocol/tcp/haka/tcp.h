
#ifndef _HAKA_PROTO_TCP_H
#define _HAKA_PROTO_TCP_H

#include <haka/types.h>
#include <haka/ipv4.h>
#include <haka/lua/object.h>

#define SWAP_TO_TCP(type, x)            SWAP_TO_BE(type, x)
#define SWAP_FROM_TCP(type, x)          SWAP_FROM_BE(type, x)
#define TCP_GET_BIT(type, v, i)         GET_BIT(SWAP_FROM_BE(type, v), i)
#define TCP_SET_BIT(type, v, i, x)      SWAP_TO_BE(type, SET_BIT(SWAP_FROM_BE(type, v), i, x))
#define TCP_GET_BITS(type, v, r)        GET_BITS(SWAP_FROM_BE(type, v), r)
#define TCP_SET_BITS(type, v, r, x)     SWAP_TO_BE(type, SET_BITS(SWAP_FROM_BE(type, v), r, x))

#define TCP_CHECK(tcp, ...)             if (!(tcp) || !(tcp)->packet) { error(L"invalid tcp packet"); return __VA_ARGS__; }

#define TCP_FLAGS_BITS                  0, 8
#define TCP_FLAGS_START                 13
#define TCP_HDR_LEN                     2 /* Header length is a multiple of 4 bytes */

#define TCP_PROTO 6

struct tcp_header {
	uint16    srcport;
	uint16    dstport;
	uint32	  seq;
	uint32	  ack_seq;
#ifdef HAKA_LITTLEENDIAN
	uint16    res:4,
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
	uint16	  hdr_len:4,
	          res:4,
	          cwr:1,
	          ecn:1,
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


/* TCP opaque structure */
struct tcp {
	struct ipv4         *packet;
	struct lua_object    lua_object;
	struct tcp_header   *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
};

struct tcp *tcp_dissect(struct ipv4 *packet);
struct tcp *tcp_create(struct ipv4 *packet);
struct ipv4 *tcp_forge(struct tcp *packet);
void tcp_release(struct tcp *packet);
int tcp_pre_modify(struct tcp *packet);
void tcp_compute_checksum(struct tcp *packet);
bool tcp_verify_checksum(const struct tcp *packet);
const uint8 *tcp_get_payload(const struct tcp *packet);
uint8 *tcp_get_payload_modifiable(struct tcp *packet);
size_t tcp_get_payload_length(const struct tcp *packet);
uint8 *tcp_resize_payload(struct tcp *packet, size_t size);
void tcp_action_drop(struct tcp *packet);
void tcp_action_send(struct tcp *packet);
bool tcp_valid(struct tcp *packet);


#define TCP_GETSET_FIELD(type, field) \
	INLINE type tcp_get_##field(const struct tcp *tcp) { TCP_CHECK(tcp, 0); return SWAP_FROM_TCP(type, tcp->header->field); } \
	INLINE void tcp_set_##field(struct tcp *tcp, type v) { TCP_CHECK(tcp); if (!tcp_pre_modify(tcp)) tcp->header->field = SWAP_TO_TCP(type, v); }

TCP_GETSET_FIELD(uint16, srcport);
TCP_GETSET_FIELD(uint16, dstport);
TCP_GETSET_FIELD(uint32, seq);
TCP_GETSET_FIELD(uint32, ack_seq);
TCP_GETSET_FIELD(uint8, res);
TCP_GETSET_FIELD(uint16, window_size);
TCP_GETSET_FIELD(uint16, urgent_pointer);
TCP_GETSET_FIELD(uint16, checksum);

INLINE uint8 tcp_get_hdr_len(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return tcp->header->hdr_len << TCP_HDR_LEN;
}

INLINE void tcp_set_hdr_len(struct tcp *tcp, uint8 v)
{
	TCP_CHECK(tcp);
	if (!tcp_pre_modify(tcp))
		tcp->header->hdr_len = v >> TCP_HDR_LEN;
}

INLINE uint16 tcp_get_flags(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return TCP_GET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS);
}

INLINE void tcp_set_flags(struct tcp *tcp, uint8 v)
{
	TCP_CHECK(tcp);
	if (!tcp_pre_modify(tcp))
		*(((uint8 *)tcp->header) + TCP_FLAGS_START) = TCP_SET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS, v);
}


#define TCP_GETSET_FLAG(name) \
	INLINE bool tcp_get_flags_##name(const struct tcp *tcp) { TCP_CHECK(tcp, 0); return tcp->header->name; } \
	INLINE void tcp_set_flags_##name(struct tcp *tcp, bool v) { TCP_CHECK(tcp); if (!tcp_pre_modify(tcp)) tcp->header->name = v; }

TCP_GETSET_FLAG(fin);
TCP_GETSET_FLAG(syn);
TCP_GETSET_FLAG(rst);
TCP_GETSET_FLAG(psh);
TCP_GETSET_FLAG(ack);
TCP_GETSET_FLAG(urg);
TCP_GETSET_FLAG(ecn);
TCP_GETSET_FLAG(cwr);

#endif /* _HAKA_PROTO_TCP_H */
