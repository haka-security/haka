/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_PROTO_IPV4_NETWORK_H
#define _HAKA_PROTO_IPV4_NETWORK_H

#include <haka/types.h>


#define IPV4_NETWORK_STRING_MAXLEN   18
#define IPV4_MASK_MAXVAL             32


typedef struct {
	ipv4addr net;
	uint8    mask;
} ipv4network;

extern const ipv4network ipv4_network_zero;

void ipv4_network_to_string(ipv4network net, char *string, size_t size);
ipv4network ipv4_network_from_string(const char *string);
uint8 ipv4_network_contains(ipv4network network, ipv4addr addr);

#endif /* _HAKA_PROTO_IPV4_NETWORK_H */
