
#ifndef _HAKA_CONTAINER_LIST_H
#define _HAKA_CONTAINER_LIST_H

#include <haka/compiler.h>
#include <stddef.h>


struct list {
	struct list    *prev;
	struct list    *next;
};


#define list_init(a)                  { STATIC_ASSERT(offsetof(typeof(*a), list)==0, invalid_list_head); _list_init(&(a)->list); }
#define list_next(a)                  ((typeof(a))(a)->list.next)
#define list_prev(a)                  ((typeof(a))(a)->list.prev)
#define list_remove(a, h, t)           _list_remove(&(a)->list, (struct list**)(h), (struct list**)(t))
#define list_insert_after(a, i, h, t)  _list_insert_after(&(a)->list, (struct list*)(i), (struct list**)(h), (struct list**)(t))
#define list_insert_before(a, i, h, t) _list_insert_before(&(a)->list, (struct list*)(i), (struct list**)(h), (struct list**)(t))


void _list_init(struct list *l);
void _list_remove(struct list *l, struct list **head, struct list **tail);
void _list_insert_after(struct list *elem, struct list *l,
		struct list **head, struct list **tail);
void _list_insert_before(struct list *elem, struct list *l,
		struct list **head, struct list **tail);

#endif /*/ _HAKA_CONTAINER_LIST_H */
