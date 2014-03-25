/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_H
#define _HAKA_VBUFFER_H

#include <haka/types.h>
#include <haka/container/list2.h>
#include <haka/lua/object.h>


/*
 * Structures
 */

#define ALL   (size_t)-1

typedef uint32 vbsize_t;
struct vbuffer_data;

struct vbuffer_data_ops {
	void   (*free)(struct vbuffer_data *data);
	void   (*addref)(struct vbuffer_data *data);
	bool   (*release)(struct vbuffer_data *data);
	uint8 *(*get)(struct vbuffer_data *data, bool write);
};

struct vbuffer_data {
	struct vbuffer_data_ops     *ops;
};

struct vbuffer_chunk;

struct vbuffer {
	struct lua_object            lua_object;
	struct vbuffer_chunk        *chunks;
};

extern const struct vbuffer vbuffer_init;

struct vbuffer_iterator {
	struct vbuffer_chunk        *chunk;
	vbsize_t                     offset;
	bool                         registered:1;
};

extern const struct vbuffer_iterator vbuffer_iterator_init;

struct vbuffer_sub {
	struct vbuffer_iterator      begin;
	bool                         use_size:1;
	union {
		vbsize_t                 length;
		struct vbuffer_iterator  end;
	};
};

struct vbuffer_sub_mmap {
	struct vbuffer_chunk        *data;
	vbsize_t                     len;
};

extern const struct vbuffer_sub_mmap vbuffer_mmap_init;


/*
 * Functions
 */

bool          vbuffer_isvalid(const struct vbuffer *buf);
bool          vbuffer_isempty(const struct vbuffer *buf);
bool          vbuffer_create_empty(struct vbuffer *buf);
bool          vbuffer_create_new(struct vbuffer *buf, size_t size, bool zero);
bool          vbuffer_create_from(struct vbuffer *buf, const char *str, size_t len);
void          vbuffer_clear(struct vbuffer *buf);
void          vbuffer_release(struct vbuffer *buf);
void          vbuffer_position(const struct vbuffer *buf, struct vbuffer_iterator *position, size_t offset);
INLINE void   vbuffer_begin(const struct vbuffer *buf, struct vbuffer_iterator *position);
INLINE void   vbuffer_end(const struct vbuffer *buf, struct vbuffer_iterator *position);
void          vbuffer_last(const struct vbuffer *buf, struct vbuffer_iterator *position);
void          vbuffer_setwritable(struct vbuffer *buf, bool writable);
bool          vbuffer_iswritable(struct vbuffer *buf);
bool          vbuffer_ismodified(struct vbuffer *buf);
void          vbuffer_clearmodified(struct vbuffer *buf);
bool          vbuffer_append(struct vbuffer *buf, struct vbuffer *buffer);
INLINE size_t vbuffer_size(struct vbuffer *buf);
INLINE bool   vbuffer_check_size(struct vbuffer *buf, size_t minsize, size_t *size);
INLINE bool   vbuffer_isflat(struct vbuffer *buf);
INLINE const uint8 *vbuffer_flatten(struct vbuffer *buf, size_t *size);
INLINE bool   vbuffer_clone(struct vbuffer *data, struct vbuffer *buffer, bool copy);
void          vbuffer_swap(struct vbuffer *a, struct vbuffer *b);

