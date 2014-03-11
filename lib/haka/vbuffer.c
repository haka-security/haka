/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <haka/vbuffer.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/thread.h>

#include "vbuffer.h"
#include "vbuffer_data.h"


/*
 * Buffer data
 */

static void vbuffer_data_release(struct vbuffer_data *data)
{
	if (data && data->ops->release(data)) {
		data->ops->free(data);
	}
}


/*
 * Buffer chunk
 */

static void vbuffer_chunk_addref(struct vbuffer_chunk *chunk)
{
	atomic_inc(&chunk->ref);
}

static void vbuffer_chunk_release(struct vbuffer_chunk *chunk)
{
	if (atomic_dec(&chunk->ref) == 0) {
		assert(!chunk->data);
		assert(!list2_elem_check(&chunk->list));
		free(chunk);
	}
}

void vbuffer_chunk_clear(struct vbuffer_chunk *chunk)
{
	if (chunk->list.next) {
		assert(chunk->list.prev);
		list2_erase(&chunk->list);
	}

	vbuffer_data_release(chunk->data);
	chunk->data = NULL;

	vbuffer_chunk_release(chunk);
}

static struct vbuffer_chunk *vbuffer_chunk_create_end(bool writable)
{
	struct vbuffer_chunk *chunk = malloc(sizeof(struct vbuffer_chunk));
	if (!chunk) {
		error(L"memory error");
		return NULL;
	}

	atomic_set(&chunk->ref, 0);
	chunk->size = 0;
	chunk->offset = 0;
	chunk->data = NULL;
	chunk->flags.modified = false;
	chunk->flags.writable = writable;
	chunk->flags.ctl = true;
	chunk->flags.end = true;
	chunk->flags.eof = true;

	list2_elem_init(&chunk->list);
	vbuffer_chunk_addref(chunk);

	chunk->list.next = chunk->list.prev = &chunk->list;
#ifdef HAKA_DEBUG
	chunk->list.is_end = true;
#endif
	return chunk;
}

struct vbuffer_chunk *vbuffer_chunk_clone(struct vbuffer_chunk *chunk, bool copy)
{
	struct vbuffer_chunk *clone = vbuffer_chunk_create(chunk->data, chunk->offset, chunk->size);
	if (!clone) return NULL;

	clone->flags = chunk->flags;
	return clone;
}

struct vbuffer_chunk *vbuffer_chunk_create(struct vbuffer_data *data, size_t offset,
		size_t length)
{
	struct vbuffer_chunk *chunk = malloc(sizeof(struct vbuffer_chunk));
	if (!chunk) {
		if (data) data->ops->free(data);
		error(L"memory error");
		return NULL;
	}

	atomic_set(&chunk->ref, 0);
	chunk->size = length;
	chunk->offset = offset;
	chunk->data = data;
	chunk->flags.modified = false;
	chunk->flags.writable = true;
	chunk->flags.ctl = false;
	chunk->flags.end = false;
	chunk->flags.eof = false;
	if (data) data->ops->addref(data);
	vbuffer_chunk_addref(chunk);

	list2_elem_init(&chunk->list);
	return chunk;
}

struct vbuffer_chunk *vbuffer_chunk_insert_ctl(struct vbuffer_chunk *insert, struct vbuffer_data *data)
{
	struct vbuffer_chunk *chunk;

	assert(data);
	assert(insert);

	chunk = malloc(sizeof(struct vbuffer_chunk));
	if (!chunk) {
		if (data) data->ops->free(data);
		error(L"memory error");
		return NULL;
	}

	atomic_set(&chunk->ref, 0);
	chunk->size = 0;
	chunk->offset = 0;
	chunk->data = data;
	chunk->flags.modified = false;
	chunk->flags.writable = insert->flags.writable;
	chunk->flags.ctl = true;
	chunk->flags.end = false;
	chunk->flags.eof = false;
	if (data) data->ops->addref(data);
	vbuffer_chunk_addref(chunk);

	list2_elem_init(&chunk->list);
	list2_insert(&insert->list, &chunk->list);

	return chunk;
}

INLINE bool vbuffer_chunk_check_writeable(struct vbuffer_chunk *chunk)
{
	if (!chunk->flags.writable) {
		error(L"read only buffer");
		return false;
	}
	return true;
}

INLINE void vbuffer_chunk_mark_modified(struct vbuffer_chunk *chunk)
{
	chunk->flags.modified = true;
}

static uint8 *vbuffer_chunk_get_data(struct vbuffer_chunk *chunk, bool write)
{
	uint8 *ptr;

	if (write && !vbuffer_chunk_check_writeable(chunk)) {
		return NULL;
	}

	assert(!chunk->flags.ctl);

	ptr = chunk->data->ops->get(chunk->data, write);
	if (!ptr) {
		assert(write); /* read should always be successful */
		assert(check_error());
		return NULL;
	}

	if (write) vbuffer_chunk_mark_modified(chunk);
	return ptr + chunk->offset;
}

struct vbuffer_chunk *vbuffer_chunk_remove_ctl(struct vbuffer_chunk *chunk)
{
	struct vbuffer_chunk *next;

	assert(chunk);
	assert(chunk->flags.ctl);

	next = vbuffer_chunk_next(chunk);

	next->flags.modified |= chunk->flags.modified;
	vbuffer_chunk_clear(chunk);

	return next;
}


/*
 * Buffer
 */

const struct vbuffer vbuffer_init = { LUA_OBJECT_INIT, NULL };

static bool _vbuffer_init(struct vbuffer *buffer, bool writable)
{
	assert(buffer);
	buffer->lua_object = lua_object_init;
	buffer->chunks = vbuffer_chunk_create_end(writable);
	return buffer->chunks != NULL;
}

bool vbuffer_isvalid(const struct vbuffer *buffer)
{
	assert(buffer);
	return buffer->chunks != NULL;
}

bool vbuffer_create_empty(struct vbuffer *buffer)
{
	return _vbuffer_init(buffer, true);
}

struct list2 *vbuffer_chunk_list(const struct vbuffer *buf)
{
	assert(buf);
	assert(buf->chunks);
	return (struct list2 *)&buf->chunks->list;
}

