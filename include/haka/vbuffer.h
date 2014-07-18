/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Virtual buffer abstraction.
 */

#ifndef _HAKA_VBUFFER_H
#define _HAKA_VBUFFER_H

#include <haka/types.h>
#include <haka/container/list2.h>
#include <haka/lua/object.h>


/*
 * Structures
 */

#define ALL   (size_t)-1 /**< Define for vbuffer size. */

typedef uint32 vbsize_t; /**< vbuffer size type. */
struct vbuffer_data;

/**
 * Clone mode.
 */
typedef enum {
	CLONE_COPY,     /**< Copy the buffer. */
	CLONE_RW,       /**< Clone the buffer keeping the writable flag. */
	CLONE_RO_ORIG,  /**< Clone the buffer and mark the original buffer as read-only. */
	CLONE_RO_CLONE, /**< Clone the buffer in read-only mode. */
} clone_mode;

/**
 * vbuffer data function table.
 */
struct vbuffer_data_ops {
	void   (*free)(struct vbuffer_data *data);
	void   (*addref)(struct vbuffer_data *data);
	bool   (*release)(struct vbuffer_data *data);
	uint8 *(*get)(struct vbuffer_data *data, bool write);
};

/**
 * Abstract vbuffer data structure.
 */
struct vbuffer_data {
	struct vbuffer_data_ops     *ops;
};

struct vbuffer_chunk;

/**
 * Virtual buffer structure.
 * It abstracts the memory representation using a scatter list
 * of memory block and allows easy and efficient modifications
 * to the memory.
 *
 * \image html "developer/ref/vbuffer.png"
 */
struct vbuffer {
	struct lua_object            lua_object;
	struct vbuffer_chunk        *chunks;
};

extern const struct vbuffer vbuffer_init; /**< Initializer for vbuffer. */

/**
 * Iterator on a virtual buffer.
 *
 * \image html "developer/ref/vbuffer_iterator.png"
 *
 */
struct vbuffer_iterator {
	struct vbuffer_chunk        *chunk;        /**< \private */
	vbsize_t                     offset;       /**< \private */
	vbsize_t                     meter;        /**< \private */
	bool                         registered:1; /**< \private */
};

extern const struct vbuffer_iterator vbuffer_iterator_init; /**< Initializer for vbuffer_iterator. */

/**
 * Sub part of a virtual buffer.
 *
 * \image html "developer/ref/vbuffer_sub.png"
 */
struct vbuffer_sub {
	struct vbuffer_iterator      begin;       /**< \private */
	bool                         use_size:1;  /**< \private */
	union {
		struct vbuffer_iterator  end;         /**< \private */
		vbsize_t                 length;      /**< \private */
	};
};

extern const struct vbuffer_sub vbuffer_sub_init; /**< Initializer for vbuffer_sub. */

/**
 * Opaque data used when mmapping some data.
 */
struct vbuffer_sub_mmap {
	struct vbuffer_chunk        *data;  /**< \private */
	vbsize_t                     len;   /**< \private */
	vbsize_t                     meter; /**< \private */
};

extern const struct vbuffer_sub_mmap vbuffer_mmap_init; /**< Initializer for vbuffer_sub_mmap. */


/*
 * Functions
 */

/**
 * Check if a vbuffer is valid.
 */
bool          vbuffer_isvalid(const struct vbuffer *buf);

/**
 * Check if a vbuffer is empty.
 */
bool          vbuffer_isempty(const struct vbuffer *buf);

/**
 * Create a new empty vbuffer.
 */
bool          vbuffer_create_empty(struct vbuffer *buf);

/**
 * Create a new vbuffer and allocate a block of memory for it.
 */
bool          vbuffer_create_new(struct vbuffer *buf, size_t size, bool zero);

/**
 * Create a new vbuffer from a memory block. The memory will be copied.
 */
bool          vbuffer_create_from(struct vbuffer *buf, const char *str, size_t len);

/**
 * Clean all data in the vbuffer.
 */
void          vbuffer_clear(struct vbuffer *buf);

/**
 * Release the vbuffer internal memory.
 */
void          vbuffer_release(struct vbuffer *buf);

/**
 * Get an iterator at the given offset.
 */
void          vbuffer_position(const struct vbuffer *buf, struct vbuffer_iterator *position, size_t offset);

/**
 * Get an iterator at the beginning.
 */
INLINE void   vbuffer_begin(const struct vbuffer *buf, struct vbuffer_iterator *position);

/**
 * Get an iterator at the end.
 */
INLINE void   vbuffer_end(const struct vbuffer *buf, struct vbuffer_iterator *position);

