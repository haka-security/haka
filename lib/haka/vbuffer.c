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

#define EXCHANGE(a, b)    { typeof(a) tmp = (a); (a) = (b); (b) = tmp; }

static void vbuffer_free_elem(struct vbuffer *buf);


/*
 * Buffer data
 */

struct vbuffer_data_basic {
	struct vbuffer_data  super;
	atomic_t             ref;
	size_t               size;
	uint8                buffer[0];
};

static struct vbuffer_data_ops vbuffer_data_basic_ops;

#define VBUFFER_DATA_BASIC  \
	struct vbuffer_data_basic *buf = (struct vbuffer_data_basic *)_buf; \
	assert(buf->super.ops == &vbuffer_data_basic_ops)

static void vbuffer_data_basic_free(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	free(buf);
}

static void vbuffer_data_basic_addref(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	atomic_inc(&buf->ref);
}

static bool vbuffer_data_basic_release(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;
	return atomic_dec(&buf->ref) == 0;
}

static uint8 *vbuffer_data_basic_get(struct vbuffer_data *_buf, bool write)
{
	VBUFFER_DATA_BASIC;
	return buf->buffer;
}

static struct vbuffer_data_ops vbuffer_data_basic_ops = {
	free:    vbuffer_data_basic_free,
	addref:  vbuffer_data_basic_addref,
	release: vbuffer_data_basic_release,
	get:     vbuffer_data_basic_get
};

static struct vbuffer_data_basic *vbuffer_data_basic(size_t size)
{
	struct vbuffer_data_basic *buf = malloc(sizeof(struct vbuffer_data_basic) + size);
	if (!buf) {
		error(L"memory error");
		return NULL;
	}

	buf->super.ops = &vbuffer_data_basic_ops;
	buf->size = size;
	atomic_set(&buf->ref, 0);
	return buf;
}

static void vbuffer_data_release(struct vbuffer_data *data)
{
	if (data->ops->release(data)) {
		data->ops->free(data);
	}
}


/*
 *  Buffer ctl data
 */

struct vbuffer_data_ctl {
	struct vbuffer_data  super;
	atomic_t             ref;
};

struct vbuffer_data_ctl_select {
	struct vbuffer_data_ctl  super;
	struct vbuffer          *ctl;
};

struct vbuffer_data_ctl_push {
	struct vbuffer_data_ctl      super;
	struct vbuffer_stream       *stream;
	struct vbuffer_stream_chunk *chunk;
};

#define VBUFFER_DATA_CTL  \
	UNUSED struct vbuffer_data_ctl *buf = (struct vbuffer_data_ctl *)_buf;

static void vbuffer_data_ctl_free(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	free(buf);
}

static void vbuffer_data_ctl_addref(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	atomic_inc(&buf->ref);
}

static bool vbuffer_data_ctl_release(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_CTL;
	return atomic_dec(&buf->ref) == 0;
}

static uint8 *vbuffer_data_ctl_get(struct vbuffer_data *_buf, bool write)
{
	VBUFFER_DATA_CTL;
	return NULL;
}

static void vbuffer_data_ctl_select_free(struct vbuffer_data *_buf)
{
	struct vbuffer_data_ctl_select *select = (struct vbuffer_data_ctl_select *)_buf;

	struct vbuffer *iter = select->ctl;
	while (iter && iter->data != _buf) {
		struct vbuffer *cur = iter;
		iter = iter->next;
		vbuffer_free_elem(cur);
	}

	free(select);
}

static struct vbuffer_data_ops vbuffer_data_ctl_select_ops = {
	free:    vbuffer_data_ctl_select_free,
	addref:  vbuffer_data_ctl_addref,
	release: vbuffer_data_ctl_release,
	get:     vbuffer_data_ctl_get
};

static struct vbuffer_data_ctl_select *vbuffer_data_ctl_select()
{
	struct vbuffer_data_ctl_select *buf = malloc(sizeof(struct vbuffer_data_ctl_select));
	if (!buf) {
		error(L"memory error");
		return NULL;
	}

	buf->super.super.ops = &vbuffer_data_ctl_select_ops;
	atomic_set(&buf->super.ref, 0);
	buf->ctl = NULL;
	return buf;
}

static struct vbuffer_data_ops vbuffer_data_ctl_push_ops = {
	free:    vbuffer_data_ctl_free,
	addref:  vbuffer_data_ctl_addref,
	release: vbuffer_data_ctl_release,
	get:     vbuffer_data_ctl_get
};

static struct vbuffer_data_ctl_push *vbuffer_data_ctl_push(struct vbuffer_stream *stream,
	struct vbuffer_stream_chunk *chunk)
{
	struct vbuffer_data_ctl_push *buf = malloc(sizeof(struct vbuffer_data_ctl_push));
	if (!buf) {
		error(L"memory error");
		return NULL;
	}

	buf->super.super.ops = &vbuffer_data_ctl_push_ops;
	atomic_set(&buf->super.ref, 0);
	buf->stream = stream;
	buf->chunk = chunk;
	return buf;
}


/*
 * Buffer
 */

INLINE bool vbuffer_check_writeable(struct vbuffer *buf)
{
	if (!buf->flags.writable) {
		error(L"read only buffer");
		return false;
	}
	return true;
}

INLINE void vbuffer_mark_modified(struct vbuffer *buf)
{
	buf->flags.modified = true;
}

struct vbuffer *vbuffer_create_new(size_t size)
{
	struct vbuffer *ret;
	struct vbuffer_data_basic *data = vbuffer_data_basic(size);
	if (!data) {
		return NULL;
	}

	ret = vbuffer_create_from_data(&data->super, 0, size);
	if (!ret) {
		free(data);
		return NULL;
	}

	return ret;
}

struct vbuffer *vbuffer_create_from_string(const char *str)
{
	struct vbuffer *ret;
	const size_t len = strlen(str);
	struct vbuffer_data_basic *data = vbuffer_data_basic(len);
	if (!data) {
		return NULL;
	}

	memcpy(data->buffer, str, len);

	ret = vbuffer_create_from_data(&data->super, 0, len);
	if (!ret) {
		free(data);
		return NULL;
	}

	return ret;
}

struct vbuffer *vbuffer_create_from_data(struct vbuffer_data *data, size_t offset, size_t length)
{
	struct vbuffer *ret = malloc(sizeof(struct vbuffer));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&ret->lua_object);
	ret->next = NULL;
	ret->length = length;
	ret->offset = offset;
	ret->data = data;
	ret->flags.modified = false;
	ret->flags.writable = true;
	ret->flags.ctl = false;
	if (data) data->ops->addref(data);
	ret->iterators = NULL;

	return ret;
}

