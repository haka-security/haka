/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_CONTAINER_LIST_H
#define _HAKA_CONTAINER_LIST_H

#include <haka/compiler.h>
#include <stddef.h>


struct list {
	struct list    *prev;
	struct list    *next;
};

#define LIST_INIT    {NULL, NULL}

#define list_getuserptr(l, offset)    ((l) ? (void*)((char*)(l)-(offset)) : NULL)

#define list_init(a)                  { _list_init(&(a)->list); }
#define list_next(a)                  ((typeof(a))(list_getuserptr((a)->list.next, offsetof(typeof(*a), list))))
#define list_prev(a)                  ((typeof(a))(list_getuserptr((a)->list.prev, offsetof(typeof(*a), list))))
#define list_remove(a, h, t)           _list_remove(&(a)->list, offsetof(typeof(*a), list), (void**)(h), (void**)(t))
#define list_insert_after(a, i, h, t)  _list_insert_after(&(a)->list, (i) ? &(((typeof(a))(i))->list) : NULL, offsetof(typeof(*a), list), (void**)(h), (void**)(t))
#define list_insert_before(a, i, h, t) _list_insert_before(&(a)->list, (i) ? &(((typeof(a))(i))->list) : NULL, offsetof(typeof(*a), list), (void**)(h), (void**)(t))

void _list_init(struct list *l);
void _list_remove(struct list *l, int offset, void **head, void **tail);
void _list_insert_after(struct list *elem, struct list *l, int offset,
		void **head, void **tail);
void _list_insert_before(struct list *elem, struct list *l, int offset,
		void **head, void **tail);

#endif /* _HAKA_CONTAINER_LIST_H */
