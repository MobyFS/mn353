#ifndef _LIST_H
#define _LIST_H

typedef struct list_t
{
	struct list_t* next;
	struct list_t* prev;
}list_t;

#define list_for_each(pos, head)  for (pos=(head)->next; pos != (head); pos=pos->next)
#define list_entry(p, type)   (type*)((char*)(p) - (int)(&((type*)0)->list))

inline void list_init(list_t* head)
{
	head->next = head;
	head->prev = head;
}

inline void list_add(list_t* head, list_t* node)
{
	node->next = head;
	node->prev = head->prev;

	head->prev->next = node;
	head->prev = node;
}

inline void list_remove(list_t* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;

	node->next = node;
	node->prev = node;
}

inline void list_insert_forwart(list_t* n, list_t* n_new)
{
	n_new->next = n;
	n_new->prev = n->prev;

	n->prev->next = n_new;
	n->prev = n_new;
}

inline void list_add_head(list_t* h, list_t* n)
{
	n->next = h->next;
	n->prev = h;
	h->next->prev = n;
	h->next = n;
}

inline int list_is_empty(list_t* head)
{
	return head->next == head ? 1 : 0;
}

#endif // _LIST_H
