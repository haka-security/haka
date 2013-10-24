
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <haka/vbuffer.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/thread.h>

#define EXCHANGE(a, b)    { typeof(a) tmp = (a); (a) = (b); (b) = tmp; }


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

static void vbuffer_data_basic_addref(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;

	atomic_inc(&buf->ref);
}

static void vbuffer_data_basic_release(struct vbuffer_data *_buf)
{
	VBUFFER_DATA_BASIC;

	if (atomic_dec(&buf->ref) == 0) {
		free(buf);
	}
}

static uint8 *vbuffer_data_basic_get(struct vbuffer_data *_buf, bool write)
{
	VBUFFER_DATA_BASIC;
	return buf->buffer;
}

static struct vbuffer_data_ops vbuffer_data_basic_ops = {
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


/*
 * Buffer
 */

struct vbuffer {
	struct vbuffer         *next;
	size_t                  length;
	size_t                  offset;
	struct vbuffer_data    *data;
};

struct vbuffer *vbuffer_create_new(size_t size)
{
	struct vbuffer *ret;
	struct vbuffer_data_basic *data = vbuffer_data_basic(size);
	if (!data) {
		return NULL;
	}

	ret = vbuffer_create_from(&data->super, size);
	if (!ret) {
		free(data);
		return NULL;
	}

	return ret;
}

struct vbuffer *vbuffer_create_from(struct vbuffer_data *data, size_t length)
{
	struct vbuffer *ret = malloc(sizeof(struct vbuffer));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->next = NULL;
	ret->length = length;
	ret->offset = 0;
	ret->data = data;
	data->ops->addref(data);

	return ret;
}

bool vbuffer_recreate_from(struct vbuffer *buf, struct vbuffer_data *data, size_t length)
{
	if (buf->next) {
		vbuffer_free(buf->next);
		buf->next = NULL;
	}

	buf->data->ops->release(buf->data);
	buf->data = data;
	buf->data->ops->addref(buf->data);
	buf->offset = 0;
	buf->length = length;
	return true;
}

static struct vbuffer *vbuffer_get(struct vbuffer *buf, size_t *off, bool keeplast)
{
	struct vbuffer *iter = buf, *last = NULL;
	while (iter) {
		last = iter;
		if (iter->length + keeplast > *off) {
			return iter;
		}

		*off -= iter->length;
		iter = iter->next;
	}

	*off = last->length;
	return last;
}

static struct vbuffer *vbuffer_split_force(struct vbuffer *buf, size_t off)
{
	struct vbuffer *newbuf;

	assert(off <= buf->length);

	newbuf = malloc(sizeof(struct vbuffer));
	if (!newbuf) {
		error(L"memory error");
		return NULL;
	}

	newbuf->data = buf->data;
	newbuf->data->ops->addref(newbuf->data);

	newbuf->length = buf->length - off;
	newbuf->offset = buf->offset + off;
	buf->length = off;

	newbuf->next = buf->next;
	buf->next = newbuf;

	return newbuf;
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

struct vbuffer *vbuffer_extract(struct vbuffer *buf, size_t off, size_t len)
{
	struct vbuffer *begin, *iter, *end;

	begin = vbuffer_get(buf, &off, true);
	if (!begin) {
		error(L"invalid offset");
		return NULL;
	}

	iter = vbuffer_split(begin, off);
	if (!iter && check_error()) {
		return NULL;
	}

