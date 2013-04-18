
#ifndef _HAKA_PROTO_IPV4_IPV4_H
#define _HAKA_PROTO_IPV4_IPV4_H

#include <haka/packet.h>
#include <haka/types.h>

#define SWAP_TO_IPV4(type, x)            SWAP_TO_BE(type, x)
#define SWAP_FROM_IPV4(type, x)          SWAP_FROM_BE(type, x)
#define IPV4_GET_BIT(type, v, i)         GET_BIT(SWAP_FROM_BE(type, v), i)
#define IPV4_SET_BIT(type, v, i, x)      SWAP_TO_BE(type, SET_BIT(SWAP_FROM_BE(type, v), i, x))
#define IPV4_GET_BITS(type, v, r)        GET_BITS(SWAP_FROM_BE(type, v), r)
#define IPV4_SET_BITS(type, v, r, x)     SWAP_TO_BE(type, SET_BITS(SWAP_FROM_BE(type, v), r, x))

#define IPV4_FLAG_RB              15
#define IPV4_FLAG_DF              15-1
#define IPV4_FLAG_MF              15-2
#define IPV4_FLAG_BITS            16-3, 16
#define IPV4_FRAGMENTOFFSET_BITS  0, 16-3

typedef uint32 ipv4addr;
#define SWAP_ipv4addr(x) SWAP_uint32(x)

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

struct ipv4 {
	struct packet       *packet;
	struct ipv4_header  *header;
	bool                 modified:1;
};

struct ipv4 *create(struct packet* packet);
void release(struct ipv4 *ip);

#define IPV4_GETSET_FIELD(type, field) \
		static inline type ipv4_get_##field(const struct ipv4 *ip) { return SWAP_FROM_IPV4(type, ip->header->field); } \
		static inline void ipv4_set_##field(struct ipv4 *ip, type v) { ip->modified = true; ip->header->field = SWAP_TO_IPV4(type, v); }

IPV4_GETSET_FIELD(uint8, hdr_len);
IPV4_GETSET_FIELD(uint8, version);
IPV4_GETSET_FIELD(uint8, tos);
IPV4_GETSET_FIELD(uint16, len);
IPV4_GETSET_FIELD(uint16, id);
IPV4_GETSET_FIELD(uint8, ttl);
IPV4_GETSET_FIELD(uint8, proto);
IPV4_GETSET_FIELD(uint16, checksum);
IPV4_GETSET_FIELD(ipv4addr, src);
IPV4_GETSET_FIELD(ipv4addr, dst);

static inline uint16 ipv4_get_frag_offset(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS);
}

static inline void ipv4_set_frag_offset(struct ipv4 *ip, uint16 v)
{
	ip->modified = true;
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS, v);
}

static inline uint16 ipv4_get_flags(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS);
}

static inline void ipv4_set_flags(struct ipv4 *ip, uint16 v)
{
	ip->modified = true;
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS, v);
}

#define IPV4_GETSET_FLAG(name, flag) \
		static inline bool ipv4_get_flags_##name(const struct ipv4 *ip) { return IPV4_GET_BIT(uint16, ip->header->fragment, flag); } \
		static inline void ipv4_set_flags_##name(struct ipv4 *ip, bool value) { ip->modified = true; ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, flag, value); }

IPV4_GETSET_FLAG(df, IPV4_FLAG_DF);
IPV4_GETSET_FLAG(mf, IPV4_FLAG_MF);
IPV4_GETSET_FLAG(rb, IPV4_FLAG_RB);

bool ipv4_verify_checksum(const struct ipv4 *ip);
void ipv4_compute_checksum(struct ipv4 *ip);

void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size);
ipv4addr ipv4_addr_from_string(const char *string);
ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d);


#endif /* _HAKA_PROTO_IPV4_IPV4_H */