struct vbuffer *vbuffer_create_ctl(struct vbuffer_data *data)
{
	struct vbuffer *ret = malloc(sizeof(struct vbuffer));
	if (!ret) {
		data->ops->free(data);
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&ret->lua_object);
	ret->next = NULL;
	ret->length = 0;
	ret->offset = 0;
	ret->data = data;
	ret->flags.modified = false;
	ret->flags.writable = true;
	ret->flags.ctl = true;
	if (data) data->ops->addref(data);
	ret->iterators = NULL;

	return ret;
}

struct vbuffer *vbuffer_duplicate(struct vbuffer *data)
{
	struct vbuffer *ret = malloc(sizeof(struct vbuffer));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&ret->lua_object);
	ret->next = NULL;
	ret->length = data->length;
	ret->offset = data->offset;
	ret->data = data->data;
	ret->flags = data->flags;
	if (data->data) data->data->ops->addref(data->data);
	ret->iterators = NULL;

	return ret;
}

bool vbuffer_zero(struct vbuffer *buf, bool mark_modified)
{
	void *iter = NULL;
	uint8 *data;
	size_t len;

	if (mark_modified) {
		if (!vbuffer_check_writeable(buf)) {
			return false;
		}

		vbuffer_mark_modified(buf);
	}

	while ((data = vbuffer_mmap(buf, &iter, &len, false))) {
		memset(data, 0, len);
	}

	return true;
}

void vbuffer_setmode(struct vbuffer *buf, bool readonly)
{
	buf->flags.writable = !readonly;
}

static struct vbuffer *vbuffer_get(struct vbuffer *buf, size_t *off, bool keeplast)
{
	struct vbuffer *iter = buf, *last = NULL;
	while (iter) {
		if (!iter->flags.ctl) {
			last = iter;
			if (iter->length + keeplast > *off) {
				return iter;
			}

			*off -= iter->length;
		}

		iter = iter->next;
	}

	if (last) {
		*off = last->length;
		return last;
	}
	else {
		return NULL;
	}
}

static struct vbuffer *vbuffer_getlast(struct vbuffer *buf)
{
	struct vbuffer *iter = buf, *last = NULL;
	while (iter) {
		last = iter;
		iter = iter->next;
	}
	return last;
}

static struct vbuffer *vbuffer_split_force(struct vbuffer *buf, size_t off)
{
	struct vbuffer *newbuf;

	assert(off <= buf->length);
	assert(!buf->flags.ctl);

	newbuf = malloc(sizeof(struct vbuffer));
	if (!newbuf) {
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&newbuf->lua_object);

	newbuf->data = buf->data;
	newbuf->data->ops->addref(newbuf->data);

	newbuf->length = buf->length - off;
	newbuf->offset = buf->offset + off;
	buf->length = off;

	newbuf->next = buf->next;
	buf->next = newbuf;

	newbuf->flags = buf->flags;

	{
		struct vbuffer_iterator *iter = buf->iterators;
		while (iter) {
			if (iter->offset > off ||
			    (iter->offset == off && iter->post)) {
				break;
			}
			iter = list_next(iter);
		}

		newbuf->iterators = iter;
		if (iter) {
			if (iter->list.prev) {
				iter->list.prev->next = NULL;
				iter->list.prev = NULL;
			}
			else {
				assert(buf->iterators == iter);
				buf->iterators = NULL;
			}
		}

		while (iter) {
			iter->buffer = newbuf;
			assert(iter->offset >= off);
			iter->offset -= off;
			iter = list_next(iter);
		}
	}

	return newbuf;
}

static void _vbuffer_update_iterators(struct vbuffer *buf)
{
	struct vbuffer_iterator *iter = buf->iterators;
	while (iter) {
		iter->buffer = buf;
		iter = list_next(iter);
	}
}

static void _vbuffer_exchange(struct vbuffer *a, struct vbuffer *b)
{
	EXCHANGE(a->length, b->length);
	EXCHANGE(a->offset, b->offset);
	EXCHANGE(a->data, b->data);
	EXCHANGE(a->next, b->next);
	EXCHANGE(a->flags, b->flags);
	EXCHANGE(a->iterators, b->iterators);
	_vbuffer_update_iterators(a);
	_vbuffer_update_iterators(b);
}

static struct vbuffer *vbuffer_split(struct vbuffer *buf, size_t off)
{
	assert(off <= buf->length);

	if (off == 0) {
		return buf;
	}
	else if (off < buf->length) {
		return vbuffer_split_force(buf, off);
	}
	else {
		return buf->next;
	}
}

static struct vbuffer *_vbuffer_extract_ctl(struct vbuffer *start, struct vbuffer *insert,
		bool replace, struct vbuffer **after)
{
	struct vbuffer *iter = start, *prev = NULL, *next;
	struct vbuffer *ctl_start = NULL, *ctl_iter = NULL;

	start = NULL;
	while (iter) {
		next = iter->next;

		if (iter->flags.ctl) {
			if (prev) {
				prev->next = iter->next;
			}

			if (!ctl_iter) {
				ctl_start = iter;
			}
			else {
				ctl_iter->next = iter;
			}

			iter->next = NULL;
			ctl_iter = iter;
		}
		else {
			prev = iter;
			if (!start) start = iter;
		}

		iter = next;
	}

	if (ctl_start) {
		if (replace) {
			if (after && *after == insert) {
				*after = ctl_start;
			}

			_vbuffer_exchange(insert, ctl_start);
			if (ctl_iter == ctl_start) insert->next = ctl_start;
			else ctl_iter->next = ctl_start;
		}
		else {
			ctl_iter->next = insert->next;
			insert->next = ctl_start;
		}
	}

	return start;
}

static struct vbuffer *_vbuffer_create_select_ctl(struct vbuffer *start, struct vbuffer *begin,
		struct vbuffer *insert)
{
	struct vbuffer *iter = start, *ctl_iter = NULL, *ctl_node = NULL;
	struct vbuffer_data_ctl_select *ctl_select = NULL;

	ctl_select = vbuffer_data_ctl_select();
	if (!ctl_select) return NULL;

	ctl_node = vbuffer_create_ctl(&ctl_select->super.super);
	if (!ctl_node) {
		return NULL;
	}

