
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

#define IPV4_FLAG_DONTFRAGMENT    15-1
#define IPV4_FLAG_MOREFRAGMENT    15-2
#define IPV4_FLAG_BITS            16-3, 16
#define IPV4_FRAGMENTOFFSET_BITS  0, 16-3

struct ipv4_header {
#ifdef HAKA_LITTLEENDIAN
	uint8    ihl:4;
	uint8    version:4;
#else
	uint8    version:4;
	uint8    ihl:4;
#endif
	uint8    tos;
	uint16   length;
	uint16   id;
	uint16   fragment;
	uint8    ttl;
	uint8    protocol;
	uint16   checksum;
	uint32   saddr;
	uint32   daddr;
};

struct ipv4 {
	struct packet       *packet;
	struct ipv4_header  *header;
	bool                 modified;
};

struct ipv4 *create(struct packet* packet);
void release(struct ipv4 *ip);

#define IPV4_GETSET_FIELD(type, field) \
		static inline type ipv4_get_##field(const struct ipv4 *ip) { return SWAP_FROM_IPV4(type, ip->header->field); } \
		static inline void ipv4_set_##field(struct ipv4 *ip, type v) { ip->header->field = SWAP_TO_IPV4(type, v); }

IPV4_GETSET_FIELD(uint8, ihl);
IPV4_GETSET_FIELD(uint8, version);
IPV4_GETSET_FIELD(uint8, tos);
IPV4_GETSET_FIELD(uint16, length);
IPV4_GETSET_FIELD(uint16, id);
IPV4_GETSET_FIELD(uint8, ttl);
IPV4_GETSET_FIELD(uint8, protocol);
IPV4_GETSET_FIELD(uint16, checksum);
IPV4_GETSET_FIELD(uint32, saddr);
IPV4_GETSET_FIELD(uint32, daddr);

static inline uint16 ipv4_get_fragmentOffset(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS);
}

static inline void ipv4_set_fragmentOffset(struct ipv4 *ip, uint16 v)
{
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS, v);
}

static inline uint16 ipv4_get_flags(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS);
}

static inline void ipv4_set_flags(const struct ipv4 *ip, uint16 v)
{
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS, v);
}

static inline bool ipv4_get_flags_dontFragment(const struct ipv4 *ip)
{
	return IPV4_GET_BIT(uint16, ip->header->fragment, IPV4_FLAG_DONTFRAGMENT);
}

static inline void ipv4_set_flags_dontFragment(const struct ipv4 *ip, bool value)
{
	ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, IPV4_FLAG_DONTFRAGMENT, value);
}

static inline bool ipv4_get_flags_moreFragment(struct ipv4 *ip)
{
	return IPV4_GET_BIT(uint16, ip->header->fragment, IPV4_FLAG_MOREFRAGMENT);
}

static inline void ipv4_set_flags_moreFragment(struct ipv4 *ip, bool value)
{
	ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, IPV4_FLAG_MOREFRAGMENT, value);
}

bool ipv4_verify_checksum(const struct ipv4 *ip);
void ipv4_compute_checksum(struct ipv4 *ip);


#endif /* _HAKA_PROTO_IPV4_IPV4_H */
