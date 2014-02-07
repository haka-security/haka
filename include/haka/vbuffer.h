/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_VBUFFER_H
#define _HAKA_VBUFFER_H

#include <haka/types.h>
#include <haka/container/list.h>
#include <haka/lua/object.h>


/*
 * Buffer
 */

#define ALL   (size_t)-1

struct vbuffer_iterator;
struct vbuffer_data;
struct vsubbuffer;

struct vbuffer_data_ops {
	void   (*free)(struct vbuffer_data *data);
	void   (*addref)(struct vbuffer_data *data);
	bool   (*release)(struct vbuffer_data *data);
	uint8 *(*get)(struct vbuffer_data *data, bool write);
};

struct vbuffer_data {
	struct vbuffer_data_ops  *ops;
};

struct vbuffer_flags {
	bool                     modified:1;
	bool                     writable:1;
	bool                     ctl:1;
};

struct vbuffer {
	struct lua_object        lua_object;
	struct vbuffer          *next;
	size_t                   length;
	size_t                   offset;
	struct vbuffer_data     *data;
	struct vbuffer_flags     flags;
	struct vbuffer_iterator *iterators;
};


struct vbuffer *vbuffer_create_new(size_t size);
struct vbuffer *vbuffer_create_from_string(const char *str, size_t len);
struct vbuffer *vbuffer_create_from_data(struct vbuffer_data *data, size_t offset, size_t length);
struct vbuffer *vbuffer_extract(struct vbuffer *buf, size_t offset, size_t length);
struct vbuffer *vbuffer_select(struct vbuffer *buf, size_t offset, size_t length, struct vbuffer **ref);
bool            vbuffer_restore(struct vbuffer *node, struct vbuffer *data);
void            vbuffer_setmode(struct vbuffer *buf, bool readonly);
void            vbuffer_free(struct vbuffer *buf);
bool            vbuffer_insert(struct vbuffer *buf, size_t offset, struct vbuffer *data);
bool            vbuffer_erase(struct vbuffer *buf, size_t offset, size_t len);
bool            vbuffer_flatten(struct vbuffer *buf);
bool            vbuffer_compact(struct vbuffer *buf);
bool            vbuffer_isflat(struct vbuffer *buf);
size_t          vbuffer_size(struct vbuffer *buf);
bool            vbuffer_checksize(struct vbuffer *buf, size_t minsize, size_t *size);
uint8          *vbuffer_mmap(struct vbuffer *buf, void **iter, size_t *len, bool write);
uint8           vbuffer_getbyte(struct vbuffer *buf, size_t offset);
void            vbuffer_setbyte(struct vbuffer *buf, size_t offset, uint8 byte);
bool            vbuffer_ismodified(struct vbuffer *buf);
void            vbuffer_clearmodified(struct vbuffer *buf);
bool            vbuffer_zero(struct vbuffer *buf, bool mark_modified);


/*
 * Iterator
 */

struct vbuffer_iterator {
	struct list     list;
	struct vbuffer *buffer;
	size_t          offset;
	bool            post:1;
	bool            readonly:1;
	bool            registered:1;
};

bool            vbuffer_iterator(struct vbuffer *buf, struct vbuffer_iterator *iter, size_t offset, bool post, bool readonly);
bool            vbuffer_iterator_copy(const struct vbuffer_iterator *src, struct vbuffer_iterator *dst);
bool            vbuffer_iterator_register(struct vbuffer_iterator *iter);
bool            vbuffer_iterator_unregister(struct vbuffer_iterator *iter);
bool            vbuffer_iterator_clear(struct vbuffer_iterator *iter);
size_t          vbuffer_iterator_available(struct vbuffer_iterator *iter);
bool            vbuffer_iterator_check_available(struct vbuffer_iterator *iter, size_t size);
bool            vbuffer_iterator_sub(struct vbuffer_iterator *iter, struct vsubbuffer *buffer, size_t len, bool advance);
size_t          vbuffer_iterator_read(struct vbuffer_iterator *iter, uint8 *buffer, size_t len, bool advance);
size_t          vbuffer_iterator_write(struct vbuffer_iterator *iter, uint8 *buffer, size_t len, bool advance);
bool            vbuffer_iterator_insert(struct vbuffer_iterator *iter, struct vbuffer *data);
bool            vbuffer_iterator_erase(struct vbuffer_iterator *iter, size_t len);
bool            vbuffer_iterator_replace(struct vbuffer_iterator *iter, size_t len, struct vbuffer *data);
size_t          vbuffer_iterator_advance(struct vbuffer_iterator *iter, size_t len);


/*
 * Sub buffer
 */

struct vsubbuffer {
	struct vbuffer_iterator  position;
	size_t                   length;
};

bool            vbuffer_sub(struct vbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub);
bool            vsubbuffer_sub(struct vsubbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub);
bool            vsubbuffer_isflat(struct vsubbuffer *buf);
bool            vsubbuffer_flatten(struct vsubbuffer *buf);
size_t          vsubbuffer_size(struct vsubbuffer *buf);
uint8          *vsubbuffer_mmap(struct vsubbuffer *buf, void **iter, size_t *remlen, size_t *len, bool write);
int64           vsubbuffer_asnumber(struct vsubbuffer *buf, bool bigendian);
void            vsubbuffer_setnumber(struct vsubbuffer *buf, bool bigendian, int64 num);
int64           vsubbuffer_asbits(struct vsubbuffer *buf, size_t offset, size_t bits, bool bigendian);
void            vsubbuffer_setbits(struct vsubbuffer *buf, size_t offset, size_t bits, bool bigendian, int64 num);
size_t          vsubbuffer_asstring(struct vsubbuffer *buf, char *str, size_t len);
size_t          vsubbuffer_setfixedstring(struct vsubbuffer *buf, const char *str, size_t len);
void            vsubbuffer_setstring(struct vsubbuffer *buf, const char *str, size_t len);
uint8           vsubbuffer_getbyte(struct vsubbuffer *buf, size_t offset);
void            vsubbuffer_setbyte(struct vsubbuffer *buf, size_t offset, uint8 byte);


/*
 * Buffer stream
 */

struct vbuffer_stream_chunk;

struct vbuffer_stream {
	struct lua_object            lua_object;
	struct vbuffer              *data;
	struct vbuffer_stream_chunk *first;
	struct vbuffer_stream_chunk *last;
	struct vbuffer_stream_chunk *read_first;
	struct vbuffer_stream_chunk *read_last;
};

struct vbuffer_stream *vbuffer_stream();
void                   vbuffer_stream_free(struct vbuffer_stream *stream);
bool                   vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *data);
struct vbuffer        *vbuffer_stream_pop(struct vbuffer_stream *stream);
struct vbuffer        *vbuffer_stream_data(struct vbuffer_stream *stream);
bool                   vbuffer_stream_current(struct vbuffer_stream *stream, struct vbuffer_iterator *position);

#endif /* _HAKA_VBUFFER_H */
