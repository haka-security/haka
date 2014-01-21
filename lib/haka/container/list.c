/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/container/list.h>

#include <assert.h>
#include <stddef.h>


void _list_init(struct list *l)
{
	l->prev = NULL;
	l->next = NULL;
}

#define getuserptr(a)     ((a) ? (void*)((char*)(a)-offset) : NULL)
#define getlistptr(a)     ((a) ? ((struct list *)((char*)(a)+offset)) : NULL)

void _list_remove(struct list *l, int offset, void **head, void **tail)
{
	if (l->next) {
		l->next->prev = l->prev;
	}
	else {
		if (tail) {
			assert(*tail == getuserptr(l));
			*tail = getuserptr(l->prev);
		}
	}

	if (l->prev) {
		l->prev->next = l->next;
	}
	else {
		if (head) {
			assert(*head == getuserptr(l));
			*head = getuserptr(l->next);
		}
	}

	l->next = NULL;
	l->prev = NULL;
}

void _list_insert_after(struct list *elem, struct list *l, int offset,
		void **head, void **tail)
{
	assert(elem);
	assert(!elem->next);
	assert(!elem->prev);

	if (!l) {
		assert(tail);
		l = getlistptr(*tail);
		assert(!l || !(getlistptr(*tail)->next));
	}

	if (l) {
		elem->next = l->next;

		if (l->next) {
			l->next->prev = elem;
		}
		else {
			if (tail) {
				assert(*tail == getuserptr(l));
				*tail = getuserptr(elem);
			}
		}

		elem->prev = l;
		l->next = elem;
	}
	else {
		if (head) *head = getuserptr(elem);
		if (tail) *tail = getuserptr(elem);
	}
}

void _list_insert_before(struct list *elem, struct list *l, int offset,
		void **head, void **tail)
{
	assert(elem);
	assert(!elem->next);
	assert(!elem->prev);

	if (!l) {
		assert(head);
		l = getlistptr(*head);
		assert(!l || !(getlistptr(*head)->prev));
	}

	if (l) {
		elem->prev = l->prev;
		if (l->prev) {
			l->prev->next = elem;
		}
		else {
			if (head) {
				assert(*head == getuserptr(l));
				*head = getuserptr(elem);
			}
		}

		elem->next = l;
		l->prev = elem;
	}
	else {
		if (head) *head = getuserptr(elem);
		if (tail) *tail = getuserptr(elem);
	}
}
