/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_TYPES_H
#define _HAKA_TYPES_H

#include <haka/config.h>
#include <byteswap.h>


typedef unsigned char              bool;

#define true                       1
#define false                      0

typedef char                       int8;
typedef HAKA_16BIT_TYPE            int16;
typedef HAKA_32BIT_TYPE            int32;
typedef HAKA_64BIT_TYPE            int64;

typedef unsigned char              uint8;
typedef unsigned HAKA_16BIT_TYPE   uint16;
typedef unsigned HAKA_32BIT_TYPE   uint32;
typedef unsigned HAKA_64BIT_TYPE   uint64;


#define SWAP_int8(x) (x)
#define SWAP_int16(x) bswap_16(x)
#define SWAP_int32(x) bswap_32(x)
#define SWAP_int64(x) bswap_64(x)

#define SWAP_uint8(x) (x)
#define SWAP_uint16(x) SWAP_int16(x)
#define SWAP_uint32(x) SWAP_int32(x)
#define SWAP_uint64(x) SWAP_int64(x)

#define SWAP(type, x)  SWAP_##type(x)

#ifdef HAKA_BIGENDIAN
	#define SWAP_TO_BE(type, x)    (x)
	#define SWAP_FROM_BE(type, x)  (x)
	#define SWAP_TO_LE(type, x)    SWAP(type, (x))
	#define SWAP_FROM_LE(type, x)  SWAP(type, (x))
#else
	#define SWAP_TO_LE(type, x)    (x)
	#define SWAP_FROM_LE(type, x)  (x)
	#define SWAP_TO_BE(type, x)    SWAP(type, (x))
	#define SWAP_FROM_BE(type, x)  SWAP(type, (x))
#endif

#define GET_BIT(v, i)           ((((v) & (1 << (i))) != 0))
#define SET_BIT(v, i, x)        ((x) ? ((v) | (1 << (i))) : ((v) & ~(1 << (i))))

#define _GET_BITS_MASK(i, j)    (((1<<((j)-(i)))-1)<<(i))

#define GET_BITS(v, i, j)       (((v) & _GET_BITS_MASK(i, j)) >> (i))
#define SET_BITS(v, i, j, x)    (((v) & ~(_GET_BITS_MASK(i, j))) | (((x) << (i)) & _GET_BITS_MASK(i, j)))

#endif /* _HAKA_TYPES_H */