/**
 * Get an iterator at tha last position of the buffer. This iterator will be different
 * if the vbuffer is not empty. The end is always at the end even if some data are appended,
 * but the last position is attached to the last byte in the buffer. So if some data are
 * added, those data will be after this last position.
 */
void          vbuffer_last(const struct vbuffer *buf, struct vbuffer_iterator *position);

/**
 * Change the writable flag on the whole buffer.
 */
void          vbuffer_setwritable(struct vbuffer *buf, bool writable);

/**
 * Check if the buffer is writable.
 * \note This function does not check that every memory block are writable.
 */
bool          vbuffer_iswritable(struct vbuffer *buf);

/**
 * Check if some memory blocks have been modified.
 */
bool          vbuffer_ismodified(struct vbuffer *buf);

/**
 * Clear the modified flag on the whole buffer.
 */
void          vbuffer_clearmodified(struct vbuffer *buf);

/**
 * Insert some data at the end of the buffer.
 */
bool          vbuffer_append(struct vbuffer *buf, struct vbuffer *buffer);

/**
 * Compute the size of the buffer.
 * \note This function needs to process all memory block.
 */
INLINE size_t vbuffer_size(struct vbuffer *buf);

/**
 * Check if the buffer is larger than a given size.
 */
INLINE bool   vbuffer_check_size(struct vbuffer *buf, size_t minsize, size_t *size);

/**
 * Check if the buffer is only made of contiguous memory block.
 */
INLINE bool   vbuffer_isflat(struct vbuffer *buf);

/**
 * Flatten a buffer if needed.
 */
INLINE const uint8 *vbuffer_flatten(struct vbuffer *buf, size_t *size);

/**
 * Create a clone of the buffer.
 */
INLINE bool   vbuffer_clone(struct vbuffer *data, struct vbuffer *buffer, bool copy);

/**
 * Swap the content of two buffers.
 */
void          vbuffer_swap(struct vbuffer *a, struct vbuffer *b);

/**
 * Check if an iterator is valid.
 */
bool          vbuffer_iterator_isvalid(const struct vbuffer_iterator *position);

/**
 * Create a copy of an iterator.
 */
void          vbuffer_iterator_copy(const struct vbuffer_iterator *src, struct vbuffer_iterator *dst);

/**
 * Clear an iterator.
 */
void          vbuffer_iterator_clear(struct vbuffer_iterator *position);

/**
 * Get the available byte from this iterator.
 * \note This function needs to process all memory block.
 */
size_t        vbuffer_iterator_available(struct vbuffer_iterator *position);

/**
 * Check if the available bytes are larger than a minimal size.
 */
bool          vbuffer_iterator_check_available(struct vbuffer_iterator *position, size_t minsize, size_t *size);

/**
 * Register the iterator to make it safe. When this is done, if the buffer behind the iterator
 * is released, the iterator will raise an error when trying to do operations with it. If it
 * is not registered, the program will probably segfault.
 */
bool          vbuffer_iterator_register(struct vbuffer_iterator *position);

/**
 * Unregister an iterator.
 */
bool          vbuffer_iterator_unregister(struct vbuffer_iterator *position);

/**
 * Insert some data at the iterator position.
 */
bool          vbuffer_iterator_insert(struct vbuffer_iterator *position, struct vbuffer *buffer, struct vbuffer_sub *sub);

/**
 * Advance the iterator on the data.
 * \returns The number of bytes advanced. This value might be lower than the given length
 * if the buffer does not have enough data.
 */
size_t        vbuffer_iterator_advance(struct vbuffer_iterator *position, size_t len);

/**
 * Skip empty vbuffer chunk at the iterator position.
 */
void          vbuffer_iterator_skip_empty(struct vbuffer_iterator *position);

/**
 * Check if the iterator is at the end of the buffer.
 */
bool          vbuffer_iterator_isend(struct vbuffer_iterator *position);

/**
 * Check if the iterator is at the end of the buffer and that this end is marked as
 * `eof`. This is used by vbuffer_stream.
 */
bool          vbuffer_iterator_iseof(struct vbuffer_iterator *position);

/**
 * Split the current memory block at the iterator position.
 *
 * Starting from an iterator on the buffer:
 *
 * \image html "developer/ref/vbuffer_iterator.png"
 *
 * The chunk pointed by the iterator will be split into two parts like this:
 *
 * \image html "developer/ref/vbuffer_split.png"
 */
bool          vbuffer_iterator_split(struct vbuffer_iterator *position);

/**
 * Create a sub buffer from the iterator position. The iterator will advance accordingly.
 */
