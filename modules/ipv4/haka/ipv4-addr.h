#ifndef _HAKA_PROTO_IPV4_ADDR_H
#define _HAKA_PROTO_IPV4_ADDR_H

#include <haka/types.h>

#define IPV4_ADDR_STRING_MAXLEN   15

/*
 * Define a type for IPv4 addresses
 * @ingroup IPv4
 */
typedef uint32 ipv4addr;
#define SWAP_ipv4addr(x) SWAP_uint32(x)


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

#endif /* _HAKA_PROTO_IPV4_ADDR_H */

