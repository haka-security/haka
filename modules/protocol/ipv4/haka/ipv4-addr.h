/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * IPv4 address API.
 */

#ifndef _HAKA_PROTO_IPV4_ADDR_H
#define _HAKA_PROTO_IPV4_ADDR_H

#include <haka/types.h>
#include <stddef.h>


/**
 * String max size for converting ipv4addr to string.
 */
#define IPV4_ADDR_STRING_MAXLEN   15

/**
 * IPv4 address type.
 */
typedef uint32 ipv4addr;

/** \cond */
#define SWAP_ipv4addr(x) SWAP_uint32(x)
/** \endcond */

/**
 * Convert IP from ipv4addr to string.
 */
void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size);

/**
 * Convert IP from string to ipv4addr structure.
 */
ipv4addr ipv4_addr_from_string(const char *string);

/**
 * Convert IP from bytes to ipv4addr.
 */
ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d);

/** \cond */
#define TOSTR_ipv4addr_size IPV4_ADDR_STRING_MAXLEN
#define TOSTR_ipv4addr(obj, str) ipv4_addr_to_string((obj), (str), IPV4_ADDR_STRING_MAXLEN)
/** \endcond */

#endif /* _HAKA_PROTO_IPV4_ADDR_H */