	while (iter) {
		if (iter->flags.ctl) {
			if (!ctl_select->ctl) {
				ctl_select->ctl = vbuffer_duplicate(iter);
				if (!ctl_select->ctl) {
					vbuffer_free(ctl_node);
					return NULL;
				}
				ctl_iter = ctl_select->ctl;
			}
			else {
				ctl_iter->next = vbuffer_duplicate(iter);
				if (!ctl_iter->next) {
					vbuffer_free(ctl_node);
					return NULL;
				}
				ctl_iter = ctl_iter->next;
			}
		}

		iter = iter->next;
	}

	if (begin == insert) {
		_vbuffer_exchange(begin, ctl_node);
		begin->next = ctl_node;
		ctl_node = begin;
	}
	else {
		ctl_node->next = begin->next;
		begin->next = ctl_node;
	}

	if (ctl_iter) ctl_iter->next = ctl_node;

	return ctl_node;
}

static struct vbuffer *_vbuffer_extract(struct vbuffer *buf, size_t off, size_t len,
		bool mark_modified, struct vbuffer **after, bool create_ctl, struct vbuffer **ctlnode)
{
	struct vbuffer *begin, *iter, *end;

	if (mark_modified && !vbuffer_check_writeable(buf)) {
		return NULL;
	}

	begin = vbuffer_get(buf, &off, true);
	if (!begin) {
		error(L"invalid offset");
		return NULL;
	}

	assert(!begin->flags.ctl);

	if (len == 0) {
		struct vbuffer_data *data = begin->data;
		iter = vbuffer_split(begin, off);

		if (create_ctl) *ctlnode = _vbuffer_create_select_ctl(NULL, begin, iter);
		if (after) *after = iter;

		/* return a new empty vbuffer */
		return vbuffer_create_from_data(data, off, 0);
	}
	else {
		iter = vbuffer_split(begin, off);
	}

	if (!iter && check_error()) {
		return NULL;
	}

	if (mark_modified) vbuffer_mark_modified(begin);

	end = NULL;

	if (len != ALL) {
		struct vbuffer *start = iter;
		const bool replace = (start == begin);

		while (iter) {
			if (len <= iter->length) {
				end = vbuffer_split(iter, len);
				if (!iter && check_error()) return NULL;
				break;
			}

			len -= iter->length;
			iter = iter->next;
		}

		if (replace) {
			if (!end) {
				end = vbuffer_create_from_data(begin->data, off, 0);
				if (!end) return NULL;
				end->flags = begin->flags;
			}

			if (iter) iter->next = NULL;

			_vbuffer_exchange(begin, end);
			start = end;
			if (after) *after = begin;
		}
		else {
			if (iter) iter->next = NULL;
			assert(begin->next == start);
			begin->next = end;
			if (after) *after = end;
		}

		assert(start != buf);
		if (mark_modified) vbuffer_mark_modified(start);
		if (create_ctl) {
			*ctlnode = _vbuffer_create_select_ctl(start, begin, iter);
		}
		else {
			start = _vbuffer_extract_ctl(start, begin, replace, after);
		}
		return start;
	}
	else {
		if (iter == begin) {
			iter = vbuffer_create_from_data(begin->data, off, 0);
			if (!iter) return NULL;
			iter->flags = begin->flags;

			_vbuffer_exchange(begin, iter);
			if (mark_modified) vbuffer_mark_modified(iter);

			if (create_ctl) {
				*ctlnode = _vbuffer_create_select_ctl(iter, begin, begin);
			}
			else {
				iter = _vbuffer_extract_ctl(iter, begin, true, after);
			}

			if (after) *after = begin;

			return iter;
		}
		else {
			if (after) *after = NULL;

			if (!iter) {
				if (create_ctl) *ctlnode = _vbuffer_create_select_ctl(NULL, begin, iter);
				iter = vbuffer_create_from_data(begin->data, off, 0);
				if (!iter) return NULL;
				iter->flags = begin->flags;
				return iter;
			}
			else {
				begin->next = NULL;
				assert(iter != buf);
				if (mark_modified) vbuffer_mark_modified(iter);

				if (create_ctl) {
					*ctlnode = _vbuffer_create_select_ctl(iter, begin, iter);
				}
				else {
					iter = _vbuffer_extract_ctl(iter, begin, begin == iter, after);
				}

				return iter;
			}
		}
	}
}

struct vbuffer *vbuffer_extract(struct vbuffer *buf, size_t off, size_t len)
{
	return _vbuffer_extract(buf, off, len, true, NULL, false, NULL);
}

struct vbuffer *vbuffer_select(struct vbuffer *buf, size_t offset, size_t length, struct vbuffer **ref)
{
	return _vbuffer_extract(buf, offset, length, false, NULL, true, ref);
}

bool vbuffer_restore(struct vbuffer *node, struct vbuffer *data)
{
	struct vbuffer *last = NULL;

	if (node->data->ops != &vbuffer_data_ctl_select_ops) {
		error(L"invalid insert node");
		return false;
	}

	/* TODO: We should verify that the ctl nodes registered
	 * in the select ctl node can be found on the data */

	if (!data) {
		/* TODO: This case should be handled differently */
		data = vbuffer_create_new(0);
	}

	last = vbuffer_getlast(data);
	last->next = node->next;
	node->next = NULL;
	_vbuffer_exchange(node, data);
	vbuffer_free(data);

	return true;
}

static void vbuffer_free_elem(struct vbuffer *buf)
{
	struct vbuffer_iterator *iter = buf->iterators;
	while (iter) {
		struct vbuffer_iterator *cur = iter;
		iter = list_next(iter);

		assert(cur->registered);
		cur->registered = false;
		vbuffer_iterator_clear(cur);
	}

	vbuffer_data_release(buf->data);
	buf->next = NULL;
	free(buf);
}

void vbuffer_free(struct vbuffer *buf)
{
	lua_object_release(buf, &buf->lua_object);

	struct vbuffer *iter = buf;
	while (iter) {
		struct vbuffer *cur = iter;
		iter = iter->next;
		vbuffer_free_elem(cur);
	}
}

static uint8 *vbuffer_get_data(struct vbuffer *buf, bool write)
{
	uint8 *ptr;

	if (write && !vbuffer_check_writeable(buf)) {
		return NULL;
	}

	assert(!buf->flags.ctl);

	ptr = buf->data->ops->get(buf->data, write);
	if (!ptr) {
		assert(write); /* read should always be successful */
		assert(check_error());
		return NULL;
	}

	if (write) vbuffer_mark_modified(buf);
	return ptr + buf->offset;
}

