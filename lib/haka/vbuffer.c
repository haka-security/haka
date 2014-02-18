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
	if (data->ops->release(data)) {
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
	if (data) data->ops->addref(data);
	vbuffer_chunk_addref(chunk);

	list2_elem_init(&chunk->list);
	return chunk;
}

struct vbuffer_chunk *vbuffer_chunk_create_ctl(struct vbuffer_data *data)
{
	struct vbuffer_chunk *chunk = malloc(sizeof(struct vbuffer_chunk));
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
	chunk->flags.writable = true;
	chunk->flags.ctl = true;
	if (data) data->ops->addref(data);
	vbuffer_chunk_addref(chunk);

	list2_elem_init(&chunk->list);
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


/*
 * Buffer
 */

const struct vbuffer vbuffer_init = { LUA_OBJECT_INIT, LIST2_INIT };

static void _vbuffer_init(struct vbuffer *buffer)
{
	buffer->lua_object = lua_object_init;
	list2_init(&buffer->chunks);
}

bool vbuffer_isvalid(const struct vbuffer *buffer)
{
	return buffer->chunks.head.next != NULL;
}

bool vbuffer_create_empty(struct vbuffer *buffer)
{
	_vbuffer_init(buffer);
	return true;
}

static bool vbuffer_create_from_data(struct vbuffer *buffer, struct vbuffer_data *data, size_t offset, size_t length)
{
	struct vbuffer_chunk *chunk = vbuffer_chunk_create(data, offset, length);
	if (!chunk) {
		return false;
	}

	_vbuffer_init(buffer);
	list2_insert(list2_end(&buffer->chunks), &chunk->list);
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

void vbuffer_clear(struct vbuffer *buf)
{
	if (vbuffer_isvalid(buf)) {
		list2_iter iter, end;

		iter = list2_begin(&buf->chunks);
		end = list2_end(&buf->chunks);
		while (iter != end) {
			struct vbuffer_chunk *cur = list2_get(iter, struct vbuffer_chunk);
			iter = list2_next(iter);

			vbuffer_chunk_clear(cur);
		}

		memset(buf, 0, sizeof(struct vbuffer));
	}
}

void vbuffer_release(struct vbuffer *buffer)
{
	vbuffer_clear(buffer);
	lua_object_release(buffer, &buffer->lua_object);
}

bool vbuffer_position(const struct vbuffer *buf, struct vbuffer_iterator *position, size_t offset)
{
	assert(vbuffer_isvalid(buf));

	if (!list2_empty(&buf->chunks)) {
		if (offset == ALL) {
			position->chunk = list2_get(list2_prev(list2_end(&buf->chunks)), struct vbuffer_chunk);
			position->offset = (vbsize)-1;
			position->registered = false;
			return true;
		}
		else {
			position->chunk = list2_get(list2_begin(&buf->chunks), struct vbuffer_chunk);
			position->offset = 0;
			position->registered = false;
			vbuffer_iterator_advance(position, offset);
			return true;
		}
	}
	else {
		return false;
	}
}

void vbuffer_setmode(struct vbuffer *buf, bool readonly)
{
	list2_iter iter, end;

	assert(vbuffer_isvalid(buf));

	iter = list2_begin(&buf->chunks);
	end = list2_end(&buf->chunks);
	while (iter != end) {
		struct vbuffer_chunk *cur = list2_get(iter, struct vbuffer_chunk);
		cur->flags.writable = !readonly;
		iter = list2_next(iter);
	}
}

bool vbuffer_ismodified(struct vbuffer *buf)
{
	list2_iter iter, end;

	assert(vbuffer_isvalid(buf));

	iter = list2_begin(&buf->chunks);
	end = list2_end(&buf->chunks);
	while (iter != end) {
		struct vbuffer_chunk *cur = list2_get(iter, struct vbuffer_chunk);
		if (cur->flags.modified) {
			return true;
		}
		iter = list2_next(iter);
	}
	return false;
}

void vbuffer_clearmodified(struct vbuffer *buf)
{
	list2_iter iter, end;

	assert(vbuffer_isvalid(buf));

	iter = list2_begin(&buf->chunks);
	end = list2_end(&buf->chunks);
	while (iter != end) {
		struct vbuffer_chunk *cur = list2_get(iter, struct vbuffer_chunk);
		cur->flags.modified = false;
		iter = list2_next(iter);
	}
}

bool vbuffer_append(struct vbuffer *buf, struct vbuffer *buffer)
{
	assert(vbuffer_isvalid(buf));

	if (!list2_empty(&buffer->chunks)) {
		list2_insert_list(list2_end(&buf->chunks), list2_begin(&buffer->chunks), list2_end(&buffer->chunks));
		assert(list2_empty(&buffer->chunks));
	}
	return true;
}

bool vbuffer_transfer(struct vbuffer *data, struct vbuffer *buffer)
{
	assert(vbuffer_isvalid(buffer));

	_vbuffer_init(data);
	list2_swap(&data->chunks, &buffer->chunks);
	vbuffer_clear(buffer);
	return true;
}


/*
 * Iterators
 */

static bool _vbuffer_iterator_check(const struct vbuffer_iterator *position)
{
	if (position->registered) {
		if (!position->chunk || !position->chunk->data) {
			error(L"invalid buffer iterator");
			return false;
		}
	}

	assert(position->chunk && position->chunk->data);
	return true;
}

static void vbuffer_iterator_init(struct vbuffer_iterator *position)
{
	position->chunk = NULL;
	position->offset = 0;
	position->registered = false;
}

bool vbuffer_iterator_valid(struct vbuffer_iterator *position)
{
	return position->chunk != NULL;
}

void vbuffer_iterator_copy(struct vbuffer_iterator *position, const struct vbuffer_iterator *source)
{
	vbuffer_iterator_init(position);

	if (!_vbuffer_iterator_check(source)) return;

	position->chunk = source->chunk;
	position->offset = source->offset;
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

static void _vbuffer_iterator_update(struct vbuffer_iterator *position, struct vbuffer_chunk *chunk, size_t offset)
{
	if (position->chunk != chunk) {
		if (position->registered) {
			vbuffer_chunk_release(position->chunk);
			vbuffer_chunk_addref(chunk);
		}

		position->chunk = chunk;
	}

	position->offset = offset;
}

static size_t _vbuffer_iterator_fix(struct vbuffer_iterator *position, list2_iter *riter)
{
	if (position->offset != (vbsize)-1) {
		if (position->offset > position->chunk->size) {
			list2_iter iter = &position->chunk->list;
			size_t offset = position->offset;

			while (!list2_atend(iter)) {
				struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);
				if (offset > chunk->size) {
					offset -= chunk->size;
				}
				else {
					_vbuffer_iterator_update(position, chunk, offset);
					break;
				}

				iter = list2_next(iter);
			}

			*riter = iter;
			return offset;
		}
		else {
			*riter = &position->chunk->list;
			return position->offset;
		}
	}
	else {
		*riter = &position->chunk->list;
		return position->chunk->size;
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
	list2_iter iter, enditer = NULL;
	size_t offset, endoffset;
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
	if (end) endoffset = _vbuffer_iterator_fix(end, &enditer);

	while (!list2_atend(iter)) {
		struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);

		if (enditer && enditer == iter) {
			if (offset > endoffset) {
				error(L"invalid buffer end");
				return false;
			}
			else {
				cursize += endoffset - offset;
				offset = 0;

				if (cursize >= minsize) {
					if (size) *size = minsize;
					return true;
				}
				else {
					break;
				}
			}
		}
		else {
			cursize += chunk->size - offset;
			offset = 0;

			if (cursize >= minsize) {
				if (size) *size = minsize;
				return true;
			}
		}

		iter = list2_next(iter);
	}

	if (size) *size = cursize;
	return false;
}

bool vbuffer_iterator_check_available(struct vbuffer_iterator *position, size_t minsize, size_t *size)
{
	return _vbuffer_iterator_check_available(position, NULL, minsize, size);
}

void vbuffer_iterator_register(struct vbuffer_iterator *position)
{
	assert(position);

	if (!position->registered) {
		position->registered = true;
		vbuffer_chunk_addref(position->chunk);

	}
}

void vbuffer_iterator_unregister(struct vbuffer_iterator *position)
{
	assert(position);

	if (position->registered) {
		vbuffer_chunk_release(position->chunk);
		position->registered = false;
	}
}

static list2_iter _vbuffer_iterator_split(struct vbuffer_iterator *position)
{
	list2_iter iter;
	size_t offset;
	struct vbuffer_chunk *chunk;

	assert(_vbuffer_iterator_check(position));

	offset = _vbuffer_iterator_fix(position, &iter);

	if (offset == 0 || list2_atend(iter)) {
		return iter;
	}

	chunk = list2_get(iter, struct vbuffer_chunk);

	if (offset == chunk->size) {
		return list2_next(iter);
	}
	else {
		struct vbuffer_chunk *newchunk;

		assert(!position->chunk->flags.ctl);

		newchunk = vbuffer_chunk_create(chunk->data,
				chunk->offset + offset,
				chunk->size - offset);

		chunk->size = offset;
		newchunk->flags = chunk->flags;
		list2_insert(list2_next(iter), &newchunk->list);

		return &newchunk->list;
	}
}

bool vbuffer_iterator_insert(struct vbuffer_iterator *position, struct vbuffer *buffer)
{
	if (!_vbuffer_iterator_check(position)) return false;

	assert(vbuffer_isvalid(buffer));

	if (!list2_empty(&buffer->chunks)) {
		list2_iter insert = _vbuffer_iterator_split(position);
		insert = list2_insert_list(insert, list2_begin(&buffer->chunks), list2_end(&buffer->chunks));

		if (list2_atend(insert)) _vbuffer_iterator_update(position, position->chunk, -1);
		else _vbuffer_iterator_update(position, list2_get(insert, struct vbuffer_chunk), 0);

		assert(list2_empty(&buffer->chunks));
	}

	return true;
}

size_t vbuffer_iterator_advance(struct vbuffer_iterator *position, size_t len)
{
	size_t clen = len;
	list2_iter iter;
	size_t offset, endoffset;
	struct vbuffer_chunk *chunk = NULL;

	if (!_vbuffer_iterator_check(position)) return (size_t)-1;

	offset = _vbuffer_iterator_fix(position, &iter);
	assert(iter);

	while (!list2_atend(iter)) {
		chunk = list2_get(iter, struct vbuffer_chunk);

		if (offset > chunk->size) {
			offset -= chunk->size;
		}
		else {
			const size_t buflen = chunk->size - offset;
			assert(chunk->size >= offset);

			if (buflen >= clen) {
				endoffset = offset + clen;
				clen = 0;
				break;
			}

			endoffset = offset + buflen;
			clen -= buflen;
			offset = 0;
		}

		iter = list2_next(iter);
	}

	assert(chunk);

	_vbuffer_iterator_update(position, chunk, endoffset);
	return (len - clen);
}

bool vbuffer_iterator_sub(struct vbuffer_iterator *position, size_t len, struct vbuffer_sub *sub, bool split)
{
	struct vbuffer_iterator begin;

	if (!_vbuffer_iterator_check(position)) return false;

	vbuffer_iterator_copy(&begin, position);
	len = vbuffer_iterator_advance(position, len);

	if (split) {
		const list2_iter iter = _vbuffer_iterator_split(position);
		_vbuffer_iterator_update(position, list2_get(iter, struct vbuffer_chunk), 0);
	}

	const bool ret = vbuffer_sub_create_from_position(sub, &begin, len);
	vbuffer_iterator_clear(&begin);

	return ret;
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

/*bool vbuffer_iterator_iseof(struct vbuffer_iterator *position)
{
}*/

uint8 *vbuffer_iterator_mmap(struct vbuffer_iterator *position, size_t maxsize, size_t *size, bool write)
{
	list2_iter iter;
	size_t offset;

	if (!_vbuffer_iterator_check(position)) return NULL;

	if (maxsize == 0) return NULL;

	offset = _vbuffer_iterator_fix(position, &iter);

	while (!list2_atend(iter)) {
		struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);
		size_t len = chunk->size;

		assert(offset <= len);
		len -= offset;

		if (!chunk->flags.ctl && len > 0) {
			if (maxsize != ALL && len > maxsize) {
				if (size) *size = maxsize;
				_vbuffer_iterator_update(position, chunk, offset + maxsize);
			}
			else {
				if (size) *size = len;
				_vbuffer_iterator_update(position, chunk, -1);
			}
			return vbuffer_chunk_get_data(chunk, write) + offset;
		}

		offset = 0;
		iter = list2_next(iter);
	}

	return NULL;
}

bool vbuffer_iterator_mark(struct vbuffer_iterator *position, bool readonly)
{
	struct vbuffer_data_ctl_mark *mark;
	struct vbuffer_chunk *chunk;
	list2_iter insert;

	if (!_vbuffer_iterator_check(position)) return false;

	mark = vbuffer_data_ctl_mark();
	if (!mark) {
		return false;
	}

	chunk = vbuffer_chunk_create_ctl(&mark->super.super);
	if (!chunk) {
		return false;
	}

	insert = _vbuffer_iterator_split(position);
	if (!insert) {
		vbuffer_chunk_release(chunk);
		return false;
	}

	list2_insert(insert, &chunk->list);
	_vbuffer_iterator_update(position, chunk, 0);
	return true;
}

bool vbuffer_iterator_unmark(struct vbuffer_iterator *position)
{
	struct vbuffer_data_ctl_mark *mark;
	list2_iter next;

	if (!_vbuffer_iterator_check(position)) return false;

	mark = vbuffer_data_cast(position->chunk->data, vbuffer_data_ctl_mark);
	if (!mark) {
		error(L"iterator is not a mark");
		return false;
	}

	next = list2_next(&position->chunk->list);
	vbuffer_chunk_clear(position->chunk);

	if (list2_atend(next)) {
		vbuffer_iterator_clear(position);
	}
	else {
		_vbuffer_iterator_update(position, list2_get(next, struct vbuffer_chunk), 0);
	}

	return true;
}


/*
 * Sub buffer
 */

static bool _vbuffer_sub_check(const struct vbuffer_sub *data)
{
	if (!_vbuffer_iterator_check(&data->begin)) return false;
	if (!data->use_size && !_vbuffer_iterator_check(&data->end)) return false;
	return true;
}

static void vbuffer_sub_init(struct vbuffer_sub *data)
{
	vbuffer_iterator_init(&data->begin);
	vbuffer_iterator_init(&data->end);
	data->use_size = false;
}

void vbuffer_sub_clear(struct vbuffer_sub *data)
{
	vbuffer_iterator_clear(&data->begin);
	if (!data->use_size) vbuffer_iterator_clear(&data->end);
}

bool vbuffer_sub_create(struct vbuffer_sub *data, struct vbuffer *buffer, size_t offset, size_t length)
{
	vbuffer_sub_init(data);

	if (list2_empty(&buffer->chunks)) return false;

	vbuffer_begin(buffer, &data->begin);
	vbuffer_iterator_advance(&data->begin, offset);

	data->use_size = false;
	if (length == ALL) {
		vbuffer_end(buffer, &data->end);
	}
	else {
		vbuffer_iterator_copy(&data->end, &data->begin);
		vbuffer_iterator_advance(&data->end, length);
	}

	return true;
}

bool vbuffer_sub_create_from_position(struct vbuffer_sub *data, struct vbuffer_iterator *position, size_t length)
{
	vbuffer_sub_init(data);

	vbuffer_iterator_copy(&data->begin, position);
	data->use_size = true;
	data->length = length;
	return true;
}

bool vbuffer_sub_create_between_position(struct vbuffer_sub *data, struct vbuffer_iterator *begin, struct vbuffer_iterator *end)
{
	vbuffer_sub_init(data);

	vbuffer_iterator_copy(&data->begin, begin);
	data->use_size = false;
	vbuffer_iterator_copy(&data->end, end);
	return true;
}

void vbuffer_sub_begin(struct vbuffer_sub *data, struct vbuffer_iterator *begin)
{
	vbuffer_iterator_copy(begin, &data->begin);
}

void vbuffer_sub_end(struct vbuffer_sub *data, struct vbuffer_iterator *end)
{
	vbuffer_iterator_copy(end, &data->end);
}

bool vbuffer_sub_position(struct vbuffer_sub *data, struct vbuffer_iterator *iter, size_t offset)
{
	vbuffer_iterator_copy(iter, &data->begin);
	vbuffer_iterator_advance(iter, offset);
	return true;
}

bool vbuffer_sub_sub(struct vbuffer_sub *data, size_t offset, size_t length, struct vbuffer_sub *buffer)
{
	vbuffer_sub_position(data, &buffer->begin, offset);
	buffer->use_size = true;
	buffer->length = length;
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
		if (data->length < minsize) minsize = data->length;
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

	while (size > 0 && (ptr = vbuffer_mmap(data, &len, true, &mmapiter))) {
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
		struct vbuffer_iterator begin;
		vbuffer_sub_begin(data, &begin);

		offset = _vbuffer_iterator_fix(&begin, &iter->data);
		if (data->use_size) iter->len = data->length;
		else iter->len = 0;

		vbuffer_iterator_clear(&begin);
	}

	if (list2_atend(iter->data) || (!data->use_size && iter->len == (vbsize)-1)) {
		return NULL;
	}

	chunk = list2_get(iter->data, struct vbuffer_chunk);
	iter->data = list2_next(iter->data);

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
		list2_iter end = NULL;
		endoffset = _vbuffer_iterator_fix(&data->end, &end);

		if (&chunk->list == end) {
			if (endoffset < offset) {
				error(L"invalid buffer end");
				return NULL;
			}
			else {
				*len = endoffset - offset;
				iter->len = -1;
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

static list2_iter _vbuffer_extract(struct vbuffer_sub *data, struct vbuffer *buffer,
		bool mark_modified, bool insert_ctl)
{
	list2_iter begin, end = NULL, iter;
	size_t size;

	assert(data);
	assert(buffer);

	if (!_vbuffer_sub_check(data)) return NULL;

	_vbuffer_init(buffer);

	if (!data->use_size) {
		/* Split end first to avoid invalidating the beginning iterator */
		struct vbuffer_iterator iter;
		vbuffer_sub_end(data, &iter);
		end = _vbuffer_iterator_split(&iter);
		vbuffer_iterator_clear(&iter);
	}
	else {
		size = data->length;
	}

	{
		struct vbuffer_iterator iter;
		vbuffer_sub_begin(data, &iter);
		begin = _vbuffer_iterator_split(&iter);
		vbuffer_iterator_clear(&iter);
	}

	if (data->use_size || !insert_ctl) {
		list2_iter ctl_insert_iter = list2_prev(begin);
		iter = begin;
		while (!list2_atend(iter)) {
			struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);

			if (data->use_size) {
				if (size > chunk->size) {
					size -= chunk->size;
				}
				else {
					struct vbuffer_iterator iter;
					vbuffer_iterator_init(&iter);
					iter.chunk = chunk;
					iter.offset = size;
					end = _vbuffer_iterator_split(&iter);
					vbuffer_iterator_clear(&iter);
					break;
				}
			}
			else {
				if (iter == end) {
					break;
				}
			}

			if (chunk->flags.ctl && !insert_ctl) {
				iter = list2_erase(&chunk->list);
				ctl_insert_iter = list2_insert(list2_next(ctl_insert_iter), &chunk->list);
			}
			else {
				iter = list2_next(iter);
			}
		}

		if (!end) end = iter;
	}

	if (begin != end) {
		list2_insert_list(list2_end(&buffer->chunks), begin, end);
	}

	if (insert_ctl) {
		struct vbuffer_chunk *mark;
		struct vbuffer_data_ctl_select *ctl = vbuffer_data_ctl_select();
		if (!ctl) {
			vbuffer_clear(buffer);
			return NULL;
		}

		mark = vbuffer_chunk_create_ctl(&ctl->super.super);
		if (!mark) {
			vbuffer_clear(buffer);
			return NULL;
		}

		list2_insert(end, &mark->list);
		return &mark->list;
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
	list2_iter iter = _vbuffer_extract(data, buffer, false, true);
	if (!iter) {
		return false;
	}

	if (ref) {
		vbuffer_iterator_init(ref);
		ref->chunk = list2_get(iter, struct vbuffer_chunk);
		ref->offset = 0;
	}

	vbuffer_sub_clear(data);
	return true;
}

bool vbuffer_restore(struct vbuffer_iterator *position, struct vbuffer *data)
{
	struct vbuffer_data_ctl_select *ctl;

	assert(data);
	if (!_vbuffer_iterator_check(position)) return false;

	ctl = vbuffer_data_cast(position->chunk->data, vbuffer_data_ctl_select);
	if (!ctl) {
		error(L"invalid restore iterator");
		return false;
	}

	if (!list2_empty(&data->chunks)) {
		list2_insert_list(&position->chunk->list, list2_begin(&data->chunks), list2_end(&data->chunks));
	}

	vbuffer_chunk_clear(position->chunk);

	vbuffer_iterator_clear(position);
	vbuffer_clear(data);
	return true;
}

bool vbuffer_erase(struct vbuffer_sub *data)
{
	struct vbuffer buffer = vbuffer_init;
	list2_iter iter = _vbuffer_extract(data, &buffer, true, false);
	if (!iter) {
		return false;
	}

	vbuffer_release(&buffer);
	return true;
}

bool vbuffer_replace(struct vbuffer_sub *data, struct vbuffer *buffer)
{
	struct vbuffer_sub new;
	struct vbuffer erase = vbuffer_init;
	list2_iter iter = _vbuffer_extract(data, &erase, true, false);
	if (!iter) {
		return false;
	}
	vbuffer_release(&erase);

	/* Update buffer range */
	if (!vbuffer_sub_create(&new, buffer, 0, ALL)) {
		return false;
	}

	list2_insert_list(iter, list2_begin(&buffer->chunks), list2_end(&buffer->chunks));
	vbuffer_clear(buffer);

	/* Update buffer range */
	_vbuffer_iterator_update(&data->begin, new.begin.chunk, new.begin.offset);
	if (data->use_size) {
		data->length = new.length;
	}
	else {
		_vbuffer_iterator_update(&data->end, new.end.chunk, new.end.offset);
	}

	return true;
}

bool vbuffer_sub_isflat(struct vbuffer_sub *data)
{
	size_t offset;
	list2_iter iter;

	if (!_vbuffer_sub_check(data)) return false;

	offset = _vbuffer_iterator_fix(&data->begin, &iter);

	if (data->use_size) {
		struct vbuffer_chunk *chunk = list2_get(iter, struct vbuffer_chunk);
		return chunk->size <= offset + data->length;
	}
	else {
		list2_iter enditer;
		_vbuffer_iterator_fix(&data->end, &enditer);
		return iter == enditer;
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
						_vbuffer_iterator_update(&data->end, prev, prev->size + data->end.offset);
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
		vbuffer_iterator_copy(&iter, &data->begin);
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
	list2_iter end;

	if (!_vbuffer_sub_check(data)) return false;

	_vbuffer_init(buffer);
	end = list2_end(&buffer->chunks);

	size = 0;

	while ((chunk = _vbuffer_sub_iterate(data, &offset, &len, &iter))) {
		if (!chunk->flags.ctl) {
			struct vbuffer_chunk *clone = vbuffer_chunk_clone(chunk, copy);
			list2_insert(end, &clone->list);

			size += clone->size;
		}
	}

	if (copy) {
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
		error(L"asnumber: unsupported size %llu", length);
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
		error(L"asnumber: unsupported size %llu", length);
		return 0;
	}
}

bool vbuffer_setnumber(struct vbuffer_sub *data, bool bigendian, int64 num)
{
	const size_t length = vbuffer_sub_size(data);

	if (length > 8) {
		error(L"setnumber: unsupported size %llu", length);
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
			error(L"setnumber: unsupported size %llu", length);
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
			error(L"setnumber: unsupported size %llu", length);
			return false;
		}

		vbuffer_sub_write(data, temp.data, length);
	}

	return true;
}

static uint8 getbits(uint8 x, int off, int size)
{
	return (x >> off) & ((1 << size)-1);
}

static uint8 setbits(uint8 x, int off, int size, uint8 v)
{
	return (x & ~(((1 << size)-1) << off)) | ((v & ((1 << size)-1)) << off);
}

int64 vbuffer_asbits(struct vbuffer_sub *data, size_t offset, size_t bits, bool bigendian)
{
	uint8 temp[8];
	int64 ret = 0;
	int i, off = offset, shiftbits = 0;
	const size_t begin = offset >> 3;
	const size_t end = (offset + bits + 7) >> 3;
	offset &= (1 << 3);
	assert(offset < 8);

	if (end > 8) {
		error(L"asbits: unsupported size");
		return -1;
	}

	vbuffer_sub_read(data, temp, end);

	for (i=begin; i<end; ++i) {
		const int size = bits > 8-off ? 8-off : bits;

		if (bigendian) {
			ret = (ret << size) | getbits(temp[i], off, size);
		}
		else {
			ret = ret | (getbits(temp[i], off, size) << shiftbits);
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
		error(L"setbit: unsupported size %llu", bits);
		return false;
	}

	if ((num < 0 && (-num & ((1ULL << bits)-1)) != -num) ||
	    (num >= 0 && (num & ((1ULL << bits)-1)) != num)) {
		error(L"setbit: invalid number, value does not fit in %d bits", bits);
		return false;
	}

	{
		uint8 temp[8];
		int i, off = offset;
		const size_t begin = offset >> 3;
		const size_t end = (offset + bits + 7) >> 3;
		offset &= (1 << 3);
		assert(offset < 8);

		if (end > 8) {
			error(L"setbit: unsupported size");
			return false;
		}

		vbuffer_sub_read(data, temp, end);

		for (i=begin; i<end; ++i) {
			const int size = bits > 8-off ? 8-off : bits;
			bits -= size;

			if (bigendian) {
				temp[i] = setbits(temp[i], off, size, num >> bits);
			}
			else {
				temp[i] = setbits(temp[i], off, size, num);
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
	if (size == len) --size;
	str[size] = 0;
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
	vbuffer_iterator_copy(&iter, &data->begin);
	vbuffer_iterator_advance(&iter, offset);
	return vbuffer_iterator_getbyte(&iter);
}

bool vbuffer_setbyte(struct vbuffer_sub *data, size_t offset, uint8 byte)
{
	struct vbuffer_iterator iter;
	vbuffer_iterator_copy(&iter, &data->begin);
	vbuffer_iterator_advance(&iter, offset);
	return vbuffer_iterator_setbyte(&iter, byte);
}