	if (len == 0) {
		if (iter) {
			iter = vbuffer_split_force(iter, 0);
		}
		else {
			iter = vbuffer_split_force(begin, begin->length);
		}

		if (!iter && check_error()) {
			return NULL;
		}

		assert(iter != buf);
		return iter;
	}
	else {
		end = NULL;

		if (len != (size_t)-1) {
			assert(iter);

			while (iter) {
				if (len < iter->length) {
					end = vbuffer_split(iter, len);
					if (!iter && check_error()) {
						return NULL;
					}

					iter->next = NULL;
					break;
				}

				len -= iter->length;
				iter = iter->next;
			}

			iter = begin->next;
			begin->next = end;
			assert(iter != buf);
			return iter;
		}
		else {
			if (!iter) {
				iter = vbuffer_split_force(begin, begin->length);
			}
			else if (off == 0) {
				iter = vbuffer_split_force(iter, 0);
			}

			begin->next = NULL;
			assert(iter != buf);
			return iter;
		}
	}
}

void vbuffer_free(struct vbuffer *buf)
{
	struct vbuffer *iter = buf;
	while (iter) {
		struct vbuffer *cur = iter;
		iter = iter->next;

		cur->data->ops->release(cur->data);
		cur->next = NULL;
		free(cur);
	}
}

static uint8 *vbuffer_get_data(struct vbuffer *buf, bool write)
{
	uint8 *ptr = buf->data->ops->get(buf->data, write);
	if (!ptr) {
		assert(write); /* read should always be successful */
		assert(check_error());
		return NULL;
	}
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

static struct vbuffer *_vbuffer_insert(struct vbuffer *buf, size_t off, struct vbuffer *data)
{
	struct vbuffer *iter, *end;

	assert(data);
	assert(buf != data);

	iter = vbuffer_get(buf, &off, true);
	if (!iter) {
		error(L"invalid offset");
		return NULL;
	}

	end = data;
	while (end->next) {
		end = end->next;
	}

	if (off == 0) {
		EXCHANGE(data->length, iter->length);
		EXCHANGE(data->offset, iter->offset);
		EXCHANGE(data->data, iter->data);
		EXCHANGE(data->next, iter->next);
		if (end == data) end = iter;

		end->next = data;
		assert(end->next != end);
	}
	else {
		vbuffer_split(iter, off);
		end->next = iter->next;
		assert(end->next != end);
		iter->next = data;
		assert(iter->next != iter);
	}

	return end;
}

bool vbuffer_insert(struct vbuffer *buf, size_t off, struct vbuffer *data)
{
	return _vbuffer_insert(buf, off, data) != NULL;
}

bool vbuffer_erase(struct vbuffer *buf, size_t off, size_t len)
{
	if (len == 0) {
		return true;
	}

	struct vbuffer *erased = vbuffer_extract(buf, off, len);
	if (!erased && check_error()) {
		return false;
	}

	vbuffer_free(erased);
	return true;
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

bool vbuffer_checksize(struct vbuffer *buf, size_t minsize)
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

			buf->data->ops->release(buf->data);
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

	if (iter) {
		if (!*iter) {
			*iter = buf;
		}
		else {
			*iter = (*iter)->next;
		}

		if (*iter) {
			if (len) *len = (*iter)->length;
			return vbuffer_get_data(*iter, write);
		}
		else {
			return NULL;
		}
	}
	else {
		if (len) *len = buf->length;
		return vbuffer_get_data(buf, write);
	}
}

struct vbuffer_data *vbuffer_mmap2(struct vbuffer *buf, void **_iter, size_t *len, size_t *offset)
{
	struct vbuffer **iter = (struct vbuffer **)_iter;
	assert(iter);

	if (!*iter) {
		*iter = buf;
	}
	else {
		*iter = (*iter)->next;
	}

	if (*iter) {
		*len = (*iter)->length;
		*offset = (*iter)->offset;
		return (*iter)->data;
	}
	else {
		return NULL;
	}
}


/*
 * Iterators
 */

bool vbuffer_iterator(struct vbuffer *buf, struct vbuffer_iterator *iter)
{
	iter->buffer = buf;
	iter->offset = 0;
	return true;
}

size_t vbuffer_iterator_read(struct vbuffer_iterator *iterator, uint8 *buffer, size_t len)
{
	size_t clen = len;
	struct vbuffer *iter = iterator->buffer;
	size_t offset = iterator->offset;
	iterator->offset = 0;

	if (!iter) {
		error(L"invalid buffer iterator");
		return (size_t)-1;
	}

	while (clen > 0) {
		size_t buflen = iter->length - offset;
		if (buflen > clen) {
			buflen = clen;
			iterator->offset = buflen;
		}

		memcpy(buffer, vbuffer_get_data(iter, false) + offset, buflen);
		buffer += buflen;
		clen -= buflen;
		offset = 0;

		iter = iter->next;
	}

	iterator->buffer = iter;

	return (len - clen);
}

bool vbuffer_iterator_insert(struct vbuffer_iterator *iter, struct vbuffer *data)
{
	struct vbuffer *last = _vbuffer_insert(iter->buffer, iter->offset, data);
	iter->buffer = last;
	iter->offset = last->length;
	return true;
}

bool vbuffer_iterator_erase(struct vbuffer_iterator *iter, size_t len)
{
	const bool ret = vbuffer_erase(iter->buffer, iter->offset, len);
	iter->buffer = iter->buffer->next;
	iter->offset = 0;
	return ret;
}

size_t vbuffer_iterator_advance(struct vbuffer_iterator *iterator, size_t len)
{
	size_t clen = len;
	struct vbuffer *iter = iterator->buffer, *last;
	size_t offset = iterator->offset;
	iterator->offset = 0;

	if (!iter || offset > iter->length) {
		error(L"invalid buffer iterator");
		return (size_t)-1;
	}

	while (iter) {
		const size_t buflen = iter->length - offset;
		assert(iter->length >= offset);

		last = iter;

		if (buflen >= clen) {
			iterator->offset = clen;
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

	if (!iter || offset > iter->length) {
		error(L"invalid buffer iterator");
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
	sub->position.buffer = buf;
	sub->position.offset = 0;
	vbuffer_iterator_advance(&sub->position, offset);

	if (length != (size_t)-1) {
		if (!vbuffer_checksize(sub->position.buffer, sub->position.offset + length)) {
			error(L"invalid size");
			return false;
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

	if (length != (size_t)-1) {
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
		struct vbuffer_iterator iter = buf->position;
		struct vbuffer *flat;
		uint8 *ptr;
		UNUSED size_t len;

		flat = vbuffer_create_new(buf->length);
		if (!flat) {
			return false;
		}

		ptr = vbuffer_mmap(flat, NULL, NULL, true);
		len = vbuffer_iterator_read(&iter, ptr, buf->length);
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

	if (!*iter) {
		*iter = buf->position.buffer;
		offset = buf->position.offset;
		*remlen = buf->length;
	}
	else {
		*iter = (*iter)->next;
		offset = 0;
	}

	if (!*iter) {
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
			clear_error();
			error(L"invalid sub buffer");
			return 0;
		}
	}
	else {
		struct vbuffer *buffer = buf->position.buffer;
		size_t offset = buf->position.offset;
		int i;

		for (i=0; i<buf->length; ++i, ++offset) {
			buffer = vbuffer_get(buffer, &offset, false);
			if (!buffer) {
				error(L"invalid sub buffer");
				return 0;
			}

			temp[i] = *(vbuffer_get_data(buffer, false) + offset);
		}

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
	if (vsubbuffer_isflat(buf)) {
		uint8 *ptr = vbuffer_iterator_get_data(&buf->position, true);
		if (!ptr) {
			clear_error();
			error(L"invalid sub buffer");
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
		struct vbuffer *buffer = buf->position.buffer;
		size_t offset = buf->position.offset;
		int i;

		switch (buf->length) {
		case 1: temp.i8 = num; break;
		case 2: temp.i16 = bigendian ? SWAP_TO_BE(int16, (int16)num) : SWAP_TO_LE(int16, (int16)num); break;
		case 4: temp.i32 = bigendian ? SWAP_TO_BE(int32, (int32)num) : SWAP_TO_LE(int32, (int32)num); break;
		case 8: temp.i64 = bigendian ? SWAP_TO_BE(int64, num) : SWAP_TO_LE(int64, num); break;
		default:
			error(L"unsupported size");
			return;
		}

		for (i=0; i<buf->length; ++i, ++offset) {
			uint8 *ptr;

			buffer = vbuffer_get(buffer, &offset, false);
			if (!buffer) {
				error(L"invalid sub buffer");
				return;
			}

			ptr = vbuffer_get_data(buffer, true);
			if (!ptr) {
				return;
			}

			*(ptr + offset) = temp.data[i];
		}
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