struct vbuffer_chunk *vbuffer_chunk_begin(const struct vbuffer *buf)
{
	assert(buf);
	assert(buf->chunks);
	return list2_get(list2_next(&buf->chunks->list), struct vbuffer_chunk, list);
}

struct vbuffer_chunk *vbuffer_chunk_end(const struct vbuffer *buf)
{
	assert(buf);
	assert(buf->chunks);
	return buf->chunks;
}

static bool vbuffer_create_from_data(struct vbuffer *buffer, struct vbuffer_data *data, size_t offset, size_t length)
{
	struct vbuffer_chunk *chunk = vbuffer_chunk_create(data, offset, length);
	if (!chunk) {
		return false;
	}

	_vbuffer_init(buffer, true);
	list2_insert(&vbuffer_chunk_end(buffer)->list, &chunk->list);
	return true;
}

bool vbuffer_create_new(struct vbuffer *buffer, size_t size, bool zero)
{
	struct vbuffer_data_basic *data = vbuffer_data_basic(size, zero);
	if (!data) {
		return false;
	}

	if (!vbuffer_create_from_data(buffer, &data->super, 0, size)) {
		return false;
	}

	return true;
}

bool vbuffer_create_from(struct vbuffer *buffer, const char *str, size_t len)
{
	struct vbuffer_data_basic *data = vbuffer_data_basic(len, false);
	if (!data) {
		return false;
	}

	memcpy(data->buffer, str, len);

	if (!vbuffer_create_from_data(buffer, &data->super, 0, len)) {
		return false;
	}

	return true;
}

struct vbuffer_chunk *vbuffer_chunk_next(struct vbuffer_chunk *chunk)
{
	assert(chunk);

	if (chunk->flags.end) return NULL;

	return list2_get(list2_next(&chunk->list), struct vbuffer_chunk, list);
}

void vbuffer_clear(struct vbuffer *buf)
{
	if (vbuffer_isvalid(buf)) {
		struct vbuffer_chunk *iter = vbuffer_chunk_begin(buf);
		while (!iter->flags.end) {
			struct vbuffer_chunk *cur = iter;
			iter = vbuffer_chunk_next(iter);

			vbuffer_chunk_clear(cur);
		}

		vbuffer_chunk_clear(buf->chunks);
		buf->chunks = NULL;
	}
}

void vbuffer_release(struct vbuffer *buffer)
{
	vbuffer_clear(buffer);
	lua_object_release(buffer, &buffer->lua_object);
}

void vbuffer_position(const struct vbuffer *buf, struct vbuffer_iterator *position, size_t offset)
{
	assert(vbuffer_isvalid(buf));

	if (offset == ALL) {
		position->chunk = buf->chunks;
		position->offset = 0;
		position->registered = false;
	}
	else {
		position->chunk = vbuffer_chunk_begin(buf);
		position->offset = 0;
		position->registered = false;
		if (offset > 0) vbuffer_iterator_advance(position, offset);
	}
}

void vbuffer_setwritable(struct vbuffer *buf, bool writable)
{
	struct vbuffer_chunk *iter;

	assert(vbuffer_isvalid(buf));

	VBUFFER_FOR_EACH(buf, iter) {
		iter->flags.writable = writable;
	}
}

bool vbuffer_iswritable(struct vbuffer *buf)
{
	assert(vbuffer_isvalid(buf));
	return vbuffer_chunk_end(buf)->flags.writable;
}

bool vbuffer_ismodified(struct vbuffer *buf)
{
	struct vbuffer_chunk *iter;

	assert(vbuffer_isvalid(buf));

	VBUFFER_FOR_EACH(buf, iter) {
		if (iter->flags.modified) {
			return true;
		}
	}
	return false;
}

void vbuffer_clearmodified(struct vbuffer *buf)
{
	struct vbuffer_chunk *iter;

	assert(vbuffer_isvalid(buf));

	VBUFFER_FOR_EACH(buf, iter) {
		iter->flags.modified = false;
	}
}

bool vbuffer_append(struct vbuffer *buf, struct vbuffer *buffer)
{
	struct list2 *list = vbuffer_chunk_list(buffer);

	assert(vbuffer_isvalid(buf));

	vbuffer_chunk_mark_modified(vbuffer_chunk_end(buf));

	list2_insert_list(&vbuffer_chunk_end(buf)->list, list2_begin(list), list2_end(list));
	assert(list2_empty(list));

	return true;
}

void vbuffer_swap(struct vbuffer *a, struct vbuffer *b)
{
	struct vbuffer_chunk *tmp;

	assert(vbuffer_isvalid(b));

	tmp = a->chunks;
	a->chunks = b->chunks;
	b->chunks = tmp;
}


/*
 * Iterators
 */

const struct vbuffer_iterator vbuffer_iterator_init = { NULL, 0, false };

static bool _vbuffer_iterator_check(const struct vbuffer_iterator *position)
{
	assert(position);

	if (!vbuffer_iterator_isvalid(position)) {
		error(L"empty iterator");
		return false;
	}

	if (position->registered) {
		if ((!position->chunk->data && !position->chunk->flags.end) ||
		    position->offset > position->chunk->size) {
			error(L"invalid buffer iterator");
			return false;
		}
	}

	return true;
}

bool vbuffer_iterator_isvalid(const struct vbuffer_iterator *position)
{
	assert(position);
	return position->chunk != NULL;
}

void vbuffer_iterator_copy(const struct vbuffer_iterator *src, struct vbuffer_iterator *dst)
{
	*dst = vbuffer_iterator_init;

	if (!_vbuffer_iterator_check(src)) return;

	dst->chunk = src->chunk;
	dst->offset = src->offset;
}

void vbuffer_iterator_clear(struct vbuffer_iterator *position)
{
	assert(position);

	if (position->chunk) {
		if (position->registered) {
			vbuffer_chunk_release(position->chunk);
			position->registered = false;
		}

		position->chunk = NULL;
	}
}

void vbuffer_iterator_update(struct vbuffer_iterator *position, struct vbuffer_chunk *chunk, size_t offset)
{
	assert(position);
	assert(chunk);

	if (position->chunk != chunk) {
		if (position->registered) {
			vbuffer_chunk_release(position->chunk);
			vbuffer_chunk_addref(chunk);
		}

		position->chunk = chunk;
	}

	position->offset = offset;
}

