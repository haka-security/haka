/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * IPv4 network API.
 */

#ifndef HAKA_PROTO_IPV4_NETWORK_H
#define HAKA_PROTO_IPV4_NETWORK_H

#include <haka/types.h>


/**
 * String max size for converting ipv4network to string.
 */
#define IPV4_NETWORK_STRING_MAXLEN   18

/** \cond */
#define IPV4_MASK_MAXVAL             32
/** \endcond */


/**
 * IPv4 network.
 */
typedef struct {
	ipv4addr net;
	uint8    mask;
} ipv4network;

/**
 * IPv4 network initializer.
 */
extern const ipv4network ipv4_network_zero;

/**
 * Convert network address from ipv4network to string.
 */
void ipv4_network_to_string(ipv4network net, char *string, size_t size);

/**
 * Convert network address from string to ipv4network structure.
 */
ipv4network ipv4_network_from_string(const char *string);

/**
 * Checks if IPv4 address is in network range.
 *
 * \return True if ip address is in network range and False otherwise.
 */
uint8 ipv4_network_contains(ipv4network network, ipv4addr addr);

#endif /* HAKA_PROTO_IPV4_NETWORK_H */
