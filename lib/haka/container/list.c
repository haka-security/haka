
#include <haka/container/list.h>

#include <assert.h>
#include <stddef.h>


void _list_init(struct list *l)
{
	l->prev = NULL;
	l->next = NULL;
}

void _list_remove(struct list *l, struct list **head, struct list **tail)
{
	if (l->next) {
		l->next->prev = l->prev;
	}
	else {
		if (tail) {
			assert(*tail == l);
			*tail = l->prev;
		}
	}

	if (l->prev) {
		l->prev->next = l->next;
	}
	else {
		if (head) {
			assert(*head == l);
			*head = l->next;
		}
	}

	l->next = NULL;
	l->prev = NULL;
}

void _list_insert_after(struct list *elem, struct list *l,
		struct list **head, struct list **tail)
{
	assert(elem);
	assert(!elem->next);
	assert(!elem->prev);

	if (!l) {
		assert(tail);
		l = *tail;
		assert(!l || !(*tail)->next);
	}

	if (l) {
		elem->next = l->next;

		if (l->next) {
			l->next->prev = elem;
		}
		else {
			if (tail) {
				assert(*tail == l);
				*tail = elem;
			}
		}

		elem->prev = l;
		l->next = elem;
	}
	else {
		if (head) *head = elem;
		if (tail) *tail = elem;
	}
}

void _list_insert_before(struct list *elem, struct list *l,
		struct list **head, struct list **tail)
{
	assert(elem);
	assert(!elem->next);
	assert(!elem->prev);

	if (!l) {
		assert(head);
		l = *head;
		assert(!l || !(*head)->prev);
	}

	if (l) {
		elem->prev = l->prev;
		if (l->prev) {
			l->prev->next = elem;
		}
		else {
			if (head) {
				assert(*head == l);
				*head = elem;
			}
		}

		elem->next = l;
		l->prev = elem;
	}
	else {
		if (head) *head = elem;
		if (tail) *tail = elem;
	}
}