uint8 vbuffer_getbyte(struct vbuffer *buf, size_t offset)
{
	struct vbuffer *iter = vbuffer_get(buf, &offset, false);
	if (!iter) {
		error(L"invalid offset");
		return 0;
	}

	return *(vbuffer_get_data(iter, false) + offset);
}

void vbuffer_setbyte(struct vbuffer *buf, size_t offset, uint8 byte)
{
	uint8 *ptr;
	struct vbuffer *iter = vbuffer_get(buf, &offset, false);
	if (!iter) {
		error(L"invalid offset");
		return;
	}

	ptr = vbuffer_get_data(iter, true);
	if (!ptr) {
		return;
	}

	*(ptr + offset) = byte;
}

static struct vbuffer *_vbuffer_insert(struct vbuffer *buf, size_t off, struct vbuffer *data,
		bool mark_modified, bool prefer_post)
{
	struct vbuffer *iter, *end;

	assert(data);
	assert(buf != data);
	assert(!lua_object_ownedbylua(&data->lua_object));

	if (mark_modified && !vbuffer_check_writeable(buf)) {
		return NULL;
	}

	if (off == 0) {
		iter = buf;
	}
	else {
		iter = vbuffer_get(buf, &off, true);
		if (!iter) {
			error(L"invalid offset");
			return NULL;
		}
	}

	lua_object_release(data, &data->lua_object);

	end = data;
	while (end->next) {
		end = end->next;
	}

	if (off == 0) {
		if (iter->length == 0 && prefer_post) {
			end->next = iter->next;
			iter->next = data;
		}
		else {
			_vbuffer_exchange(data, iter);
			if (end == data) end = iter;

			end->next = data;
			assert(end->next != end);
		}

		if (mark_modified) vbuffer_mark_modified(iter);
	}
	else {
		vbuffer_split(iter, off);
		end->next = iter->next;
		assert(end->next != end);
		iter->next = data;
		assert(iter->next != iter);

		if (mark_modified) vbuffer_mark_modified(iter);
	}

	return end;
}

bool vbuffer_insert(struct vbuffer *buf, size_t off, struct vbuffer *data)
{
	if (off == ALL) {
		struct vbuffer *last = vbuffer_getlast(buf);
		assert(last);
		return _vbuffer_insert(last, last->length, data, true, true) != NULL;
	}
	else {
		return _vbuffer_insert(buf, off, data, true, false) != NULL;
	}
}

static bool _vbuffer_erase(struct vbuffer *buf, size_t off, size_t len, struct vbuffer **after)
{
	if (len == 0) {
		return true;
	}

	struct vbuffer *erased = _vbuffer_extract(buf, off, len, true, after, false, NULL);
	if (!erased && check_error()) {
		return false;
	}

	vbuffer_free(erased);
	return true;
}

bool vbuffer_erase(struct vbuffer *buf, size_t off, size_t len)
{
	return _vbuffer_erase(buf, off, len, NULL);
}

size_t vbuffer_size(struct vbuffer *buf)
{
	size_t size = 0;
	struct vbuffer *iter = buf;

	while (iter) {
		size += iter->length;
		iter = iter->next;
	}

	return size;
}

bool vbuffer_checksize(struct vbuffer *buf, size_t minsize, size_t *rsize)
{
	size_t size = 0;
	struct vbuffer *iter = buf;

	while (iter) {
		size += iter->length;
		if (size >= minsize) {
			return true;
		}

		iter = iter->next;
	}

	if (rsize) *rsize = size;
	return false;
}

bool vbuffer_compact(struct vbuffer *buf)
{
	struct vbuffer *iter = buf, *next;
	while (iter) {
		next = iter->next;
		assert(next != iter);
		if (next) {
			if (next->data == iter->data) {
				if (iter->offset + iter->length == next->offset) {
					iter->length += next->length;
					iter->next = next->next;
					iter->flags.modified |= next->flags.modified;
					iter->flags.writable &= next->flags.writable;
					next->next = NULL;
					vbuffer_free(next);
					next = iter;
				}
			}
		}

		iter = next;
	}
	return true;
}

bool vbuffer_flatten(struct vbuffer *buf)
{
	if (!vbuffer_isflat(buf)) {
		vbuffer_compact(buf);
		if (!vbuffer_isflat(buf)) {
			struct vbuffer *iter = buf;
			const size_t size = vbuffer_size(buf);
			struct vbuffer_data_basic *flat;
			uint8 *ptr;

			flat = vbuffer_data_basic(size);
			if (!flat) {
				return false;
			}

			ptr = flat->buffer;

			while (iter) {
				memcpy(ptr, vbuffer_get_data(iter, false), iter->length);
				ptr += iter->length;
				iter = iter->next;
			}

			if (buf->next) {
				vbuffer_free(buf->next);
				buf->next = NULL;
			}

			vbuffer_data_release(buf->data);
			buf->data = &flat->super;
			flat->super.ops->addref(&flat->super);

			buf->offset = 0;
			buf->length = size;
		}
	}
	return true;
}

bool vbuffer_isflat(struct vbuffer *buf)
{
	return !buf->next;
}

uint8 *vbuffer_mmap(struct vbuffer *buf, void **_iter, size_t *len, bool write)
{
	struct vbuffer **iter = (struct vbuffer **)_iter;
	assert(buf);

	if (write && !vbuffer_check_writeable(buf)) {
		return NULL;
	}

	if (iter) {
		while (true) {
			if (!*iter) {
				*iter = buf;
			}
			else {
				*iter = (*iter)->next;
			}

			if (*iter) {
				if ((*iter)->flags.ctl) {
					continue;
				}

				if (len) *len = (*iter)->length;
				if (write) vbuffer_mark_modified(*iter);
				return vbuffer_get_data(*iter, write);
			}
			else {
				return NULL;
			}
		}
	}
	else {
		if (len) *len = buf->length;
		if (write) vbuffer_mark_modified(buf);
		if (buf->flags.ctl) {
			return NULL;
		}
		else {
			return vbuffer_get_data(buf, write);
		}
	}
}

bool vbuffer_ismodified(struct vbuffer *buf)
{
	struct vbuffer *iter = buf;
	while (iter) {
		if (iter->flags.modified) {
			return true;
		}
		iter = iter->next;
	}
	return false;
}

void vbuffer_clearmodified(struct vbuffer *buf)
{
	struct vbuffer *iter = buf;
	while (iter) {
		iter->flags.modified = false;
		iter = iter->next;
	}
}

