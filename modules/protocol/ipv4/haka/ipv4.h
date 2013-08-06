
#ifndef _HAKA_PROTO_IPV4_IPV4_H
#define _HAKA_PROTO_IPV4_IPV4_H

#include <haka/packet.h>
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/error.h>
#include <haka/lua/object.h>

#include "haka/ipv4-addr.h"
#include "haka/ipv4-network.h"

#define SWAP_TO_IPV4(type, x)            SWAP_TO_BE(type, x)
#define SWAP_FROM_IPV4(type, x)          SWAP_FROM_BE(type, x)
#define IPV4_GET_BIT(type, v, i)         GET_BIT(SWAP_FROM_BE(type, v), i)
#define IPV4_SET_BIT(type, v, i, x)      SWAP_TO_BE(type, SET_BIT(SWAP_FROM_BE(type, v), i, x))
#define IPV4_GET_BITS(type, v, r)        GET_BITS(SWAP_FROM_BE(type, v), r)
#define IPV4_SET_BITS(type, v, r, x)     SWAP_TO_BE(type, SET_BITS(SWAP_FROM_BE(type, v), r, x))

#define IPV4_CHECK(ip, ...)              if (!(ip) || !(ip)->packet) { error(L"invalid ipv4 packet"); return __VA_ARGS__; }

#define IPV4_FLAG_RB               15
#define IPV4_FLAG_DF               15-1
#define IPV4_FLAG_MF               15-2
#define IPV4_FLAG_BITS             16-3, 16
#define IPV4_FRAGMENTOFFSET_BITS   0, 16-3
#define IPV4_FRAGMENTOFFSET_OFFSET 3 /* Fragment offset is a multiple of 8 bytes */
#define IPV4_HDR_LEN_OFFSET        2 /* Header length is a multiple of 4 bytes */


struct ipv4_header {
#ifdef HAKA_LITTLEENDIAN
	uint8    hdr_len:4;
	uint8    version:4;
#else
	uint8    version:4;
	uint8    hdr_len:4;
#endif
	uint8    tos;
	uint16   len;
	uint16   id;
	uint16   fragment;
	uint8    ttl;
	uint8    proto;
	uint16   checksum;
	ipv4addr src;
	ipv4addr dst;
};

/* IPv4 opaque structure */
struct ipv4 {
	struct packet       *packet;
	struct lua_object    lua_object;
	struct ipv4_header  *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
	bool                 drop:1;
};

struct ipv4 *ipv4_dissect(struct packet *packet);
struct ipv4 *ipv4_create(struct packet *packet);
struct packet *ipv4_forge(struct ipv4 *ip);
void ipv4_release(struct ipv4 *ip);
int ipv4_pre_modify(struct ipv4 *ip);
int ipv4_pre_modify_header(struct ipv4 *ip);
bool ipv4_verify_checksum(const struct ipv4 *ip);
void ipv4_compute_checksum(struct ipv4 *ip);
int16 inet_checksum(uint16 *ptr, uint16 size);
const uint8 *ipv4_get_payload(struct ipv4 *ip);
uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip);
size_t ipv4_get_payload_length(struct ipv4 *ip);
uint8 *ipv4_resize_payload(struct ipv4 *ip, size_t size);
const char *ipv4_get_proto_dissector(struct ipv4 *ip);
void ipv4_register_proto_dissector(uint8 proto, const char *dissector);
void ipv4_action_drop(struct ipv4 *ip);
void ipv4_action_send(struct ipv4 *ip);
bool ipv4_valid(struct ipv4 *ip);


#define IPV4_GETSET_FIELD(type, field) \
		INLINE type ipv4_get_##field(const struct ipv4 *ip) { IPV4_CHECK(ip, 0); return SWAP_FROM_IPV4(type, ip->header->field); } \
		INLINE void ipv4_set_##field(struct ipv4 *ip, type v) { IPV4_CHECK(ip); if (!ipv4_pre_modify_header(ip)) ip->header->field = SWAP_TO_IPV4(type, v); }

IPV4_GETSET_FIELD(uint8, version);
IPV4_GETSET_FIELD(uint8, tos);
IPV4_GETSET_FIELD(uint16, len);
IPV4_GETSET_FIELD(uint16, id);
IPV4_GETSET_FIELD(uint8, ttl);
IPV4_GETSET_FIELD(uint8, proto);
IPV4_GETSET_FIELD(uint16, checksum);
IPV4_GETSET_FIELD(ipv4addr, src);
IPV4_GETSET_FIELD(ipv4addr, dst);

INLINE uint8 ipv4_get_hdr_len(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return ip->header->hdr_len << IPV4_HDR_LEN_OFFSET;
}

INLINE void ipv4_set_hdr_len(struct ipv4 *ip, uint8 v)
{
	IPV4_CHECK(ip);
	if (!ipv4_pre_modify_header(ip))
		ip->header->hdr_len = v >> IPV4_HDR_LEN_OFFSET;
}

INLINE uint16 ipv4_get_frag_offset(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return (IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS)) << IPV4_FRAGMENTOFFSET_OFFSET;
}

INLINE void ipv4_set_frag_offset(struct ipv4 *ip, uint16 v)
{
	IPV4_CHECK(ip);
	if (!ipv4_pre_modify_header(ip))
		ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS, v >> IPV4_FRAGMENTOFFSET_OFFSET);
}

INLINE uint16 ipv4_get_flags(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS);
}

INLINE void ipv4_set_flags(struct ipv4 *ip, uint16 v)
{
	IPV4_CHECK(ip);
	if (!ipv4_pre_modify_header(ip))
		ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS, v);
}

#define IPV4_GETSET_FLAG(name, flag) \
		INLINE bool ipv4_get_flags_##name(const struct ipv4 *ip) { IPV4_CHECK(ip, 0); return IPV4_GET_BIT(uint16, ip->header->fragment, flag); } \
		INLINE void ipv4_set_flags_##name(struct ipv4 *ip, bool v) { IPV4_CHECK(ip); if (!ipv4_pre_modify_header(ip)) ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, flag, v); }

IPV4_GETSET_FLAG(df, IPV4_FLAG_DF);
IPV4_GETSET_FLAG(mf, IPV4_FLAG_MF);
IPV4_GETSET_FLAG(rb, IPV4_FLAG_RB);

#endif /* _HAKA_PROTO_IPV4_IPV4_H */