static size_t _vbuffer_iterator_fix(struct vbuffer_iterator *position, struct vbuffer_chunk **riter)
{
	assert(position);
	assert(riter);

	if (position->offset > position->chunk->size) {
		size_t offset = position->offset;
		struct vbuffer_chunk *iter = position->chunk;
		while (!iter->flags.end) {
			if (offset > iter->size) {
				offset -= iter->size;
			}
			else {
				break;
			}

			iter = vbuffer_chunk_next(iter);
		}

		*riter = iter;
		return offset;
	}
	else {
		*riter = position->chunk;
		return position->offset;
	}
}

size_t vbuffer_iterator_available(struct vbuffer_iterator *position)
{
	size_t size = 0;
	vbuffer_iterator_check_available(position, (size_t)-1, &size);
	return size;
}

static bool _vbuffer_iterator_check_available(struct vbuffer_iterator *position, struct vbuffer_iterator *end, size_t minsize, size_t *size)
{
	struct vbuffer_chunk *iter, *enditer = NULL;
	size_t offset, endoffset = 0;
	size_t cursize = 0;

	if (!_vbuffer_iterator_check(position)) {
		if (size) *size = (size_t)-1;
		return false;
	}

	if (minsize == 0) {
		if (size) *size = minsize;
		return true;
	}

	offset = _vbuffer_iterator_fix(position, &iter);
	if (end) {
		if (!_vbuffer_iterator_check(end)) {
			if (size) *size = (size_t)-1;
			return false;
		}

		endoffset = _vbuffer_iterator_fix(end, &enditer);
	}

	while (!iter->flags.end) {
		if (enditer && enditer == iter) {
			if (offset > endoffset) {
				error(L"invalid buffer end");
				return false;
			}

			cursize += endoffset - offset;
			offset = 0;

			if (cursize >= minsize) {
				if (size) *size = minsize;
				return true;
			}

			break;
		}
		else {
			cursize += iter->size - offset;
			offset = 0;

			if (cursize >= minsize) {
				if (size) *size = minsize;
				return true;
			}
		}

		iter = vbuffer_chunk_next(iter);
	}

	if (size) *size = cursize;
	return false;
}

bool vbuffer_iterator_check_available(struct vbuffer_iterator *position, size_t minsize, size_t *size)
{
	return _vbuffer_iterator_check_available(position, NULL, minsize, size);
}

bool vbuffer_iterator_register(struct vbuffer_iterator *position)
{
	assert(position);

	if (!_vbuffer_iterator_check(position)) return false;

	if (!position->registered) {
		position->registered = true;
		vbuffer_chunk_addref(position->chunk);
	}

	return true;
}

bool vbuffer_iterator_unregister(struct vbuffer_iterator *position)
{
	assert(position);

	if (!_vbuffer_iterator_check(position)) return false;

	if (position->registered) {
		vbuffer_chunk_release(position->chunk);
		position->registered = false;
	}

	return true;
}

static struct vbuffer_chunk *_vbuffer_iterator_split(struct vbuffer_iterator *position, bool mark_modified)
{
	struct vbuffer_chunk *iter, *newchunk;
	size_t offset;

	assert(_vbuffer_iterator_check(position));

	offset = _vbuffer_iterator_fix(position, &iter);

	if (mark_modified) {
		if (!vbuffer_chunk_check_writeable(iter)) return NULL;
		vbuffer_chunk_mark_modified(iter);
	}

	if (offset == 0 || iter->flags.end) {
		return iter;
	}

	if (offset == iter->size) {
		return vbuffer_chunk_next(iter);
	}

	/* Split and create a new chunk */
	assert(!iter->flags.ctl);

	newchunk = vbuffer_chunk_create(iter->data,
			iter->offset + offset,
			iter->size - offset);

	iter->size = offset;
	newchunk->flags = iter->flags;
	list2_insert(list2_next(&iter->list), &newchunk->list);

	return newchunk;
}

bool vbuffer_iterator_insert(struct vbuffer_iterator *position, struct vbuffer *buffer)
{
	struct list2 *list;
	struct vbuffer_chunk *insert;

	if (!_vbuffer_iterator_check(position)) return false;

	assert(vbuffer_isvalid(buffer));

	list = vbuffer_chunk_list(buffer);
	if (list2_empty(list)) return true;

	insert = _vbuffer_iterator_split(position, true);
	if (!insert) return false;

	list2_insert_list(&insert->list, list2_begin(list), list2_end(list));

	vbuffer_iterator_update(position, insert, 0);

	assert(list2_empty(list));
	return true;
}

size_t vbuffer_iterator_advance(struct vbuffer_iterator *position, size_t len)
{
	size_t clen = len;
	struct vbuffer_chunk *iter;
	size_t offset, endoffset = 0;

	if (!_vbuffer_iterator_check(position)) return (size_t)-1;

	offset = _vbuffer_iterator_fix(position, &iter);
	assert(iter);

	while (!iter->flags.end) {
		if (offset > iter->size) {
			offset -= iter->size;
		}
		else {
			const size_t buflen = iter->size - offset;
			if (buflen >= clen) {
				endoffset = offset + clen;
				clen = 0;
				break;
			}

			clen -= buflen;
			offset = 0;
		}

		iter = vbuffer_chunk_next(iter);
	}

	vbuffer_iterator_update(position, iter, endoffset);
	return (len - clen);
}

bool vbuffer_iterator_split(struct vbuffer_iterator *position)
{
	if (!_vbuffer_iterator_check(position)) return false;

	struct vbuffer_chunk *iter = _vbuffer_iterator_split(position, false);
	if (!iter) return false;

	vbuffer_iterator_update(position, iter, 0);
	return true;
}