static void vbuffer_register_iterator(struct vbuffer *buf, struct vbuffer_iterator *iterator)
{
	struct vbuffer_iterator *last = NULL;
	struct vbuffer_iterator *iter = buf->iterators;

	assert(iterator->buffer == buf);

	while (iter) {
		assert(iter->buffer == buf);
		if (iter->offset > iterator->offset ||
		    (iter->offset == iterator->offset &&
		     !iterator->post && iter->post)) {
			break;
		}

		last = iter;
		iter = list_next(iter);
	}

	if (last) {
		list_insert_after(iterator, last, &buf->iterators, NULL);
	}
	else {
		list_insert_before(iterator, NULL, &buf->iterators, NULL);
	}
}

static void vbuffer_unregister_iterator(struct vbuffer *buf, struct vbuffer_iterator *iterator)
{
	assert(iterator->buffer == buf);

#ifdef HAKA_DEBUG
	{
		struct vbuffer_iterator *iter = buf->iterators;
		while (iter) {
			assert(iter->buffer == buf);
			if (iter == iterator) {
				break;
			}
			iter = list_next(iter);
		}
		assert(iter);
	}
#endif

	list_remove(iterator, &buf->iterators, NULL);
}


/*
 * Iterators
 */

bool vbuffer_iterator(struct vbuffer *buf, struct vbuffer_iterator *iter, size_t offset, bool post, bool readonly)
{
	list_init(iter);
	iter->buffer = buf;
	iter->offset = 0;
	iter->post = post;
	iter->readonly = readonly;
	iter->registered = false;
	return vbuffer_iterator_advance(iter, offset) == 0;
}

bool vbuffer_iterator_clear(struct vbuffer_iterator *iter)
{
	if (iter->registered) {
		vbuffer_unregister_iterator(iter->buffer, iter);
		iter->registered = false;
	}

	iter->buffer = NULL;
	iter->offset = 0;
	return true;
}

static bool vbuffer_iterator_check(struct vbuffer_iterator *iter, bool modify, bool move)
{
	if (!iter || iter->offset > iter->buffer->length) {
		error(L"invalid buffer iterator");
		return false;
	}

	if (move && iter->registered) {
		error(L"buffer iterator is not movable");
		return false;
	}

	if (modify && iter->readonly) {
		error(L"read only iterator");
		return false;
	}

	return true;
}

bool vbuffer_iterator_register(struct vbuffer_iterator *iter)
{
	if (!vbuffer_iterator_check(iter, false, false)) {
		return false;
	}

	if (!iter->registered) {
		vbuffer_register_iterator(iter->buffer, iter);
		iter->registered = true;
		return true;
	}
	else {
		return false;
	}
}

bool vbuffer_iterator_unregister(struct vbuffer_iterator *iter)
{
	if (iter->registered) {
		if (!vbuffer_iterator_check(iter, false, false)) {
			return false;
		}

		vbuffer_unregister_iterator(iter->buffer, iter);
		iter->registered = false;
		return true;
	}
	else {
		return false;
	}
}

bool vbuffer_iterator_copy(const struct vbuffer_iterator *src, struct vbuffer_iterator *dst)
{
	list_init(dst);

	dst->buffer = src->buffer;
	dst->offset = src->offset;
	dst->post = false;
	dst->readonly = src->readonly;
	dst->registered = false;
	return true;
}

bool vbuffer_iterator_sub(struct vbuffer_iterator *iter, struct vsubbuffer *buffer, size_t len, bool advance)
{
	if (!vbuffer_iterator_check(iter, false, advance)) {
		return false;
	}

	if (len != ALL) {
		size_t size;
		if (!vbuffer_checksize(iter->buffer, iter->offset + len, &size)) {
			if (size < iter->offset) {
				error(L"invalid size");
				return false;
			}

			len = size - iter->offset;
		}
	}
	else {
		len = vbuffer_size(iter->buffer);
		if (len < iter->offset) {
			error(L"invalid size");
			return false;
		}

		len -= iter->offset;
	}

	vbuffer_iterator_copy(iter, &buffer->position);
	buffer->length = len;

	if (advance) {
		vbuffer_iterator_advance(iter, len);
	}

	return true;
}

size_t vbuffer_iterator_available(struct vbuffer_iterator *iterator)
{
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;
	size_t size = 0;

	if (!vbuffer_iterator_check(iterator, false, false)) {
		return 0;
	}

	while (iter) {
		const size_t buflen = iter->length - offset;

		size += buflen;
		offset = 0;
		iter = iter->next;
	}

	return size;
}

bool vbuffer_iterator_check_available(struct vbuffer_iterator *iterator, size_t size)
{
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;

	if (!vbuffer_iterator_check(iterator, false, false)) {
		return false;
	}

	while (iter) {
		const size_t buflen = iter->length - offset;
		if (buflen >= size) {
			return true;
		}

		size -= buflen;
		offset = 0;
		iter = iter->next;
	}

	return false;
}

size_t vbuffer_iterator_read(struct vbuffer_iterator *iterator, uint8 *buffer, size_t len, bool advance)
{
	size_t clen = len;
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;

	if (!vbuffer_iterator_check(iterator, false, advance)) {
		return (size_t)-1;
	}

	while (iter) {
		if (!iter->flags.ctl) {
			size_t buflen = iter->length - offset;
			if (buflen > clen) {
				buflen = clen;
			}

			memcpy(buffer, vbuffer_get_data(iter, false) + offset, buflen);
			buffer += buflen;
			clen -= buflen;

			if (clen == 0) {
				offset += buflen;
				break;
			}
			else {
				offset = 0;
			}
		}

		iter = iter->next;
	}

	if (advance) {
		assert(!iterator->registered);
		iterator->buffer = iter;
		iterator->offset = offset;
	}

	return (len - clen);
}

size_t vbuffer_iterator_write(struct vbuffer_iterator *iterator, uint8 *buffer, size_t len, bool advance)
{
	size_t clen = len;
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;

	if (!vbuffer_iterator_check(iterator, true, advance)) {
		return (size_t)-1;
	}

	while (iter) {
		if (!iter->flags.ctl) {
			uint8 *ptr = vbuffer_get_data(iter, true);
			if (!ptr) {
				break;
			}

			size_t buflen = iter->length - offset;
			if (buflen > clen) {
				buflen = clen;
			}

			memcpy(ptr + offset, buffer, buflen);
			buffer += buflen;
			clen -= buflen;

			if (clen == 0) {
				offset += buflen;
				break;
			}
			else {
				offset = 0;
			}
		}

		iter = iter->next;
	}

	if (advance) {
		assert(!iterator->registered);
		iterator->buffer = iter;
		iterator->offset = offset;
	}

	return (len - clen);
}

