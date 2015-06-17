/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_CONTAINER_VECTOR_H
#define HAKA_CONTAINER_VECTOR_H

#include <haka/compiler.h>
#include <haka/types.h>
#include <stddef.h>


struct vector {
	size_t         element_size;
	size_t         count;
	size_t         allocated_count;
	void          *data;
	void         (*destruct)(void *elem);
};

#define VECTOR_INIT(type, destruct)       {sizeof(type), 0, 0, NULL, destruct}

#define vector_create(v, type, destruct)                  _vector_create((v), sizeof(type), 0, (destruct))
#define vector_create_reserve(v, type, count, destruct)   _vector_create((v), sizeof(type), (count), (destruct))
#define vector_getvalue(v, type, index)         (*(type*)_vector_get((v), sizeof(type), (index)))
#define vector_get(v, type, index)              ((type*)_vector_get((v), sizeof(type), (index)))
#define vector_set(v, type, index, value)       (*(type*)_vector_get((v), sizeof(type), (index)) = (value))
#define vector_push(v, type)                    ((type*)_vector_push((v), sizeof(type)))
#define vector_last(v, type)                    ((type*)_vector_get((v), sizeof(type), vector_count(v)-))
#define vector_first(v, type)                   ((type*)_vector_get((v), sizeof(type), 0))

INLINE size_t vector_count(struct vector *v) { return v->count; }
INLINE bool   vector_isempty(struct vector *v) { return v->count == 0; }
void          vector_destroy(struct vector *v);
void          vector_pop(struct vector *v);
bool          vector_resize(struct vector *v, size_t count);
bool          vector_reserve(struct vector *v, size_t count);
void          vector_swap(struct vector *a, struct vector *b);

bool _vector_create(struct vector *v, size_t elemsize, size_t reservecount, void (*destruct)(void *elem));
void *_vector_get(struct vector *v, size_t elemsize, int index);
void *_vector_push(struct vector *v, size_t elemsize);

#endif /* HAKA_CONTAINER_VECTOR_H */
