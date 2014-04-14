/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/container/vector.h>
#include <haka/error.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>


#define SWAP_VAR(t, a, b, el)  \
	do {                       \
		t.el = (b)->el;        \
		(b)->el = (a)->el;     \
		(a)->el = t.el;        \
	} while(false);

void vector_destroy(struct vector *v)
{
	vector_resize(v, 0);
	vector_reserve(v, 0);
}

void vector_pop(struct vector *v)
{
	assert(v->count > 1);
	vector_resize(v, v->count-1);
}

bool vector_resize(struct vector *v, size_t count)
{
	if (count < v->count) {
		if (v->destruct) {
			int i;
			uint8 *iter = v->data;
			for (i=count, iter+=count*v->element_size; i<v->count; ++i, iter+=v->element_size) {
				v->destruct(iter);
			}
		}

		v->count = count;
		return true;
	}
	else if (count > v->count) {
		if (count <= v->allocated_count) {
			v->count = count;
			return true;
		}
		else {
			v->count = count;
			return vector_reserve(v, count);
		}
	}
	return true;
}

bool vector_reserve(struct vector *v, size_t count)
{
	if (count < v->count) {
		count = v->count;
	}

	if (count != v->allocated_count) {
		void *data = realloc(v->data, count*v->element_size);
		if (!data && count > 0) {
			free(v->data);
			v->data = NULL;
			error(L"memory error");
			return false;
		}
		v->allocated_count = count;
		v->data = data;
	}

	return true;
}

void vector_swap(struct vector *a, struct vector *b)
{
	struct vector tmp;

	assert(a);
	assert(b);

	SWAP_VAR(tmp, a, b, element_size);
	SWAP_VAR(tmp, a, b, count);
	SWAP_VAR(tmp, a, b, allocated_count);
	SWAP_VAR(tmp, a, b, data);
	SWAP_VAR(tmp, a, b, destruct);
}

bool _vector_create(struct vector *v, size_t elemsize, size_t reservecount, void (*destruct)(void *elem))
{
	v->element_size = elemsize;
	v->count = 0;
	v->allocated_count = reservecount;
	v->destruct = destruct;

	if (reservecount > 0) {
		v->data = malloc(v->element_size*reservecount);
		if (!v->data) {
			error(L"memory error");
			return false;
		}
	}
	else {
		v->data = NULL;
	}

	return true;
}

void *_vector_get(struct vector *v, size_t elemsize, int index)
{
	uint8 *iter = v->data;

	assert(v->data);
	assert(elemsize == v->element_size);
	assert(index >= 0 && index < v->count);

	return iter + v->element_size*index;
}

static bool vector_grow(struct vector *v)
{
	return vector_reserve(v, v->count*2+1);
}

void *_vector_push(struct vector *v, size_t elemsize)
{
	if (v->count == v->allocated_count) {
		if (!vector_grow(v)) {
			return NULL;
		}
	}

	vector_resize(v, v->count+1);
	return _vector_get(v, elemsize, v->count-1);
}
