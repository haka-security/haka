
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

/**
 * Bytes swapping macro.
 * @param type The C type of the value
 * @param x The value
 * @return The byte swapped value.
 * @ingroup Utilities
 */
#define SWAP(type, x)  SWAP_##type(x)

/**
 * @def SWAP_TO_BE(type, x)
 * Swapping value frm local machine endianess to big endian.
 * @param type The C type of the value
 * @param x The value
 * @return The byte swapped value.
 * @ingroup Utilities
 */

/**
 * @def SWAP_FROM_BE(type, x)
 * Swapping value from big endian to local machine endianess.
 * @param type The C type of the value
 * @param x The value
 * @return The byte swapped value.
 * @ingroup Utilities
 */

/**
 * @def SWAP_TO_LE(type, x)
 * Swapping value from local machine endianess to little endian.
 * @param type The C type of the value
 * @param x The value
 * @return The byte swapped value.
 * @ingroup Utilities
 */

/**
 * @def SWAP_FROM_LE(type, x)
 * Swapping value from little endian to local machine endianess.
 * @param type The C type of the value
 * @param x The value
 * @return The byte swapped value.
 * @ingroup Utilities
 */

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
 * Gets the bit i from an integer.
 * @param v The value
 * @param i The number of the bit to get
 * @return true or false
 * @ingroup Utilities
 */
#define GET_BIT(v, i)           ((((v) & (1 << (i))) != 0))

/**
 * Sets the bit i of an integer.
 * @param v The value
 * @param i The number of the bit to set
 * @param x The new value of the bit
 * @return The new integer value
 * @ingroup Utilities
 */
#define SET_BIT(v, i, x)        ((x) ? ((v) | (1 << (i))) : ((v) & ~(1 << (i))))

#define _GET_BITS_MASK(i, j)    (((1<<((j)-(i)))-1)<<(i))

/**
 * Gets the bits in range [i;j] from an integer. The result will also be shifted right by i bits.
 * @param v The value
 * @param i The lower value of the range
 * @param j The upper value of the range
 * @return The resulting bits
 * @ingroup Utilities
 */
#define GET_BITS(v, i, j)       (((v) & _GET_BITS_MASK(i, j)) >> (i))

/**
 * Sets the bits in range [i;j] of an integer.
 * @param v The value
 * @param i The lower value of the range
 * @param j The upper value of the range
 * @param x The new bits value
 * @return The new integer value
 * @ingroup Utilities
 */
#define SET_BITS(v, i, j, x)    (((v) & ~(_GET_BITS_MASK(i, j))) | (((x) << (i)) & _GET_BITS_MASK(i, j)))

#endif /* _HAKA_TYPES_H */