size_t        vbuffer_iterator_sub(struct vbuffer_iterator *position, size_t len, struct vbuffer_sub *sub, bool split);

/**
 * Get the current byte.
 * \note The iterator will not move.
 */
uint8         vbuffer_iterator_getbyte(struct vbuffer_iterator *position);

/**
 * Set the current byte.
 * \note The iterator will not move.
 */
bool          vbuffer_iterator_setbyte(struct vbuffer_iterator *position, uint8 byte);

/**
 * Mmap each block one by one.
 */
uint8        *vbuffer_iterator_mmap(struct vbuffer_iterator *position, size_t maxsize, size_t *size, bool write);

/**
 * Insert a mark at the iterator position.
 *
 * The resulting buffer will look like this:
 *
 * \image html "developer/ref/vbuffer_mark.png"
 */
bool          vbuffer_iterator_mark(struct vbuffer_iterator *position, bool readonly);

/**
 * Unmark the buffer.
 * \note The iterator must be placed on a mark created with vbuffer_iterator_mark()
 */
bool          vbuffer_iterator_unmark(struct vbuffer_iterator *position);

/**
 * Check if a buffer could be inserted. This verify that the insert will not create
 * loops.
 */
bool          vbuffer_iterator_isinsertable(struct vbuffer_iterator *position, struct vbuffer *buffer);

/**
 * Clear a sub buffer.
 */
void          vbuffer_sub_clear(struct vbuffer_sub *data);

/**
 * Register a sub buffer.
 * \see vbuffer_iterator_register()
 */
bool          vbuffer_sub_register(struct vbuffer_sub *data);

/**
 * Unregister a sub buffer.
 */
bool          vbuffer_sub_unregister(struct vbuffer_sub *data);

/**
 * Create a new sub buffer.
 */
void          vbuffer_sub_create(struct vbuffer_sub *data, struct vbuffer *buffer, size_t offset, size_t length);

/**
 * Create a new sub buffer from an iterator position.
 */
bool          vbuffer_sub_create_from_position(struct vbuffer_sub *data, struct vbuffer_iterator *position, size_t length);

/**
 * Create a new sub buffer between two iterator positions.
 */
bool          vbuffer_sub_create_between_position(struct vbuffer_sub *data, struct vbuffer_iterator *begin, struct vbuffer_iterator *end);

/**
 * Get an iterator at the beginning of the sub buffer.
 */
void          vbuffer_sub_begin(struct vbuffer_sub *data, struct vbuffer_iterator *begin);

/**
 * Get an iterator at the end of the sub buffer.
 */
void          vbuffer_sub_end(struct vbuffer_sub *data, struct vbuffer_iterator *end);

/**
 * Get an iterator at the given offset.
 */
bool          vbuffer_sub_position(struct vbuffer_sub *data, struct vbuffer_iterator *iter, size_t offset);

/**
 * Create a new sub buffer.
 */
bool          vbuffer_sub_sub(struct vbuffer_sub *data, size_t offset, size_t length, struct vbuffer_sub *buffer);

/**
 * Compute the size of the sub buffer.
 * \note This function needs to process all memory block.
 */
size_t        vbuffer_sub_size(struct vbuffer_sub *data);

/**
 * Check if the sub buffer is larger than a given minimal size.
 */
bool          vbuffer_sub_check_size(struct vbuffer_sub *data, size_t minsize, size_t *size);

/**
 * Copy the data in the sub buffer.
 */
size_t        vbuffer_sub_read(struct vbuffer_sub *data, uint8 *ptr, size_t size);

/**
 * Copy and replace the data in the sub buffer.
 */
size_t        vbuffer_sub_write(struct vbuffer_sub *data, const uint8 *ptr, size_t size);

/**
 * Flatten the sub buffer if needed.
 */
const uint8  *vbuffer_sub_flatten(struct vbuffer_sub *data, size_t *size);

/**
 * Compact the sub buffer by aggregating memory blocks when possible.
 */
bool          vbuffer_sub_compact(struct vbuffer_sub *data);

/**
 * Check if the sub buffer is only made of one memory block.
 */
bool          vbuffer_sub_isflat(struct vbuffer_sub *data);

/**
 * Clone the content of the sub buffer.
 */
bool          vbuffer_sub_clone(struct vbuffer_sub *data, struct vbuffer *buffer, clone_mode mode);

/**
 * Mmap every memory block of the buffer.
 */
uint8        *vbuffer_mmap(struct vbuffer_sub *data, size_t *len, bool write, struct vbuffer_sub_mmap *mmap_iter, struct vbuffer_iterator *iter);