size_t vbuffer_iterator_sub(struct vbuffer_iterator *position, size_t len, struct vbuffer_sub *sub, bool split)
{
	struct vbuffer_iterator begin;

	if (!_vbuffer_iterator_check(position)) return -1;

	vbuffer_iterator_copy(position, &begin);
	len = vbuffer_iterator_advance(position, len);

	if (split) {
		struct vbuffer_chunk *iter = _vbuffer_iterator_split(position, false);
		if (!iter) {
                        assert(check_error());
                        vbuffer_iterator_update(position, begin.chunk, begin.offset);
			vbuffer_iterator_clear(&begin);
			return -1;
		}

		vbuffer_iterator_update(position, iter, 0);
	}

	const bool ret = vbuffer_sub_create_from_position(sub, &begin, len);
        if (!ret) len = -1;
	vbuffer_iterator_clear(&begin);

	return len;
}

uint8 vbuffer_iterator_getbyte(struct vbuffer_iterator *position)
{
	uint8 *ptr;
	size_t length;

	if (!_vbuffer_iterator_check(position)) return -1;

	ptr = vbuffer_iterator_mmap(position, 1, &length, false);
	if (!ptr || length < 1) return -1;

	return *ptr;

}

bool vbuffer_iterator_setbyte(struct vbuffer_iterator *position, uint8 byte)
{
	uint8 *ptr;
	size_t length;

	if (!_vbuffer_iterator_check(position)) return false;

	ptr = vbuffer_iterator_mmap(position, 1, &length, true);
	if (!ptr || length < 1) return false;

	*ptr = byte;
	return true;
}

bool vbuffer_iterator_isend(struct vbuffer_iterator *position)
{
	struct vbuffer_chunk *iter;
	UNUSED size_t offset;

	if (!_vbuffer_iterator_check(position)) return false;

	offset = _vbuffer_iterator_fix(position, &iter);

	if (iter->flags.end) {
		assert(offset == 0);
		return true;
	}

	return false;
}

bool vbuffer_iterator_iseof(struct vbuffer_iterator *position)
{
	struct vbuffer_chunk *iter;
	UNUSED size_t offset;

	if (!_vbuffer_iterator_check(position)) return false;

	offset = _vbuffer_iterator_fix(position, &iter);

	if (iter->flags.end && iter->flags.eof) {
		assert(offset == 0);
		return true;
	}

	return false;
}

uint8 *vbuffer_iterator_mmap(struct vbuffer_iterator *position, size_t maxsize, size_t *size, bool write)
{
	struct vbuffer_chunk *iter;
	size_t offset;

	if (!_vbuffer_iterator_check(position)) return NULL;

	if (maxsize == 0) return NULL;

	offset = _vbuffer_iterator_fix(position, &iter);

	while (!iter->flags.end) {
		size_t len = iter->size;

		assert(offset <= len);
		len -= offset;

		if (!iter->flags.ctl && len > 0) {
			if (maxsize != ALL && len > maxsize) {
				if (size) *size = maxsize;
				vbuffer_iterator_update(position, iter, offset + maxsize);
			}
			else {
				if (size) *size = len;
				vbuffer_iterator_update(position, vbuffer_chunk_next(iter), 0);
			}
			return vbuffer_chunk_get_data(iter, write) + offset;
		}

		offset = 0;
		iter = vbuffer_chunk_next(iter);
	}

	return NULL;
}

bool vbuffer_iterator_mark(struct vbuffer_iterator *position, bool readonly)
{
	struct vbuffer_data_ctl_mark *mark;
	struct vbuffer_chunk *chunk;
	struct vbuffer_chunk *insert;

	if (!_vbuffer_iterator_check(position)) return false;

	insert = _vbuffer_iterator_split(position, false);
	if (!insert) return false;

	mark = vbuffer_data_ctl_mark(readonly);
	if (!mark) {
		return false;
	}

	chunk = vbuffer_chunk_insert_ctl(insert, &mark->super.super);
	if (!chunk) return false;

	vbuffer_iterator_update(position, chunk, 0);
	return true;
}

bool vbuffer_iterator_unmark(struct vbuffer_iterator *position)
{
	struct vbuffer_data_ctl_mark *mark;
	struct vbuffer_chunk *next;

	if (!_vbuffer_iterator_check(position)) return false;

	mark = vbuffer_data_cast(position->chunk->data, vbuffer_data_ctl_mark);
	if (!mark) {
		error(L"iterator is not a mark");
		return false;
	}

	next = vbuffer_chunk_remove_ctl(position->chunk);

	vbuffer_iterator_update(position, next, 0);
	return true;
}

bool vbuffer_iterator_isinsertable(struct vbuffer_iterator *position, struct vbuffer *buffer)
{
	struct vbuffer_chunk *iter = vbuffer_chunk_begin(buffer);

	if (!_vbuffer_iterator_check(position)) return false;

	while (iter) {
		if (iter == position->chunk) return false;
		iter = vbuffer_chunk_next(iter);
	}

	return true;
}


/*
 * Sub buffer
 */

static bool _vbuffer_sub_check(const struct vbuffer_sub *data)
{
	assert(data);
	if (!_vbuffer_iterator_check(&data->begin)) return false;
	if (!data->use_size && !_vbuffer_iterator_check(&data->end)) return false;
	return true;
}

static void vbuffer_sub_init(struct vbuffer_sub *data)
{
	assert(data);
	data->begin = vbuffer_iterator_init;
	data->end = vbuffer_iterator_init;
	data->use_size = false;
}

void vbuffer_sub_clear(struct vbuffer_sub *data)
{
	vbuffer_iterator_clear(&data->begin);
	if (!data->use_size) vbuffer_iterator_clear(&data->end);
}

bool vbuffer_sub_register(struct vbuffer_sub *data)
{
	bool ret = vbuffer_iterator_register(&data->begin);
	if (!data->use_size) ret &= vbuffer_iterator_register(&data->end);
	return ret;
}

bool vbuffer_sub_unregister(struct vbuffer_sub *data)
{
	bool ret = vbuffer_iterator_unregister(&data->begin);
	if (!data->use_size) ret &= vbuffer_iterator_unregister(&data->end);
	return ret;
}

void vbuffer_sub_create(struct vbuffer_sub *data, struct vbuffer *buffer, size_t offset, size_t length)
{
	vbuffer_sub_init(data);

	vbuffer_begin(buffer, &data->begin);
	vbuffer_iterator_advance(&data->begin, offset);

	data->use_size = false;
	if (length == ALL) {
		vbuffer_end(buffer, &data->end);
	}
	else {
		vbuffer_iterator_copy(&data->begin, &data->end);
		vbuffer_iterator_advance(&data->end, length);
	}
}