bool vbuffer_iterator_insert(struct vbuffer_iterator *iter, struct vbuffer *data)
{
	struct vbuffer *last;

	if (!vbuffer_iterator_check(iter, true, true)) {
		return false;
	}

	last = _vbuffer_insert(iter->buffer, iter->offset, data, true, iter->post);
	if (!last) {
		assert(check_error());
		return false;
	}

	assert(!iter->registered);

	iter->buffer = last;
	iter->offset = last->length;

	return true;
}

bool vbuffer_iterator_erase(struct vbuffer_iterator *iter, size_t len)
{
	bool ret;
	struct vbuffer *after = NULL;

	if (!vbuffer_iterator_check(iter, true, true)) {
		return false;
	}

	ret = _vbuffer_erase(iter->buffer, iter->offset, len, &after);
	assert(!iter->registered);

	if (after) {
		iter->buffer = after;
		iter->offset = 0;
	}

	return ret;
}

bool vbuffer_iterator_replace(struct vbuffer_iterator *iter, size_t len, struct vbuffer *data)
{
	if (!vbuffer_iterator_erase(iter, len)) {
		return false;
	}

	return vbuffer_iterator_insert(iter, data);
}

size_t vbuffer_iterator_advance(struct vbuffer_iterator *iterator, size_t len)
{
	size_t clen = len;
	struct vbuffer *iter = iterator->buffer, *last = NULL;
	size_t offset = iterator->offset;
	iterator->offset = 0;

	if (!vbuffer_iterator_check(iterator, false, true)) {
		return (size_t)-1;
	}

	assert(iter);
	assert(!iterator->registered);

	while (iter) {
		const size_t buflen = iter->length - offset;
		assert(iter->length >= offset);

		last = iter;

		if (buflen >= clen) {
			iterator->offset = offset + clen;
			iterator->buffer = iter;
			return 0;
		}

		clen -= buflen;
		offset = 0;
		iter = iter->next;
	}

	iterator->buffer = last;
	iterator->offset = last->length;

	return (len - clen);
}

static uint8 *vbuffer_iterator_get_data(struct vbuffer_iterator *iterator, bool write)
{
	uint8 *ptr;
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;

	if (!vbuffer_iterator_check(iterator, write, false)) {
		return NULL;
	}

	ptr = vbuffer_get_data(iter, write);
	if (!ptr) {
		return NULL;
	}

	return ptr + offset;
}


/*
 * Sub buffer
 */

bool vbuffer_sub(struct vbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub)
{
	vbuffer_iterator(buf, &sub->position, offset, false, false);

	if (length != ALL) {
		size_t size;
		if (!vbuffer_checksize(sub->position.buffer, sub->position.offset + length, &size)) {
			if (size < sub->position.offset) {
				error(L"invalid size");
				return false;
			}

			length = size - sub->position.offset;
		}
	}
	else {
		length = vbuffer_size(sub->position.buffer);
		if (length < sub->position.offset) {
			error(L"invalid size");
			return false;
		}

		length -= sub->position.offset;
	}

	sub->length = length;
	return true;
}

bool vsubbuffer_sub(struct vsubbuffer *buf, size_t offset, size_t length, struct vsubbuffer *sub)
{
	sub->position = buf->position;
	vbuffer_iterator_advance(&sub->position, offset);

	if (length != ALL) {
		if (length+offset > buf->length) {
			error(L"invalid size");
			return false;
		}
	}
	else {
		length = buf->length;
		if (length < offset) {
			error(L"invalid size");
			return false;
		}

		length -= offset;
	}

	sub->length = length;
	return true;
}

bool vsubbuffer_isflat(struct vsubbuffer *buf)
{
	return (buf->position.buffer->length >= buf->position.offset + buf->length);
}

bool vsubbuffer_flatten(struct vsubbuffer *buf)
{
	if (!vsubbuffer_isflat(buf)) {
		struct vbuffer_iterator iter;
		struct vbuffer *flat;
		uint8 *ptr;
		UNUSED size_t len;

		vbuffer_iterator_copy(&buf->position, &iter);

		flat = vbuffer_create_new(buf->length);
		if (!flat) {
			return false;
		}

		ptr = vbuffer_mmap(flat, NULL, NULL, true);
		len = vbuffer_iterator_read(&iter, ptr, buf->length, false);
		assert(len == buf->length);

		vbuffer_erase(iter.buffer, iter.offset, buf->length);
		vbuffer_insert(iter.buffer, iter.offset, flat);
	}
	return true;
}

size_t vsubbuffer_size(struct vsubbuffer *buf)
{
	return buf->length;
}

uint8 *vsubbuffer_mmap(struct vsubbuffer *buf, void **_iter, size_t *remlen, size_t *len, bool write)
{
	struct vbuffer **iter = (struct vbuffer **)_iter;
	size_t offset;
	assert(iter);
	assert(remlen);

	while (true) {
		if (!*iter) {
			*iter = buf->position.buffer;
			offset = buf->position.offset;
			*remlen = buf->length;
		}
		else {
			*iter = (*iter)->next;
			offset = 0;
		}

		if (*iter) {
			if ((*iter)->flags.ctl) {
				continue;
			}

			uint8 *ptr = vbuffer_get_data(*iter, write);
			if (!ptr) {
				return NULL;
			}

			*len = (*iter)->length - offset;
			if (*remlen >= *len) {
				*remlen -= *len;
				return ptr + offset;
			}
			else {
				*len = *remlen;
				*remlen = 0;
				return ptr + offset;
			}
		}
		else {
			return NULL;
		}
	}
}

int64 vsubbuffer_asnumber(struct vsubbuffer *buf, bool bigendian)
{
	uint8 temp[8];
	const uint8 *ptr;

	if (buf->length > 8) {
		error(L"unsupported size");
		return 0;
	}

	if (vsubbuffer_isflat(buf)) {
		ptr = vbuffer_iterator_get_data(&buf->position, false);
		if (!ptr) {
			return 0;
		}
	}
	else {
		vbuffer_iterator_read(&buf->position, temp, buf->length, false);
		ptr = temp;
	}

	switch (buf->length) {
	case 1: return *ptr;
	case 2: return bigendian ? SWAP_FROM_BE(int16, *(int16*)ptr) : SWAP_FROM_LE(int16, *(int16*)ptr);
	case 4: return bigendian ? SWAP_FROM_BE(int32, *(int32*)ptr) : SWAP_FROM_LE(int32, *(int32*)ptr);
	case 8: return bigendian ? SWAP_FROM_BE(int64, *(int64*)ptr) : SWAP_FROM_LE(int64, *(int64*)ptr);
	default:
		error(L"unsupported size");
		return 0;
	}
}