bool          vbuffer_iterator_isvalid(const struct vbuffer_iterator *position);
void          vbuffer_iterator_copy(const struct vbuffer_iterator *src, struct vbuffer_iterator *dst);
void          vbuffer_iterator_clear(struct vbuffer_iterator *position);
size_t        vbuffer_iterator_available(struct vbuffer_iterator *position);
bool          vbuffer_iterator_check_available(struct vbuffer_iterator *position, size_t minsize, size_t *size);
bool          vbuffer_iterator_register(struct vbuffer_iterator *position);
bool          vbuffer_iterator_unregister(struct vbuffer_iterator *position);
bool          vbuffer_iterator_insert(struct vbuffer_iterator *position, struct vbuffer *buffer, struct vbuffer_sub *sub);
size_t        vbuffer_iterator_advance(struct vbuffer_iterator *position, size_t len);
bool          vbuffer_iterator_isend(struct vbuffer_iterator *position);
bool          vbuffer_iterator_iseof(struct vbuffer_iterator *position);
bool          vbuffer_iterator_split(struct vbuffer_iterator *position);
size_t        vbuffer_iterator_sub(struct vbuffer_iterator *position, size_t len, struct vbuffer_sub *sub, bool split);
uint8         vbuffer_iterator_getbyte(struct vbuffer_iterator *position);
bool          vbuffer_iterator_setbyte(struct vbuffer_iterator *position, uint8 byte);
uint8        *vbuffer_iterator_mmap(struct vbuffer_iterator *position, size_t maxsize, size_t *size, bool write);
bool          vbuffer_iterator_mark(struct vbuffer_iterator *position, bool readonly);
bool          vbuffer_iterator_unmark(struct vbuffer_iterator *position);
bool          vbuffer_iterator_isinsertable(struct vbuffer_iterator *position, struct vbuffer *buffer);

void          vbuffer_sub_clear(struct vbuffer_sub *data);
bool          vbuffer_sub_register(struct vbuffer_sub *data);
bool          vbuffer_sub_unregister(struct vbuffer_sub *data);
void          vbuffer_sub_create(struct vbuffer_sub *data, struct vbuffer *buffer, size_t offset, size_t length);
bool          vbuffer_sub_create_from_position(struct vbuffer_sub *data, struct vbuffer_iterator *position, size_t length);
bool          vbuffer_sub_create_between_position(struct vbuffer_sub *data, struct vbuffer_iterator *begin, struct vbuffer_iterator *end);
void          vbuffer_sub_begin(struct vbuffer_sub *data, struct vbuffer_iterator *begin);
void          vbuffer_sub_end(struct vbuffer_sub *data, struct vbuffer_iterator *end);
bool          vbuffer_sub_position(struct vbuffer_sub *data, struct vbuffer_iterator *iter, size_t offset);
bool          vbuffer_sub_sub(struct vbuffer_sub *data, size_t offset, size_t length, struct vbuffer_sub *buffer);
size_t        vbuffer_sub_size(struct vbuffer_sub *data);
bool          vbuffer_sub_check_size(struct vbuffer_sub *data, size_t minsize, size_t *size);
size_t        vbuffer_sub_read(struct vbuffer_sub *data, uint8 *ptr, size_t size);
size_t        vbuffer_sub_write(struct vbuffer_sub *data, const uint8 *ptr, size_t size);
const uint8  *vbuffer_sub_flatten(struct vbuffer_sub *data, size_t *size);
bool          vbuffer_sub_compact(struct vbuffer_sub *data);
bool          vbuffer_sub_isflat(struct vbuffer_sub *data);
bool          vbuffer_sub_clone(struct vbuffer_sub *data, struct vbuffer *buffer, bool copy);

uint8        *vbuffer_mmap(struct vbuffer_sub *data, size_t *len, bool write, struct vbuffer_sub_mmap *iter);
bool          vbuffer_zero(struct vbuffer_sub *data);
bool          vbuffer_extract(struct vbuffer_sub *data, struct vbuffer *buffer);
bool          vbuffer_select(struct vbuffer_sub *data, struct vbuffer *buffer, struct vbuffer_iterator *ref);
bool          vbuffer_restore(struct vbuffer_iterator *position, struct vbuffer *data);
bool          vbuffer_erase(struct vbuffer_sub *data);
bool          vbuffer_replace(struct vbuffer_sub *data, struct vbuffer *buffer);

int64         vbuffer_asnumber(struct vbuffer_sub *data, bool bigendian);
bool          vbuffer_setnumber(struct vbuffer_sub *data, bool bigendian, int64 num);
int64         vbuffer_asbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian);
bool          vbuffer_setbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian, int64 num);
size_t        vbuffer_asstring(struct vbuffer_sub *data, char *str, size_t len);
size_t        vbuffer_setfixedstring(struct vbuffer_sub *data, const char *str, size_t len);
bool          vbuffer_setstring(struct vbuffer_sub *data, const char *str, size_t len);
uint8         vbuffer_getbyte(struct vbuffer_sub *data, size_t offset);
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