/**
 * Set each byte of the buffer to zero.
 */
bool          vbuffer_zero(struct vbuffer_sub *data);

/**
 * Extract part of a buffer. The data are removed from the original buffer and moved to a new
 * vbuffer.
 */
bool          vbuffer_extract(struct vbuffer_sub *data, struct vbuffer *buffer);

/**
 * Extract part of a buffer. The data are removed from the original buffer and moved to a new
 * vbuffer. Unlike vbuffer_extract(), this operator does not mark the buffer as modified. It
 * is needed to restore the data using the function vbuffer_restore().
 */
bool          vbuffer_select(struct vbuffer_sub *data, struct vbuffer *buffer, struct vbuffer_iterator *ref);

/**
 * Restore data extracted using vbuffer_select().
 */
bool          vbuffer_restore(struct vbuffer_iterator *position, struct vbuffer *data, bool clone);

/**
 * Erase the data in the sub buffer.
 */
bool          vbuffer_erase(struct vbuffer_sub *data);

/**
 * Replace the data in the sub buffer.
 */
bool          vbuffer_replace(struct vbuffer_sub *data, struct vbuffer *buffer);

/**
 * Convert the content of the buffer to a number.
 * \note The function will raise an error if the buffer size is not supported.
 */
int64         vbuffer_asnumber(struct vbuffer_sub *data, bool bigendian);

/**
 * Set the content of the buffer from a number.
 * \note The function will raise an error if the buffer size is not supported.
 */
bool          vbuffer_setnumber(struct vbuffer_sub *data, bool bigendian, int64 num);

/**
 * Convert bits of the content of the buffer to a number.
 * \note The function will raise an error if the buffer size is not supported.
 */
int64         vbuffer_asbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian);

/**
 * Set bits of the content of the buffer from a number.
 * \note The function will raise an error if the buffer size is not supported.
 */
bool          vbuffer_setbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian, int64 num);

/**
 * Convert the content of the buffer to a string.
 */
size_t        vbuffer_asstring(struct vbuffer_sub *data, char *str, size_t len);

/**
 * Set the content of the buffer from a string. This function will change the data inplace
 * without modifying the buffer size.
 */
size_t        vbuffer_setfixedstring(struct vbuffer_sub *data, const char *str, size_t len);

/**
 * Set the content of the buffer from a string. This function will change the size of the
 * buffer to match the one given by the arguments.
 */
bool          vbuffer_setstring(struct vbuffer_sub *data, const char *str, size_t len);

/**
 * Get the value of a byte at the given position in the buffer.
 */
uint8         vbuffer_getbyte(struct vbuffer_sub *data, size_t offset);

/**
 * Set the value of a byte at the given position in the buffer.
 */
bool          vbuffer_setbyte(struct vbuffer_sub *data, size_t offset, uint8 byte);


/*
 * Inlines
 */

INLINE void   vbuffer_begin(const struct vbuffer *buf, struct vbuffer_iterator *position)
{
	vbuffer_position(buf, position, 0);
}

INLINE void   vbuffer_end(const struct vbuffer *buf, struct vbuffer_iterator *position)
{
	vbuffer_position(buf, position, ALL);
}

INLINE size_t vbuffer_size(struct vbuffer *buf)
{
	struct vbuffer_sub sub;
	vbuffer_sub_create(&sub, buf, 0, ALL);
	return vbuffer_sub_size(&sub);
}

INLINE bool   vbuffer_check_size(struct vbuffer *buf, size_t minsize, size_t *size)
{
	struct vbuffer_sub sub;
	vbuffer_sub_create(&sub, buf, 0, ALL);
	return vbuffer_sub_check_size(&sub, minsize, size);
}

INLINE bool   vbuffer_isflat(struct vbuffer *buf)
{
	struct vbuffer_sub sub;
	vbuffer_sub_create(&sub, buf, 0, ALL);
	return vbuffer_sub_isflat(&sub);
}

INLINE const uint8 *vbuffer_flatten(struct vbuffer *buf, size_t *size)
{
	struct vbuffer_sub sub;
	vbuffer_sub_create(&sub, buf, 0, ALL);
	return vbuffer_sub_flatten(&sub, size);
}

INLINE bool   vbuffer_clone(struct vbuffer *data, struct vbuffer *buffer, bool copy)
{
	struct vbuffer_sub sub;
	vbuffer_sub_create(&sub, data, 0, ALL);
	return vbuffer_sub_clone(&sub, buffer, copy);
}

#endif /* _HAKA_VBUFFER_H */