void vsubbuffer_setnumber(struct vsubbuffer *buf, bool bigendian, int64 num)
{
	if (buf->length > 8) {
		error(L"unsupported size");
		return;
	}

	if ((num < 0 && (-num & ((1ULL << buf->length*8)-1)) != -num) ||
	    (num >= 0 && (num & ((1ULL << buf->length*8)-1)) != num)) {
		error(L"invalid number, value does not fit in %d bytes", buf->length);
		return;
	}

	if (vsubbuffer_isflat(buf)) {
		uint8 *ptr = vbuffer_iterator_get_data(&buf->position, true);
		if (!ptr) {
			return;
		}

		switch (buf->length) {
		case 1: *ptr = num; break;
		case 2: *(int16*)ptr = bigendian ? SWAP_TO_BE(int16, (int16)num) : SWAP_TO_LE(int16, (int16)num); break;
		case 4: *(int32*)ptr = bigendian ? SWAP_TO_BE(int32, (int32)num) : SWAP_TO_LE(int32, (int32)num); break;
		case 8: *(int64*)ptr = bigendian ? SWAP_TO_BE(int64, num) : SWAP_TO_LE(int64, num); break;
		default:
			error(L"unsupported size");
			return;
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

		switch (buf->length) {
		case 1: temp.i8 = num; break;
		case 2: temp.i16 = bigendian ? SWAP_TO_BE(int16, (int16)num) : SWAP_TO_LE(int16, (int16)num); break;
		case 4: temp.i32 = bigendian ? SWAP_TO_BE(int32, (int32)num) : SWAP_TO_LE(int32, (int32)num); break;
		case 8: temp.i64 = bigendian ? SWAP_TO_BE(int64, num) : SWAP_TO_LE(int64, num); break;
		default:
			error(L"unsupported size");
			return;
		}

		vbuffer_iterator_write(&buf->position, temp.data, buf->length, false);
	}
}

static uint8 getbits(uint8 x, int off, int size)
{
#if HAKA_LITTLEENDIAN
	return (x >> (8-off-size)) & ((1 << size)-1);
#else
	return (x >> off) & ((1 << size)-1);
#endif
}

static uint8 setbits(uint8 x, int off, int size, uint8 v)
{
#if HAKA_LITTLEENDIAN
	return (x & ~(((1 << size)-1) << (8-off-size))) | ((v & ((1 << size)-1)) << (8-off-size));
#else
	return (x & ~(((1 << size)-1) << off)) | ((v & ((1 << size)-1)) << off));
#endif
}