bool vbuffer_sub_create_from_position(struct vbuffer_sub *data, struct vbuffer_iterator *position, size_t length)
{
	vbuffer_sub_init(data);

	vbuffer_iterator_copy(position, &data->begin);
	data->use_size = true;
	data->length = length;
	return true;
}

bool vbuffer_sub_create_between_position(struct vbuffer_sub *data, struct vbuffer_iterator *begin, struct vbuffer_iterator *end)
{
	vbuffer_sub_init(data);

	vbuffer_iterator_copy(begin, &data->begin);
	data->use_size = false;
	vbuffer_iterator_copy(end, &data->end);
	return true;
}

void vbuffer_sub_begin(struct vbuffer_sub *data, struct vbuffer_iterator *begin)
{
	vbuffer_iterator_copy(&data->begin, begin);
}

void vbuffer_sub_end(struct vbuffer_sub *data, struct vbuffer_iterator *end)
{
	vbuffer_iterator_copy(&data->end, end);
}

bool vbuffer_sub_position(struct vbuffer_sub *data, struct vbuffer_iterator *iter, size_t offset)
{
	assert(data);
	assert(iter);

	if (offset == ALL) {
		if (data->use_size) {
			vbuffer_iterator_copy(&data->begin, iter);
			vbuffer_iterator_advance(iter, data->length);
		}
		else vbuffer_iterator_copy(&data->end, iter);
	}
	else {
		vbuffer_iterator_copy(&data->begin, iter);
		if (offset > 0) vbuffer_iterator_advance(iter, offset);
	}
	return true;
}

bool vbuffer_sub_sub(struct vbuffer_sub *data, size_t offset, size_t length, struct vbuffer_sub *buffer)
{
	assert(buffer);

	vbuffer_sub_position(data, &buffer->begin, offset);
	if (length == ALL) {
		buffer->use_size = false;
		vbuffer_sub_position(data, &buffer->end, ALL);
	}
	else {
		buffer->use_size = true;
		buffer->length = length;
	}
	return true;
}

size_t vbuffer_sub_size(struct vbuffer_sub *data)
{
	size_t size;
	vbuffer_sub_check_size(data, (size_t)-1, &size);
	return size;
}

bool vbuffer_sub_check_size(struct vbuffer_sub *data, size_t minsize, size_t *size)
{
	assert(data);

	if (!_vbuffer_sub_check(data)) return false;

	if (data->use_size) {
		if (minsize == ALL) minsize = data->length;
		return vbuffer_iterator_check_available(&data->begin, minsize, size);
	}
	else {
		return _vbuffer_iterator_check_available(&data->begin, &data->end, minsize, size);
	}
}

size_t vbuffer_sub_read(struct vbuffer_sub *data, uint8 *dst, size_t size)
{
	struct vbuffer_sub_mmap mmapiter = vbuffer_mmap_init;
	uint8 *ptr;
	size_t len, ret = 0;

	if (!_vbuffer_sub_check(data)) return false;

	while (size > 0 && (ptr = vbuffer_mmap(data, &len, false, &mmapiter))) {
		if (len > size) len = size;

		memcpy(dst, ptr, len);
		dst += len;
		size -= len;
		ret += len;
	}

	return ret;
}

size_t vbuffer_sub_write(struct vbuffer_sub *data, const uint8 *src, size_t size)
{
	struct vbuffer_sub_mmap mmapiter = vbuffer_mmap_init;
	uint8 *ptr;
	size_t len, ret = 0;

	if (!_vbuffer_sub_check(data)) return false;

	while (size > 0 && (ptr = vbuffer_mmap(data, &len, true, &mmapiter))) {
		if (len > size) len = size;

		memcpy(ptr, src, len);
		src += len;
		size -= len;
		ret += len;
	}

	return ret;
}

const struct vbuffer_sub_mmap vbuffer_mmap_init = { NULL, 0 };

static struct vbuffer_chunk *_vbuffer_sub_iterate(struct vbuffer_sub *data, size_t *start, size_t *len, struct vbuffer_sub_mmap *iter)
{
	size_t offset = 0;
	struct vbuffer_chunk *chunk;

	assert(iter);
	assert(len);

	if (!iter->data) {
		/* Initialize iterator data */
		struct vbuffer_iterator begin;
		vbuffer_sub_begin(data, &begin);

		offset = _vbuffer_iterator_fix(&begin, &iter->data);
		if (data->use_size) iter->len = data->length;
		else iter->len = (vbsize_t)-1;

		vbuffer_iterator_clear(&begin);
	}

	/* Check if we are at the end of the sub buffer */
	if (iter->data->flags.end || iter->len == 0) {
		*len = 0;
		return NULL;
	}

	chunk = iter->data;
	iter->data = vbuffer_chunk_next(iter->data);

	assert(offset <= chunk->size);
	*start = offset;

	*len = chunk->size - offset;

	if (data->use_size) {
		if (iter->len >= *len) {
			iter->len -= *len;
		}
		else {
			*len = iter->len;
			iter->len = 0;
		}
	}
	else {
		size_t endoffset;
		struct vbuffer_chunk *end = NULL;
		endoffset = _vbuffer_iterator_fix(&data->end, &end);

		if (chunk == end) {
			if (endoffset < offset) {
				error(L"invalid buffer end");
				return NULL;
			}
			else {
				*len = endoffset - offset;

				/* Set len to 0 to mark for the iterator end */
				iter->len = 0;
			}
		}
	}

	return chunk;
}

uint8 *vbuffer_mmap(struct vbuffer_sub *data, size_t *len, bool write, struct vbuffer_sub_mmap *iter)
{
	size_t offset;
	struct vbuffer_chunk *chunk;
	struct vbuffer_sub_mmap _iter;

	if (!iter) {
		_iter = vbuffer_mmap_init;
		iter = &_iter;
	}

	while ((chunk = _vbuffer_sub_iterate(data, &offset, len, iter))) {
		uint8 *ptr;

		if (chunk->flags.ctl || *len == 0) {
			continue;
		}

		ptr = vbuffer_chunk_get_data(chunk, write);
		if (!ptr) {
			return NULL;
		}

		assert(offset <= chunk->size);
		ptr += offset;

		return ptr;
	}

	return NULL;
}

