/**
 * @file ipv4.h
 * @brief  IPv4 Dissector API
 * @author Pierre-Sylvain Desse
 *
 * The file contains the IPv4 disscetor API functions and structures definition
 */

/**
 * @defgroup IPv4 IPv4
 * @brief IPv4 dissector API functions and structures.
 * @ingroup ExternProtocolModule
 */

#ifndef _HAKA_PROTO_IPV4_IPV4_H
#define _HAKA_PROTO_IPV4_IPV4_H

#include <haka/packet.h>
#include <haka/types.h>
#include <haka/compiler.h>
#include <haka/error.h>

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

/*
 * Define a type for IPv4 addresses
 * @ingroup IPv4
 */
typedef uint32 ipv4addr;
#define SWAP_ipv4addr(x) SWAP_uint32(x)

typedef struct {
	ipv4addr net;
	uint8    mask;
}ipv4network;

extern const ipv4network ipv4_network_zero;

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

/**
 * IPv4 opaque structure
 * @ingroup IPv4
 */
struct ipv4 {
	struct packet       *packet;
	struct ipv4_header  *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
	bool                 drop:1;
};

/**
 * @brief Dissect IPv4 header
 * @param packet Opaque packet structure
 * @return Pointer to IPv4 structure
 * @ingroup IPv4
 */
struct ipv4 *ipv4_dissect(struct packet *packet);

/**
 * @brief Forge IPv4 packet
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
struct packet *ipv4_forge(struct ipv4 *ip);

/**
 * @brief Release IPv4 structure memory allocation
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_release(struct ipv4 *ip);

/**
 * Function that need to be called before modifying fields not part if
 * the header.
 * It is automatically called if you use the standard accessors.
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_pre_modify(struct ipv4 *ip);

/**
 * Function that need to be called before modifying fields of the header.
 * It is automatically called if you use the standard header accessors.
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_pre_modify_header(struct ipv4 *ip);

#define IPV4_GETSET_FIELD(type, field) \
		INLINE type ipv4_get_##field(const struct ipv4 *ip) { IPV4_CHECK(ip, 0); return SWAP_FROM_IPV4(type, ip->header->field); } \
		INLINE void ipv4_set_##field(struct ipv4 *ip, type v) { IPV4_CHECK(ip); ipv4_pre_modify_header(ip); ip->header->field = SWAP_TO_IPV4(type, v); }


/**
 * @fn uint8 ipv4_get_version(const struct ipv4 *ip)
 * @brief Get IPv4 version
 * @param ip IPv4 structure
 * @return IPv4 version value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_version(struct ipv4 *ip, uint8 v)
 * @brief Set IPv4 version
 * @param ip IPv4 structure
 * @param v Version value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint8, version);

/**
 * @fn uint8 ipv4_get_tos(const struct ipv4 *ip)
 * @brief Get IPv4 type of service (tos)
 * @param ip IPv4 structure
 * @return IPv4 type of service value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_tos(struct ipv4 *ip, uint8 v)
 * @brief Set IPv4 type of service (tos)
 * @param ip IPv4 structure
 * @param v Type of service value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint8, tos);

/**
 * @fn uint16 ipv4_get_len(const struct ipv4 *ip)
 * @brief Get IPv4 total length
 * @param ip IPv4 structure
 * @return IPv4 total length value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_len(struct ipv4 *ip, uint16 v)
 * @brief Set IPv4 total length
 * @param ip IPv4 structure
 * @param v Total length value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint16, len);

/**
 * @fn uint16 ipv4_get_id(const struct ipv4 *ip)
 * @brief Get IPv4 identification (id)
 * @param ip IPv4 structure
 * @return IPv4 identification value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_id(struct ipv4 *ip, uint16 v)
 * @brief Set IPv4 identification (id)
 * @param ip IPv4 structure
 * @param v Identification value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint16, id);

/**
 * @fn uint8 ipv4_get_ttl(const struct ipv4 *ip)
 * @brief Get IPv4 time to live (ttl)
 * @param ip IPv4 structure
 * @return IPv4 time to live value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_get_ttl(struct ipv4 *ip, uint8 v)
 * @brief Set IPv4 time to live (ttl)
 * @param ip IPv4 structure
 * @param v Time to live value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint8, ttl);

/**
 * @fn uint8 ipv4_get_proto(const struct ipv4 *ip)
 * @brief Get IPv4 protocol
 * @param ip IPv4 structure
 * @return IPv4 protocol value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_proto(struct ipv4 *ip, uint8 v)
 * @brief Set IPv4 protocol
 * @param ip IPv4 structure
 * @param v Protocol value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint8, proto);

/**
 * @fn uint16 ipv4_get_checksum(const struct ipv4 *ip)
 * @brief Get IPv4 cheksum
 * @param ip IPv4 structure
 * @return IPv4 checksum value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_checksum(struct ipv4 *ip, uint16 v)
 * @brief Set IPv4 cheksum
 * @param ip IPv4 structure
 * @param v Checksum value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint16, checksum);

/**
 * @fn ipv4addr ipv4_get_src(const struct ipv4 *ip)
 * @brief Get IPv4 source address
 * @param ip IPv4 structure
 * @return IPv4 source address value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_src(struct ipv4 *ip, ipv4addr v)
 * @brief Set IPv4 source address
 * @param ip IPv4 structure
 * @param v Source address value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(ipv4addr, src);

/**
 * @fn ipv4addr ipv4_get_dst(const struct ipv4 *ip)
 * @brief Get IPv4 destination address
 * @param ip IPv4 structure
 * @return IPv4 destination address value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_dst(struct ipv4 *ip, ipv4addr v)
 * @brief Set IPv4 destination address
 * @param ip IPv4 structure
 * @param v Destination address value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(ipv4addr, dst);

/**
 * @fn uint8 ipv4_get_hdr_len(const struct ipv4 *ip)
 * @brief Get IPv4 header length
 * @param ip IPv4 structure
 * @return IPv4 header length value
 * @ingroup IPv4
 */
