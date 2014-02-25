/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_CONTAINER_LIST2_H
#define _HAKA_CONTAINER_LIST2_H

#include <haka/compiler.h>
#include <haka/types.h>
#include <stddef.h>
#include <assert.h>


struct list2_elem {
	struct list2_elem    *prev;
	struct list2_elem    *next;
#ifdef HAKA_DEBUG
	bool                 is_end:1;
	bool                 is_elem:1;
#endif
};

struct list2 {
	struct list2_elem     head;
};

#define LIST2_INIT  { LIST2_ELEM_INIT }

#define LIST2_ELEM_INIT  { NULL, NULL }

INLINE void       list2_elem_init(struct list2_elem *elem)
{
	elem->next = elem->prev = NULL;
#ifdef HAKA_DEBUG
	elem->is_end = false;
	elem->is_elem = true;
#endif
}

INLINE bool       list2_elem_check(struct list2_elem *elem) { return elem->next && elem->prev; }

typedef struct list2_elem *list2_iter;

INLINE list2_iter list2_begin(const struct list2 *list) { return list->head.next; }
INLINE list2_iter list2_end(const struct list2 *list) { return (list2_iter)&list->head; }
INLINE list2_iter list2_next(const list2_iter iter) { return iter->next; }
INLINE list2_iter list2_prev(const list2_iter iter) { return iter->prev; }

void              list2_init(struct list2 *list);
list2_iter        list2_insert(list2_iter iter, struct list2_elem *list);
list2_iter        list2_insert_list(list2_iter iter, list2_iter begin, list2_iter end);
list2_iter        list2_erase(list2_iter list);
INLINE bool       list2_empty(const struct list2 *list) { return list2_begin(list) == list2_end(list); }
void              list2_swap(struct list2 *a, struct list2 *b);

INLINE void      *_list2_getuserptr(struct list2_elem *l, size_t offset)
{
#ifdef HAKA_DEBUG
	assert(l->is_elem);
#endif
	return ((l) ? (void *)((char *)(l)-(offset)) : NULL);
}

#define list2_get(iter, type, list)          ((type*)_list2_getuserptr(iter, offsetof(type, list)))
#define list2_first(l, type, list)           ((type*)(list2_empty((l)) ? NULL : _list2_getuserptr(list2_begin(l), offsetof(type, list))))
#define list2_last(l, type, list)            ((type*)(list2_empty((l)) ? NULL : _list2_getuserptr(list2_prev(list2_end(l)), offsetof(type, list))))

#endif /* _HAKA_CONTAINER_LIST2_H */