bool vbuffer_zero(struct vbuffer_sub *data)
{
	struct vbuffer_sub_mmap mmapiter = vbuffer_mmap_init;
	uint8 *ptr;
	size_t len;

	if (!_vbuffer_sub_check(data)) return false;

	while ((ptr = vbuffer_mmap(data, &len, true, &mmapiter))) {
		memset(ptr, 0, len);
	}

	return check_error();
}

static struct vbuffer_chunk *_vbuffer_extract(struct vbuffer_sub *data, struct vbuffer *buffer,
		bool mark_modified, bool insert_ctl)
{
	struct vbuffer_chunk *begin, *end = NULL, *iter;
	size_t size = 0;

	assert(data);
	assert(buffer);

	if (!_vbuffer_sub_check(data)) return NULL;

	if (!data->use_size) {
		/* Split end first to avoid invalidating the beginning iterator */
		struct vbuffer_iterator iter;
		vbuffer_sub_end(data, &iter);
		end = _vbuffer_iterator_split(&iter, mark_modified);
		vbuffer_iterator_clear(&iter);
		if (!end) return NULL;
	}
	else {
		size = data->length;
	}

	{
		struct vbuffer_iterator iter;
		vbuffer_sub_begin(data, &iter);
		begin = _vbuffer_iterator_split(&iter, mark_modified);
		vbuffer_iterator_clear(&iter);
		if (!begin) return NULL;
	}

	_vbuffer_init(buffer, begin->flags.writable);

	if (data->use_size || !insert_ctl) {
		list2_iter ctl_insert_iter = list2_prev(&begin->list);
		iter = begin;
		while (!iter->flags.end) {
			if (data->use_size) {
				if (size > iter->size) {
					size -= iter->size;
				}
				else {
					struct vbuffer_iterator splitpos;
					splitpos = vbuffer_iterator_init;
					splitpos.chunk = iter;
					splitpos.offset = size;
					end = _vbuffer_iterator_split(&splitpos, mark_modified);
					vbuffer_iterator_clear(&splitpos);
					if (!end) return NULL;
					break;
				}
			}
			else {
				if (iter == end) {
					break;
				}
			}

			if (iter->flags.ctl && !insert_ctl) {
				list2_iter eraseiter = list2_erase(&iter->list);
				ctl_insert_iter = list2_insert(list2_next(ctl_insert_iter), &iter->list);
				iter = list2_get(eraseiter, struct vbuffer_chunk, list);
			}
			else {
				iter = vbuffer_chunk_next(iter);
			}
		}

		if (!end) end = iter;
	}

	if (begin != end) {
		list2_insert_list(&vbuffer_chunk_end(buffer)->list, &begin->list, &end->list);
	}

	if (insert_ctl) {
		struct vbuffer_chunk *mark;
		struct vbuffer_data_ctl_select *ctl = vbuffer_data_ctl_select();
		if (!ctl) {
			vbuffer_clear(buffer);
			return NULL;
		}

		mark = vbuffer_chunk_insert_ctl(end, &ctl->super.super);
		if (!mark) {
			vbuffer_clear(buffer);
			return NULL;
		}

		return mark;
	}
	else {
		return end;
	}
}

bool vbuffer_extract(struct vbuffer_sub *data, struct vbuffer *buffer)
{
	const bool ret = _vbuffer_extract(data, buffer, true, false) != NULL;
	vbuffer_sub_clear(data);
	return ret;
}

bool vbuffer_select(struct vbuffer_sub *data, struct vbuffer *buffer, struct vbuffer_iterator *ref)
{
	struct vbuffer_chunk *iter = _vbuffer_extract(data, buffer, false, true);
	if (!iter) return false;

	if (ref) {
		*ref = vbuffer_iterator_init;
		ref->chunk = iter;
		ref->offset = 0;
	}

	vbuffer_sub_clear(data);
	return true;
}

bool vbuffer_restore(struct vbuffer_iterator *position, struct vbuffer *data)
{
	struct vbuffer_data_ctl_select *ctl;

	if (!_vbuffer_iterator_check(position)) return false;

	ctl = vbuffer_data_cast(position->chunk->data, vbuffer_data_ctl_select);
	if (!ctl) {
		error(L"invalid restore iterator");
		return false;
	}

	if (data) {
		list2_insert_list(&position->chunk->list, &vbuffer_chunk_begin(data)->list, &vbuffer_chunk_end(data)->list);
		vbuffer_clear(data);
	}

	vbuffer_chunk_remove_ctl(position->chunk);

	vbuffer_iterator_clear(position);
	return true;
}

bool vbuffer_erase(struct vbuffer_sub *data)
{
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_chunk *iter = _vbuffer_extract(data, &buffer, true, false);
	if (!iter) return false;

	vbuffer_release(&buffer);
	return true;
}

bool vbuffer_replace(struct vbuffer_sub *data, struct vbuffer *buffer)
{
	struct vbuffer_sub new;
	struct vbuffer erase = vbuffer_init;
	struct vbuffer_chunk *iter = _vbuffer_extract(data, &erase, true, false);
	if (!iter) return false;

	vbuffer_release(&erase);

	/* Update buffer range */
	vbuffer_sub_create(&new, buffer, 0, ALL);

	iter->flags.modified = true;

	list2_insert_list(&iter->list, &vbuffer_chunk_begin(buffer)->list, &vbuffer_chunk_end(buffer)->list);
	vbuffer_clear(buffer);

	/* Update buffer range */
	vbuffer_iterator_update(&data->begin, new.begin.chunk, new.begin.offset);
	if (data->use_size) {
		data->length = new.length;
	}
	else {
		vbuffer_iterator_update(&data->end, iter, 0);
	}

	return true;
}

bool vbuffer_sub_isflat(struct vbuffer_sub *data)
{
	size_t offset;
	struct vbuffer_chunk *iter;

	if (!_vbuffer_sub_check(data)) return false;

	offset = _vbuffer_iterator_fix(&data->begin, &iter);

	if (data->use_size) {
		return iter->size <= offset + data->length;
	}
	else {
		struct vbuffer_chunk *enditer;
		const size_t endoffset = _vbuffer_iterator_fix(&data->end, &enditer);
		return iter == enditer || (endoffset == 0 && vbuffer_chunk_next(iter) == enditer);
	}
}

