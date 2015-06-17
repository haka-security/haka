/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Basic types used all along Haka.
 */

#ifndef HAKA_TYPES_H
#define HAKA_TYPES_H

#include <haka/config.h>
#include <byteswap.h>


typedef unsigned char              bool; /**< Boolean. Its value can be `true` or `false`. */

#define true                       1 /**< Boolean value `true`. */
#define false                      0 /**< Boolean value `false`. */

typedef char                       int8;  /**< Signed 1 byte integer type. */
typedef HAKA_16BIT_TYPE            int16; /**< Signed 2 byte integer type. */
typedef HAKA_32BIT_TYPE            int32; /**< Signed 4 byte integer type. */
typedef HAKA_64BIT_TYPE            int64; /**< Signed 8 byte integer type. */

typedef unsigned char              uint8;  /**< Unsigned 1 byte integer type. */
typedef unsigned HAKA_16BIT_TYPE   uint16; /**< Unsigned 2 byte integer type. */
typedef unsigned HAKA_32BIT_TYPE   uint32; /**< Unsigned 4 byte integer type. */
typedef unsigned HAKA_64BIT_TYPE   uint64; /**< Unsigned 8 byte integer type. */

/** \cond */
#define SWAP_int8(x) (x)
#define SWAP_int16(x) bswap_16(x)
#define SWAP_int32(x) bswap_32(x)
#define SWAP_int64(x) bswap_64(x)

#define SWAP_uint8(x) (x)
#define SWAP_uint16(x) SWAP_int16(x)
#define SWAP_uint32(x) SWAP_int32(x)
#define SWAP_uint64(x) SWAP_int64(x)
/** \endcond */

/**
 * Byte swapping utility macro.
 *
 * \param type C type (int32, uint64...)
 * \param x Value to swap.
 */
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


/**
 * \def SWAP_TO_BE(type, x)
 * Byte swapping utility macro. Swap from local machine endian
 * to big endian.
 *
 * \param type C type (int32, uint64...)
 * \param x Value to swap.
 */

/**
 * \def SWAP_FROM_BE(type, x)
 * Byte swapping utility macro. Swap from big endian to local machine
 * endian.
 *
 * \param type C type (int32, uint64...)
 * \param x Value to swap.
 */

/**
 * \def SWAP_TO_LE(type, x)
 * Byte swapping utility macro. Swap from local machine endian
 * to little endian.
 *
 * \param type C type (int32, uint64...)
 * \param x Value to swap.
 */

/**
 * \def SWAP_FROM_LE(type, x)
 * Byte swapping utility macro. Swap from little endian to local machine
 * endian.
 *
 * \param type C type (int32, uint64...)
 * \param x Value to swap.
 */

/**
 * Get the bit `i` from an integer `v`.
 */
#define GET_BIT(v, i)           ((((v) & (1 << (i))) != 0))

/**
 * Set the bit `i` from an integer `v` to `x`.
 */
#define SET_BIT(v, i, x)        ((x) ? ((v) | (1 << (i))) : ((v) & ~(1 << (i))))

/** \cond */
#define GET_BITS_MASK(i, j)    (((1<<((j)-(i)))-1)<<(i))
/** \endcond */

/**
 * Get the bits in range [`i` ; `j`] from an integer `v`.
 */
#define GET_BITS(v, i, j)       (((v) & GET_BITS_MASK(i, j)) >> (i))

/**
 * Set the bits in range [`i` ; `j`] from an integer `v` to `x`.
 */
#define SET_BITS(v, i, j, x)    (((v) & ~(GET_BITS_MASK(i, j))) | (((x) << (i)) & GET_BITS_MASK(i, j)))

#endif /* HAKA_TYPES_H */
