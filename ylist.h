#pragma once

/*
 * Copyright (C) 2003-2012 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ydef.h"

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct ylist_head {
	struct ylist_head *next, *prev;
};

#define YLIST_HEAD_INIT(name) { &(name), &(name) }

#define YLIST_HEAD(name) \
	struct ylist_head name = YLIST_HEAD_INIT(name)

static inline void INIT_YLIST_HEAD(struct ylist_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct ylist_head *n,
			      struct ylist_head *prev,
			      struct ylist_head *next)
{
	next->prev = n;
	n->next = next;
	n->prev = prev;
	prev->next = n;
}

/**
 * list_add - add a new entry
 * @n: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void ylist_add(struct ylist_head *n, struct ylist_head *head)
{
	__list_add(n, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @n: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void ylist_add_tail(struct ylist_head *n, struct ylist_head *head)
{
	__list_add(n, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct ylist_head * prev, struct ylist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(struct ylist_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void ylist_del(struct ylist_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (struct ylist_head *)LIST_POISON1;
	entry->prev = (struct ylist_head *)LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @n : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void ylist_replace(struct ylist_head *old,
				struct ylist_head *n)
{
	n->next = old->next;
	n->next->prev = n;
	n->prev = old->prev;
	n->prev->next = n;
}

static inline void ylist_replace_init(struct ylist_head *old,
					struct ylist_head *n)
{
	ylist_replace(old, n);
	INIT_YLIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void ylist_del_init(struct ylist_head *entry)
{
	__list_del_entry(entry);
	INIT_YLIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void ylist_move(struct ylist_head *list, struct ylist_head *head)
{
	__list_del_entry(list);
	ylist_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void ylist_move_tail(struct ylist_head *list,
				  struct ylist_head *head)
{
	__list_del_entry(list);
	ylist_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int ylist_is_last(const struct ylist_head *list,
				const struct ylist_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int ylist_empty(const struct ylist_head *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int ylist_empty_careful(const struct ylist_head *head)
{
	struct ylist_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void ylist_rotate_left(struct ylist_head *head)
{
	struct ylist_head *first;

	if (!ylist_empty(head)) {
		first = head->next;
		ylist_move_tail(first, head);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int ylist_is_singular(const struct ylist_head *head)
{
	return !ylist_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct ylist_head *list,
		struct ylist_head *head, struct ylist_head *entry)
{
	struct ylist_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void ylist_cut_position(struct ylist_head *list,
		struct ylist_head *head, struct ylist_head *entry)
{
	if (ylist_empty(head))
		return;
	if (ylist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_YLIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct ylist_head *list,
				 struct ylist_head *prev,
				 struct ylist_head *next)
{
	struct ylist_head *first = list->next;
	struct ylist_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void ylist_splice(const struct ylist_head *list,
				struct ylist_head *head)
{
	if (!ylist_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct ylist_head *list,
				struct ylist_head *head)
{
	if (!ylist_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void ylist_splice_init(struct ylist_head *list,
				    struct ylist_head *head)
{
	if (!ylist_empty(list)) {
		__list_splice(list, head, head->next);
		INIT_YLIST_HEAD(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void ylist_splice_tail_init(struct ylist_head *list,
					 struct ylist_head *head)
{
	if (!ylist_empty(list)) {
		__list_splice(list, head->prev, head);
		INIT_YLIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct ylist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define ylist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define ylist_first_entry(ptr, type, member) \
	ylist_entry((ptr)->next, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct ylist_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define ylist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * __list_for_each	-	iterate over a list
 * @pos:	the &struct ylist_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant doesn't differ from list_for_each() any more.
 * We don't do prefetching in either case.
 */
#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct ylist_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define ylist_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct ylist_head to use as a loop cursor.
 * @n:		another &struct ylist_head to use as temporary storage
 * @head:	the head for your list.
 */
#define ylist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct ylist_head to use as a loop cursor.
 * @n:		another &struct ylist_head to use as temporary storage
 * @head:	the head for your list.
 */
#define ylist_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define ylist_for_each_entry(pos, head, member)				\
	for (pos = ylist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = ylist_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define ylist_for_each_entry_reverse(pos, head, member)			\
	for (pos = ylist_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = ylist_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define ylist_prepare_entry(pos, head, member) \
	((pos) ? : ylist_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define ylist_for_each_entry_continue(pos, head, member) 		\
	for (pos = ylist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = ylist_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define ylist_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = ylist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = ylist_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define ylist_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);	\
	     pos = ylist_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define ylist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = ylist_entry((head)->next, typeof(*pos), member),	\
		n = ylist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = ylist_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define ylist_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = ylist_entry(pos->member.next, typeof(*pos), member), 		\
		n = ylist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = ylist_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define ylist_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = ylist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = ylist_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define ylist_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = ylist_entry((head)->prev, typeof(*pos), member),	\
		n = ylist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = ylist_entry(n->member.prev, typeof(*n), member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the list_for_each_entry_safe loop
 * @n:		temporary storage used in list_for_each_entry_safe
 * @member:	the name of the list_struct within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define ylist_safe_reset_next(pos, n, member)				\
	n = ylist_entry(pos->member.next, typeof(*pos), member)