static bool _vbuffer_compact(struct vbuffer_sub *data, bool *has_ctl, size_t *rsize)
{
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
	size_t offset, len, size;
	struct vbuffer_chunk *chunk, *prev = NULL;

	if (!_vbuffer_sub_check(data)) return false;

	size = 0;

	while ((chunk = _vbuffer_sub_iterate(data, &offset, &len, &iter))) {
		size += len;

		if (has_ctl && chunk->flags.ctl) *has_ctl = true;

		if (prev) {
			if (prev->data == chunk->data) {
				if (prev->offset + prev->size == chunk->offset) {
					if (!data->use_size && chunk == data->end.chunk) {
						vbuffer_iterator_update(&data->end, prev, prev->size + data->end.offset);
					}

					prev->size += chunk->size;
					prev->flags.modified |= chunk->flags.modified;
					prev->flags.writable &= chunk->flags.writable;

					vbuffer_chunk_clear(chunk);
					continue;
				}
			}
		}

		prev = chunk;
	}

	if (rsize) *rsize = size;

	return true;
}

static const uint8 *_vbuffer_sub_flatten(struct vbuffer_sub *data, size_t size)
{
	struct vbuffer flat = vbuffer_init;
	struct vbuffer_iterator flatbegin;
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
	size_t offset, len;
	struct vbuffer_chunk *chunk;
	uint8 *dst, *dsti;

	if (!vbuffer_create_new(&flat, size, false)) {
		return NULL;
	}

	vbuffer_begin(&flat, &flatbegin);
	dst = flatbegin.chunk->data->ops->get(flatbegin.chunk->data, true);
	assert(dst);

	dsti = dst;

	while ((chunk = _vbuffer_sub_iterate(data, &offset, &len, &iter))) {
		uint8 *ptr;

		assert(!chunk->flags.ctl);
		assert(size >= len);

		ptr = vbuffer_chunk_get_data(chunk, false);
		assert(ptr);
		assert(offset <= chunk->size);
		ptr += offset;

		memcpy(dsti, ptr, len);
		dsti += len;
		size -= len;
	}

	vbuffer_replace(data, &flat);
	vbuffer_release(&flat);

	return dst;
}

const uint8 *vbuffer_sub_flatten(struct vbuffer_sub *data, size_t *rsize)
{
	if (!_vbuffer_sub_check(data)) return NULL;

	if (!vbuffer_sub_isflat(data)) {
		bool has_ctl = false;
		size_t size;
		if (!_vbuffer_compact(data, &has_ctl, &size) || has_ctl) {
			error(L"buffer cannot be flatten");
			return NULL;
		}

		if (!vbuffer_sub_isflat(data)) {
			if (rsize) *rsize = size;
			return _vbuffer_sub_flatten(data, size);
		}
	}

	{
		struct vbuffer_iterator iter;
		vbuffer_iterator_copy(&data->begin, &iter);
		assert(vbuffer_sub_isflat(data));
		return vbuffer_iterator_mmap(&iter, ALL, rsize, false);
	}
}

bool vbuffer_sub_compact(struct vbuffer_sub *data)
{
	return _vbuffer_compact(data, NULL, NULL);
}

bool vbuffer_sub_clone(struct vbuffer_sub *data, struct vbuffer *buffer, bool copy)
{
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
	size_t offset, len, size;
	struct vbuffer_chunk *chunk;
	struct vbuffer_chunk *end;

	if (!_vbuffer_sub_check(data)) return false;

	_vbuffer_init(buffer, true);
	end = vbuffer_chunk_end(buffer);

	size = 0;

	while ((chunk = _vbuffer_sub_iterate(data, &offset, &len, &iter))) {
		if (!chunk->flags.ctl) {
			struct vbuffer_chunk *clone = vbuffer_chunk_clone(chunk, copy);
			list2_insert(&end->list, &clone->list);

			end->flags.writable = clone->flags.writable;
			size += clone->size;
		}
	}

	if (copy) {
		/* Use flatten to copy the data */
		_vbuffer_sub_flatten(data, size);
	}

	return true;
}

int64 vbuffer_asnumber(struct vbuffer_sub *data, bool bigendian)
{
	uint8 temp[8];
	const uint8 *ptr;
	const size_t length = vbuffer_sub_size(data);

	if (length > 8) {
		error(L"asnumber: unsupported size %zu", length);
		return 0;
	}

	if (vbuffer_sub_isflat(data)) {
		size_t mmaplen;
		struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
		ptr = vbuffer_mmap(data, &mmaplen, false, &iter);
		if (!ptr) {
			return 0;
		}

		assert(mmaplen == length);
	}
	else {
		vbuffer_sub_read(data, temp, length);
		ptr = temp;
	}

	switch (length) {
	case 1: return *ptr;
	case 2: return bigendian ? SWAP_FROM_BE(int16, *(int16*)ptr) : SWAP_FROM_LE(int16, *(int16*)ptr);
	case 4: return bigendian ? SWAP_FROM_BE(int32, *(int32*)ptr) : SWAP_FROM_LE(int32, *(int32*)ptr);
	case 8: return bigendian ? SWAP_FROM_BE(int64, *(int64*)ptr) : SWAP_FROM_LE(int64, *(int64*)ptr);
	default:
		error(L"asnumber: unsupported size %zu", length);
		return 0;
	}
}