int64 vsubbuffer_asbits(struct vsubbuffer *buf, size_t offset, size_t bits, bool bigendian)
{
#if HAKA_LITTLEENDIAN
	if (!bigendian) {
#else
	if (bigendian) {
#endif
		int64 n = vsubbuffer_asnumber(buf, bigendian);
		return (n >> offset) & ((1 << (bits))-1);
	}
	else {
		uint8 temp[8];
		int64 ret = 0;
		int i, off = offset, shiftbits = 0;
		const size_t end = (offset + bits + 7) >> 3;
		assert(offset < 8);

		if (end > 8) {
			error(L"unsupported size");
			return -1;
		}

		vbuffer_iterator_read(&buf->position, temp, end, false);

		for (i=0; i<end; ++i) {
			const int size = bits > 8-off ? 8-off : bits;

			if (bigendian) {
				ret = (ret << size) | getbits(temp[i], off, size);
			}
			else {
				ret = ret | (getbits(temp[i], off, size) >> shiftbits);
				shiftbits += size;
			}

			off = 0;
			bits -= size;
		}

		return ret;
	}
}

void vsubbuffer_setbits(struct vsubbuffer *buf, size_t offset, size_t bits, bool bigendian, int64 num)
{
	if (bits > 64) {
		error(L"unsupported size");
		return;
	}

	if ((num < 0 && (-num & ((1ULL << bits)-1)) != -num) ||
	    (num >= 0 && (num & ((1ULL << bits)-1)) != num)) {
		error(L"invalid number, value does not fit in %d bits", bits);
		return;
	}

#if HAKA_LITTLEENDIAN
	if (!bigendian) {
#else
	if (bigendian) {
#endif
		int64 n = vsubbuffer_asnumber(buf, bigendian);
		n &= ~(((1 << (bits))-1) << offset);
		n |= (num & ((1 << (bits))-1)) << offset;
		vsubbuffer_setnumber(buf, bigendian, n);
	}
	else {
		uint8 temp[8];
		int i, off = offset, shiftbits = 0;
		const size_t end = (offset + bits + 7) >> 3;
		assert(offset < 8);

		if (end > 8) {
			error(L"unsupported size");
			return;
		}

		vbuffer_iterator_read(&buf->position, temp, end, false);

		for (i=0; i<end; ++i) {
			const int size = bits > 8-off ? 8-off : bits;

			if (bigendian) {
				shiftbits += size;
				temp[i] = setbits(temp[i], off, size, num >> (bits-shiftbits));
			}
			else {
				temp[i] = setbits(temp[i], off, size, num);
				num >>= size;
			}

			off = 0;
		}

		vbuffer_iterator_write(&buf->position, temp, end, false);
	}
}

size_t vsubbuffer_asstring(struct vsubbuffer *buf, char *str, size_t len)
{
	struct vbuffer *buffer = buf->position.buffer;
	size_t offset = buf->position.offset;
	int i;

	if (len > buf->length) {
		len = buf->length;
	}

	if (len == 0) {
		return 0;
	}

	for (i=0; i<len; ++i, ++offset) {
		buffer = vbuffer_get(buffer, &offset, false);
		if (!buffer) {
			error(L"invalid sub buffer");
			return 0;
		}

		str[i] = *(vbuffer_get_data(buffer, false) + offset);
	}

	return len;
}

size_t vsubbuffer_setfixedstring(struct vsubbuffer *buf, const char *str, size_t len)
{
	struct vbuffer *buffer = buf->position.buffer;
	size_t offset = buf->position.offset;
	int i;

	if (len > buf->length) {
		len = buf->length;
	}

	if (len == 0) {
		return 0;
	}

	for (i=0; i<len; ++i, ++offset) {
		uint8 *ptr;
		buffer = vbuffer_get(buffer, &offset, false);
		if (!buffer) {
			error(L"invalid sub buffer");
			return 0;
		}

		ptr = vbuffer_get_data(buffer, true);
		if (!ptr) {
			return 0;
		}
		*(ptr + offset) = str[i];
	}

	return len;
}

void vsubbuffer_setstring(struct vsubbuffer *buf, const char *str, size_t len)
{
	struct vbuffer *modif;
	uint8 *ptr;

	vbuffer_erase(buf->position.buffer, buf->position.offset, buf->length);

	modif = vbuffer_create_new(len);
	if (!modif) {
		return;
	}

	ptr = vbuffer_mmap(modif, NULL, NULL, true);
	assert(ptr);
	memcpy(ptr, str, len);

	vbuffer_insert(buf->position.buffer, buf->position.offset, modif);
	buf->length = len;
}

uint8 vsubbuffer_getbyte(struct vsubbuffer *buf, size_t offset)
{
	if (offset >= buf->length) {
		error(L"invalid offset");
		return 0;
	}

	return vbuffer_getbyte(buf->position.buffer, buf->position.offset + offset);
}

void vsubbuffer_setbyte(struct vsubbuffer *buf, size_t offset, uint8 byte)
{
	if (offset >= buf->length) {
		error(L"invalid offset");
		return;
	}

	vbuffer_setbyte(buf->position.buffer, buf->position.offset + offset, byte);
}


/*
 * Buffer stream
 */

struct vbuffer_stream_chunk {
	struct list                    list;
	struct vbuffer_data_ctl_push  *ctl_data;
	struct vbuffer                *ctl;
};

struct vbuffer_stream *vbuffer_stream()
{
	struct vbuffer_stream *ret = malloc(sizeof(struct vbuffer_stream));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&ret->lua_object);
	ret->data = NULL;
	ret->first = NULL;
	ret->last = NULL;
	ret->read_first = NULL;
	ret->read_last = NULL;
	return ret;
}

static void _vbuffer_stream_free_chunks(struct vbuffer_stream_chunk *chunk)
{
	struct vbuffer_stream_chunk *iter, *next;

	iter = chunk;
	while (iter) {
		next = list_next(iter);
		free(iter);
		iter = next;
	}
}

void vbuffer_stream_free(struct vbuffer_stream *stream)
{
	_vbuffer_stream_free_chunks(stream->first);
	_vbuffer_stream_free_chunks(stream->read_first);

	lua_object_release(stream, &stream->lua_object);

	if (stream->data) vbuffer_free(stream->data);
	free(stream);
}

bool vbuffer_stream_push(struct vbuffer_stream *stream, struct vbuffer *data)
{
	struct vbuffer *last;
	struct vbuffer_stream_chunk *chunk = malloc(sizeof(struct vbuffer_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	list_init(chunk);

	chunk->ctl_data = vbuffer_data_ctl_push(stream, chunk);
	if (!chunk->ctl_data) {
		return false;
	}

	chunk->ctl = vbuffer_create_ctl(&chunk->ctl_data->super.super);
	if (!chunk->ctl) {
		error(L"memory error");
		free(chunk);
		return false;
	}

	last = vbuffer_getlast(data);
	assert(last);
	_vbuffer_insert(last, last->length, chunk->ctl, false, true);

	list_insert_after(chunk, stream->last, &stream->first, &stream->last);

	if (!stream->data) {
		stream->data = data;
	}
	else {
		struct vbuffer *last = vbuffer_getlast(stream->data);
		assert(last);
		_vbuffer_insert(last, last->length, data, false, true);
	}

	return true;
}

struct vbuffer *vbuffer_stream_pop(struct vbuffer_stream *stream)
{
	struct vbuffer *last, *iter, *prev;
	struct vbuffer_stream_chunk *current = stream->first;
	bool keep_for_read = false;

	if (!stream->first) {
		return NULL;
	}

	if (stream->read_last) {
		last = stream->read_last->ctl;
		prev = last;
		iter = last->next;
	}
	else {
		last = stream->data;
		prev = NULL;
		iter = last;
	}

	/* Check if the data can be pop */
	while (true) {
		assert(iter);

		if (iter->data == &current->ctl_data->super.super) {
			break;
		}

#ifdef HAKA_DEBUG
		if (iter->data->ops == &vbuffer_data_ctl_push_ops) {
			struct vbuffer_data_ctl_push *ctl_data = (struct vbuffer_data_ctl_push *)iter->data;
			assert(ctl_data->stream != stream);
		}
#endif

		if (iter->data->ops == &vbuffer_data_ctl_select_ops) {
			struct vbuffer_data_ctl_select *ctl_data = (struct vbuffer_data_ctl_select *)iter->data;
			struct vbuffer *ctl_iter = ctl_data->ctl;
			while (ctl_iter && ctl_iter != iter) {
				if (iter->data->ops == &vbuffer_data_ctl_push_ops) {
					struct vbuffer_data_ctl_push *ctl_data = (struct vbuffer_data_ctl_push *)iter->data;
					if (ctl_data->stream == stream) {
						return NULL;
					}
				}

				ctl_iter = ctl_iter->next;
			}
		}

		if (iter->iterators) {
			struct vbuffer_iterator *pos = iter->iterators;
			while (pos) {
				if (!pos->readonly) {
					return NULL;
				}

				keep_for_read = true;
				pos = list_next(pos);
			}
		}

		prev = iter;
		iter = iter->next;
	}

	if (keep_for_read) {
		//TODO
		return NULL;
	}

	list_remove(current, &stream->first, &stream->last);

	free(current);

	if (last == stream->data) {
		if (iter->next) {
			stream->data = iter->next;
			iter->next = NULL;
		}
		else {
			stream->data = NULL;
		}

		if (prev) {
			prev->next = NULL;
			vbuffer_free(iter);
		}
		else {
			assert(iter == last);
			vbuffer_free(iter);
			last = vbuffer_create_new(0);
		}
	}
	else {
		last->next = iter->next;
		prev->next = NULL;
		iter->next = NULL;
		vbuffer_free(iter);
	}

	return last;
}

struct vbuffer *vbuffer_stream_data(struct vbuffer_stream *stream)
{
	return stream->data;
}

bool vbuffer_stream_current(struct vbuffer_stream *stream, struct vbuffer_iterator *position)
{
	if (stream->last && list_prev(stream->last)) {
		return vbuffer_iterator(list_prev(stream->last)->ctl, position, 0, true, false);
	}
	else if (stream->data) {
		return vbuffer_iterator(stream->data, position, 0, false, false);
	}
	else {
		return false;
	}
}
