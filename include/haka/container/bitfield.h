/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_CONTAINER_BITFIELD_H
#define _HAKA_CONTAINER_BITFIELD_H

#include <haka/compiler.h>
#include <haka/types.h>
#include <assert.h>


/*
 * Basic bitfield type which store bit by group of 32
 * inside a 32 bit integer.
 */
struct bitfield {
	size_t     size;
	int32      data[0];
};

#define BITFIELD_STATIC(size, name) \
	struct name { \
		struct bitfield bitfield; \
		int32           data[(size+31) / 32]; \
	};

#define BITFIELD_STATIC_INIT(n) { { size: n }, {0} }

/**
 * Get the value of a flag in the bitfield.
 */
INLINE bool bitfield_get(struct bitfield *bf, int off)
{
	assert(off < bf->size);
	return (bf->data[off >> 5] & (1L << (off & 0x1f))) != 0;
}

/**
 * Set the value of a flag in the bitfield.
 */
INLINE void bitfield_set(struct bitfield *bf, int off, bool val)
{
	int32 * const ptr = bf->data + (off >> 5);
	const int32 mask = off & 0x1f;

	assert(off < bf->size);

	*ptr = (*ptr & ~(1L << mask)) | (val << mask);
}

#endif /* _HAKA_CONTAINER_BITFIELD_H */