bool vbuffer_setnumber(struct vbuffer_sub *data, bool bigendian, int64 num)
{
	const size_t length = vbuffer_sub_size(data);

	if (length > 8) {
		error(L"setnumber: unsupported size %zu", length);
		return false;
	}

	if ((num < 0 && (-num & ((1ULL << (length*8))-1)) != -num) ||
	    (num >= 0 && (num & ((1ULL << (length*8))-1)) != num)) {
		error(L"setnumber: invalid number, value does not fit in %d bytes", length);
		return false;
	}

	if (vbuffer_sub_isflat(data)) {
		size_t mmaplen;
		struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
		uint8 *ptr = vbuffer_mmap(data, &mmaplen, true, &iter);
		if (!ptr) {
			return false;
		}

		switch (length) {
		case 1: *ptr = num; break;
		case 2: *(int16*)ptr = bigendian ? SWAP_TO_BE(int16, (int16)num) : SWAP_TO_LE(int16, (int16)num); break;
		case 4: *(int32*)ptr = bigendian ? SWAP_TO_BE(int32, (int32)num) : SWAP_TO_LE(int32, (int32)num); break;
		case 8: *(int64*)ptr = bigendian ? SWAP_TO_BE(int64, num) : SWAP_TO_LE(int64, num); break;
		default:
			error(L"setnumber: unsupported size %zu", length);
			return false;
		}
	}
	else {
		union {
			int8  i8;
			int16 i16;
			int32 i32;
			int64 i64;
			uint8 data[8];
		} temp;

		switch (length) {
		case 1: temp.i8 = num; break;
		case 2: temp.i16 = bigendian ? SWAP_TO_BE(int16, (int16)num) : SWAP_TO_LE(int16, (int16)num); break;
		case 4: temp.i32 = bigendian ? SWAP_TO_BE(int32, (int32)num) : SWAP_TO_LE(int32, (int32)num); break;
		case 8: temp.i64 = bigendian ? SWAP_TO_BE(int64, num) : SWAP_TO_LE(int64, num); break;
		default:
			error(L"setnumber: unsupported size %zu", length);
			return false;
		}

		vbuffer_sub_write(data, temp.data, length);
	}

	return true;
}

static uint8 getbits(uint8 x, int off, int size, bool bigendian)
{
#ifdef HAKA_BIGENDIAN
	if (!bigendian)
#else
	if (bigendian)
#endif
		off = 8-off-size;

	return (x >> off) & ((1 << size)-1);
}

static uint8 setbits(uint8 x, int off, int size, uint8 v, bool bigendian)
{
#ifdef HAKA_BIGENDIAN
	if (!bigendian)
#else
	if (bigendian)
#endif
		off = 8-off-size;

	/* The first block erase the bits, the second one will set them */
	return (x & ~(((1 << size)-1) << off)) | ((v & ((1 << size)-1)) << off);
}

int64 vbuffer_asbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian)
{
	const size_t length = vbuffer_sub_size(data);
	uint8 temp[8];
	int64 ret = 0;
	int i, off = offset, shiftbits = 0;
	const size_t begin = offset >> 3;
	const size_t end = (offset + bits + 7) >> 3;
	offset &= ((1 << 3)-1);
	assert(offset < 8);

	if (begin >= length) {
		error(L"asbits: invalid bit offset");
		return -1;
	}

	if (end > length) {
		error(L"asbits: invalid bit size");
		return -1;
	}

	if (end > 8) {
		error(L"asbits: unsupported size");
		return -1;
	}

	vbuffer_sub_read(data, temp, end);

	for (i=begin; i<end; ++i) {
		const int size = bits > 8-off ? 8-off : bits;

		if (bigendian) {
			ret = (ret << size) | getbits(temp[i], off, size, true);
		}
		else {
			ret = ret | (getbits(temp[i], off, size, false) << shiftbits);
			shiftbits += size;
		}

		off = 0;
		bits -= size;
	}

	return ret;
}

bool vbuffer_setbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian, int64 num)
{
	if (bits > 64) {
		error(L"setbits: unsupported size %zu", bits);
		return false;
	}

	if ((num < 0 && (-num & ((1ULL << bits)-1)) != -num) ||
	    (num >= 0 && (num & ((1ULL << bits)-1)) != num)) {
		error(L"setbits: invalid number, value does not fit in %d bits", bits);
		return false;
	}

	{
		const size_t length = vbuffer_sub_size(data);
		uint8 temp[8];
		int i, off = offset;
		const size_t begin = offset >> 3;
		const size_t end = (offset + bits + 7) >> 3;
		offset &= ((1 << 3)-1);
		assert(offset < 8);

		if (begin >= length) {
			error(L"setbits: invalid bit offset");
			return -1;
		}

		if (end > length) {
			error(L"setbits: invalid bit size");
			return -1;
		}

		if (end > 8) {
			error(L"setbits: unsupported size");
			return false;
		}

		vbuffer_sub_read(data, temp, end);

		for (i=begin; i<end; ++i) {
			const int size = bits > 8-off ? 8-off : bits;
			bits -= size;

			if (bigendian) {
				temp[i] = setbits(temp[i], off, size, num >> bits, true);
			}
			else {
				temp[i] = setbits(temp[i], off, size, num, false);
				num >>= size;
			}

			off = 0;
		}

		vbuffer_sub_write(data, temp, end);
	}

	return true;
}

size_t vbuffer_asstring(struct vbuffer_sub *data, char *str, size_t len)
{
	size_t size = vbuffer_sub_read(data, (uint8 *)str, len);
	if (check_error()) return (size_t)-1;

	if (size == len) --size;
	str[size] = '\0';
	return size;
}

size_t vbuffer_setfixedstring(struct vbuffer_sub *data, const char *str, size_t len)
{
	return vbuffer_sub_write(data, (const uint8 *)str, len);
}

bool vbuffer_setstring(struct vbuffer_sub *data, const char *str, size_t len)
{
	bool ret;
	struct vbuffer newbuf = vbuffer_init;
	if (!vbuffer_create_from(&newbuf, str, len)) return false;
	ret = vbuffer_replace(data, &newbuf);
	vbuffer_release(&newbuf);
	return ret;
}

uint8 vbuffer_getbyte(struct vbuffer_sub *data, size_t offset)
{
	struct vbuffer_iterator iter;
	vbuffer_iterator_copy(&data->begin, &iter);
	vbuffer_iterator_advance(&iter, offset);
	return vbuffer_iterator_getbyte(&iter);
}

bool vbuffer_setbyte(struct vbuffer_sub *data, size_t offset, uint8 byte)
{
	struct vbuffer_iterator iter;
	vbuffer_iterator_copy(&data->begin, &iter);
	vbuffer_iterator_advance(&iter, offset);
	return vbuffer_iterator_setbyte(&iter, byte);
}