INLINE uint8 ipv4_get_hdr_len(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return ip->header->hdr_len << IPV4_HDR_LEN_OFFSET;
}

/**
 * @fn void ipv4_set_hdr_len(struct ipv4 *ip, type v)
 * @brief Set IPv4 header length to value v
 * @param ip IPv4 structure
 * @param v value to set
 * @ingroup IPv4
 */
INLINE void ipv4_set_hdr_len(struct ipv4 *ip, uint8 v)
{
	IPV4_CHECK(ip);
	ipv4_pre_modify_header(ip);
	ip->header->hdr_len = v >> IPV4_HDR_LEN_OFFSET;
}

/**
 * @brief Get IPv4 fragment offset
 * @param ip IPv4 structure
 * @return IPv4 fragment offset value
 * @ingroup IPv4
 */
INLINE uint16 ipv4_get_frag_offset(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return (IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS)) << IPV4_FRAGMENTOFFSET_OFFSET;
}

/**
 * @brief Set IPv4 fragment offset
 * @param ip IPv4 structure
 * @param v value to set
 * @ingroup IPv4
 */
INLINE void ipv4_set_frag_offset(struct ipv4 *ip, uint16 v)
{
	IPV4_CHECK(ip);
	ipv4_pre_modify_header(ip);
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS, v >> IPV4_FRAGMENTOFFSET_OFFSET);
}

/**
 * @brief Get IPv4 flags
 * @param ip IPv4 structure
 * @return IPv4 flags value
 * @ingroup IPv4
 */
INLINE uint16 ipv4_get_flags(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS);
}

/**
 * @brief Set IPv4 flags
 * @param ip IPv4 structure
 * @param v New value of the flags
 * @ingroup IPv4
 */
INLINE void ipv4_set_flags(struct ipv4 *ip, uint16 v)
{
	IPV4_CHECK(ip);
	ipv4_pre_modify_header(ip);
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS, v);
}

