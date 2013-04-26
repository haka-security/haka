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

/*
 * Define a type for IPv4 addresses 
 * @ingroup IPv4
 */
typedef uint32 ipv4addr;
#define SWAP_ipv4addr(x) SWAP_uint32(x)

/**
 * IPv4 header structure.
 * @ingroup IPv4
 */
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
 * IPv4 structure
 * @ingroup IPv4
 */
struct ipv4 {
	struct packet       *packet;
	struct ipv4_header  *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
};

/**
 * @brief Dissect IPv4 header
 * @param packet Opaque packet structure
 * @return Pointer to IPv4 structure
 * @ingroup IPv4
 */
struct ipv4 *ipv4_dissect(struct packet *packet);

/**
 * @brief Forge IPv4 header
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_forge(struct ipv4 *ip);

/**
 * @brief Release IPv4 structure memory allocation
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_release(struct ipv4 *ip);

/**
 * @brief Make IPv4 header field values modifiable
 * @param ip IPv4 structure
 * @ingroup IPv4
 */
void ipv4_modified(struct ipv4 *ip);

#define IPV4_GETSET_FIELD(type, field) \
		static inline type ipv4_get_##field(const struct ipv4 *ip) { return SWAP_FROM_IPV4(type, ip->header->field); } \
		static inline void ipv4_set_##field(struct ipv4 *ip, type v) { ipv4_modified(ip); ip->header->field = SWAP_TO_IPV4(type, v); }

/**
 * @fn uint8 ipv4_get_hdr_len(const struct ipv4 *ip)
 * @brief Get IPv4 header length
 * @param ip IPv4 structure
 * @return IPv4 header length value
 * @ingroup IPv4
 */

/**
 * @fn void ipv4_set_hdr_len(struct ipv4 *ip, type v)
 * @brief Set IPv4 header length to value v
 * @param ip IPv4 structure
 * @param v value to set 
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(uint8, hdr_len);

/**
 * @fn uint8 ipv4_get_version(const struct ipv4 *ip)
 * @brief Get IPv4 version
 * @param ip IPv4 structure
 * @return IPv4 version value
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
IPV4_GETSET_FIELD(uint8, tos);

/**
 * @fn uint16 ipv4_get_len(const struct ipv4 *ip)
 * @brief Get IPv4 total length
 * @param ip IPv4 structure
 * @return IPv4 total length value
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
IPV4_GETSET_FIELD(uint16, id);

/**
 * @fn uint8 ipv4_get_ttl(const struct ipv4 *ip)
 * @brief Get IPv4 time to live (ttl)
 * @param ip IPv4 structure
 * @return IPv4 time to live value
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
IPV4_GETSET_FIELD(uint8, proto);

/**
 * @fn uint16 ipv4_get_checksum(const struct ipv4 *ip)
 * @brief Get IPv4 cheksum
 * @param ip IPv4 structure
 * @return IPv4 checksum value
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
IPV4_GETSET_FIELD(ipv4addr, src);

/**
 * @fn ipv4addr ipv4_get_dst(const struct ipv4 *ip)
 * @brief Get IPv4 destination address
 * @param ip IPv4 structure
 * @return IPv4 destination address value
 * @ingroup IPv4
 */
IPV4_GETSET_FIELD(ipv4addr, dst);


/**
 * @brief Get IPv4 fragment offset
 * @param ip IPv4 structure
 * @return IPv4 fragment offset value
 * @ingroup IPv4
 */
static inline uint16 ipv4_get_frag_offset(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS);
}

/**
 * @brief Set IPv4 fragment offset to provided value
 * @param ip IPv4 structure
 * @param v value to set
 * @return IPv4 fragment offset value
 * @ingroup IPv4
 */
static inline void ipv4_set_frag_offset(struct ipv4 *ip, uint16 v)
{
	ipv4_modified(ip);
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FRAGMENTOFFSET_BITS, v);
}

static inline uint16 ipv4_get_flags(const struct ipv4 *ip)
{
	return IPV4_GET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS);
}

static inline void ipv4_set_flags(struct ipv4 *ip, uint16 v)
{
	ipv4_modified(ip);
	ip->header->fragment = IPV4_SET_BITS(uint16, ip->header->fragment, IPV4_FLAG_BITS, v);
}

#define IPV4_GETSET_FLAG(name, flag) \
		static inline bool ipv4_get_flags_##name(const struct ipv4 *ip) { return IPV4_GET_BIT(uint16, ip->header->fragment, flag); } \
		static inline void ipv4_set_flags_##name(struct ipv4 *ip, bool value) { ipv4_modified(ip); ip->header->fragment = IPV4_SET_BIT(uint16, ip->header->fragment, flag, value); }

/**
 * @fn bool ipv4_get_flags_df(const struct ipv4 *ip)
 * @brief Get IPv4 don't fragment bit
 * @param ip IPv4 structure
 * @return IPv4 don't fragment bit value
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
IPV4_GETSET_FLAG(mf, IPV4_FLAG_MF);

/**
 * @fn bool ipv4_get_flags_rb(const struct ipv4 *ip)
 * @brief Get IPv4 reserved bit
 * @param ip IPv4 structure
 * @return IPv4 reserved bit value
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
 * @brief Convert IP from ipv4addr to string
 * @param addr address to be converted
 * @param string converted address
 * @param size string size 
 * @ingroup IPv4
 */
void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size);

/**
 * @brief Convert IP from string to ipv4addr structure
 * @param string address to be converted
 * @return ipv4addr converted address 
 * @ingroup IPv4
 */
ipv4addr ipv4_addr_from_string(const char *string);

/**
 * @brief Convert IP from bytes to ipv4addr
 * @param a first IP address byte (starting from left)
 * @param b second IP address byte
 * @param c third IP address byte
 * @param d forth IP address byte 
 * @return ipv4addr converted address
 * @ingroup IPv4
 */
ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d);


#endif /* _HAKA_PROTO_IPV4_IPV4_H */
