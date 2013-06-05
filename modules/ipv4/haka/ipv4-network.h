
#ifndef _HAKA_PROTO_IPV4_NETWORK_H
#define _HAKA_PROTO_IPV4_NETWORK_H

#include <haka/types.h>


#define IPV4_NET_STRING_MAXLEN    18
#define IPV4_MASK_MAXVAL          32


/*
* Define a type for IPv4 network addresses
* @ingroup IPv4
*/
typedef struct {
	ipv4addr net;
	uint8    mask;
} ipv4network;

extern const ipv4network ipv4_network_zero;


/**
 * @brief Convert network address from ipv4network to string
 * @param net network address to be converted
 * @param string converted network address
 * @param size string size
 * @ingroup IPv4
 */
void ipv4_network_to_string(ipv4network net, char *string, size_t size);

/**
 * @brief Convert network address from string to ipv4network structure
 * @param string network address to be converted
 * @return ipv4network converted network address
 * @ingroup IPv4
 */
ipv4network ipv4_network_from_string(const char *string);

/**
 * Check if IPv4 address is in network range
 * @param network IPv4 network address
 * @param addr Ipv4 address
 * @return true if ip address is in network range and false otherwise
 */
uint8 ipv4_network_contains(ipv4network network, ipv4addr addr);

#endif /* _HAKA_PROTO_IPV4_NETWORK_H */