#define IPV4_GETSET_FLAG(name, flag) \
		INLINE bool ipv4_get_flags_##name(const struct ipv4 *ip) { IPV4_CHECK(ip, 0); return IPV4_GET_BIT(uint16, ip->header->fragment, flag); } \
		INLINE void ipv4_set_flags_##name(struct ipv4 *ip, bool v) { IPV4_CHECK(ip); ipv4_pre_modify_header(ip); ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, flag, v); }

/**
 * @fn bool ipv4_get_flags_df(const struct ipv4 *ip)
 * @brief Get IPv4 don't fragment bit
 * @param ip IPv4 structure
 * @return IPv4 don't fragment bit value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_flags_df(struct ipv4 *ip, bool v)
 * @brief Set IPv4 don't fragment bit
 * @param ip IPv4 structure
 * @param v IPv4 don't fragment bit value
 * @ingroup IPv4
 */
IPV4_GETSET_FLAG(df, IPV4_FLAG_DF);

/**
 * @fn bool ipv4_get_flags_mf(const struct ipv4 *ip)
 * @brief Get IPv4 more fragments bit
 * @param ip IPv4 structure
 * @return IPv4 more fragments bit value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_flags_mf(struct ipv4 *ip, bool v)
 * @brief Set IPv4 more fragments bit
 * @param ip IPv4 structure
 * @param v IPv4 more fragments bit value
 * @ingroup IPv4
 */
IPV4_GETSET_FLAG(mf, IPV4_FLAG_MF);

/**
 * @fn bool ipv4_get_flags_rb(const struct ipv4 *ip)
 * @brief Get IPv4 reserved bit
 * @param ip IPv4 structure
 * @return IPv4 reserved bit value
 * @ingroup IPv4
 */
/**
 * @fn void ipv4_set_flags_rb(struct ipv4 *ip, bool v)
 * @brief Set IPv4 reserved bit
 * @param ip IPv4 structure
 * @param v IPv4 reserved bit value
 * @ingroup IPv4
 */
IPV4_GETSET_FLAG(rb, IPV4_FLAG_RB);

/**
 * @brief Verify IPv4 checksum
 * @param ip IPv4 structure
 * @return true if checksum is valid and false otherwise
 * @ingroup IPv4
 */
bool ipv4_verify_checksum(const struct ipv4 *ip);

/**
 * @brief Compute IPv4 checksum according to RFC #1071
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_compute_checksum(struct ipv4 *ip);

/**
 * Get IPv4 payload data.
 * @param ip IPv4 structure
 * @return Pointer to the IPv4 payload data.
 * @ingroup IPv4
 */
const uint8 *ipv4_get_payload(struct ipv4 *ip);

/**
 * Get IPv4 modifiable payload data.
 * @param ip IPv4 structure
 * @return Pointer to the IPv4 payload data.
 * @ingroup IPv4
 */
uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip);

/**
 * Get IPv4 payload length.
 * @param ip IPv4 structure
 * @return The IPv4 payload size.
 * @ingroup IPv4
 */
size_t ipv4_get_payload_length(struct ipv4 *ip);

/**
 * Get the protocol dissector name to use for this packet.
 * @param ip IPv4 structure
 * @return The dissector name.
 * @ingroup IPv4
 */
const char *ipv4_get_proto_dissector(struct ipv4 *ip);

/**
 * Register the dissector for a given IP protocoal number
 * @param proto Protocol number
 * @param dissector Dissector
 * @ingroup IPv4
 */
void ipv4_register_proto_dissector(uint8 proto, const char *dissector);

/**
 * Drop the IP packet
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_action_drop(struct ipv4 *ip);

/**
 * Get if the packet is valid and can continue to be processed.
 * @param ip IPv4 structure
 * @return True if the packet is valid.
 * @ingroup IPv4
 */
bool ipv4_valid(struct ipv4 *ip);

/**
 * Compute standard checksum on the provided data (RFC #107).
 * @param ptr Pointer to the data
 * @param size Size of input data
 * @return The computed checksum.
 */
int16 inet_checksum(uint16 *ptr, uint16 size);

#endif /* _HAKA_PROTO_IPV4_IPV4_H */
