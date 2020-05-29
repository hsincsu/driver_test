/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIST_H
#define LIST_H

//added by hs
/*POLL FORM 5.0.5 KERNEL , TO USE USERLIST IN USESPACE --BY HS*/

//add some definition for userlist_head or huserlist_head from include/linux/type.h

struct userlist_head {
        struct userlist_head *next, *prev;
};

struct huserlist_head {
        struct huserlist_node *first;
};

struct huserlist_node {
        struct huserlist_node *next, **pprev;
};

//add from /include/linux/poison.h
/*      
 * Architectures might want to move the poison pointer offset
 * into some well-recognized area such as 0xdead000000000000,
 * that is also not mappable by user-space exploits:
 */     
#ifdef CONFIG_ILLEGAL_POINTER_VALUE // THIS IS FOR DEBUG userlist,SO NOT USED IN USER --BY HS
# define POISON_POINTER_DELTA _AC(CONFIG_ILLEGAL_POINTER_VALUE, UL)
#else
# define POISON_POINTER_DELTA 0
#endif


/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized userlist entries.
 */     
#define USERLIST_POISON1  ((void *) 0x100 + POISON_POINTER_DELTA)
#define USERLIST_POISON2  ((void *) 0x200 + POISON_POINTER_DELTA)




#define USERLIST_HEAD_INIT(name) { &(name), &(name) }

#define USERLIST_HEAD(name) \
	struct userlist_head name = USERLIST_HEAD_INIT(name)


static inline void INIT_USERLIST_HEAD(struct userlist_head *userlist)
{
	userlist->next = userlist;
	userlist->prev = userlist;
}

#ifdef CONFIG_DEBUG_USERLIST_USER // changed by hs
extern bool __userlist_add_valid(struct userlist_head *new,
			      struct userlist_head *prev,
			      struct userlist_head *next);
extern bool __userlist_del_entry_valid(struct userlist_head *entry);
#else
static inline bool __userlist_add_valid(struct userlist_head *new,
				struct userlist_head *prev,
				struct userlist_head *next)
{
	return true;
}
static inline bool __userlist_del_entry_valid(struct userlist_head *entry)
{
	return true;
}
#endif

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal userlist manipulation where we know
 * the prev/next entries already!
 */
static inline void __userlist_add(struct userlist_head *new,
			      struct userlist_head *prev,
			      struct userlist_head *next)
{
	if (!__userlist_add_valid(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new; //added by hs 
	//WRITE_ONCE(prev->next, new);//del by hs
}

/**
 * userlist_add - add a new entry
 * @new: new entry to be added
 * @head: userlist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void userlist_add(struct userlist_head *new, struct userlist_head *head)
{
	__userlist_add(new, head, head->next);
}


/**
 * userlist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: userlist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void userlist_add_tail(struct userlist_head *new, struct userlist_head *head)
{
	__userlist_add(new, head->prev, head);
}

/*
 * Delete a userlist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal userlist manipulation where we know
 * the prev/next entries already!
 */
static inline void __userlist_del(struct userlist_head * prev, struct userlist_head * next)
{
	next->prev = prev;
	prev->next = next;//added by hs
	//WRITE_ONCE(prev->next, next); //del by hs
}

/**
 * userlist_del - deletes entry from userlist.
 * @entry: the element to delete from the userlist.
 * Note: userlist_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __userlist_del_entry(struct userlist_head *entry)
{
	if (!__userlist_del_entry_valid(entry))
		return;

	__userlist_del(entry->prev, entry->next);
}

static inline void userlist_del(struct userlist_head *entry)
{
	__userlist_del_entry(entry);
	entry->next = USERLIST_POISON1;
	entry->prev = USERLIST_POISON2;
}

/**
 * userlist_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void userlist_replace(struct userlist_head *old,
				struct userlist_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void userlist_replace_init(struct userlist_head *old,
					struct userlist_head *new)
{
	userlist_replace(old, new);
	INIT_USERLIST_HEAD(old);
}

/**
 * userlist_del_init - deletes entry from userlist and reinitialize it.
 * @entry: the element to delete from the userlist.
 */
static inline void userlist_del_init(struct userlist_head *entry)
{
	__userlist_del_entry(entry);
	INIT_USERLIST_HEAD(entry);
}

/**
 * userlist_move - delete from one userlist and add as another's head
 * @userlist: the entry to move
 * @head: the head that will precede our entry
 */
static inline void userlist_move(struct userlist_head *userlist, struct userlist_head *head)
{
	__userlist_del_entry(userlist);
	userlist_add(userlist, head);
}

/**
 * userlist_move_tail - delete from one userlist and add as another's tail
 * @userlist: the entry to move
 * @head: the head that will follow our entry
 */
static inline void userlist_move_tail(struct userlist_head *userlist,
				  struct userlist_head *head)
{
	__userlist_del_entry(userlist);
	userlist_add_tail(userlist, head);
}

/**
 * userlist_bulk_move_tail - move a subsection of a userlist to its tail
 * @head: the head that will follow our entry
 * @first: first entry to move
 * @last: last entry to move, can be the same as first
 *
 * Move all entries between @first and including @last before @head.
 * All three entries must belong to the same linked userlist.
 */
static inline void userlist_bulk_move_tail(struct userlist_head *head,
				       struct userlist_head *first,
				       struct userlist_head *last)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

/**
 * userlist_is_last - tests whether @userlist is the last entry in userlist @head
 * @userlist: the entry to test
 * @head: the head of the userlist
 */
static inline int userlist_is_last(const struct userlist_head *userlist,
				const struct userlist_head *head)
{
	return userlist->next == head;
}

/**
 * userlist_empty - tests whether a userlist is empty
 * @head: the userlist to test.
 */
static inline int userlist_empty(const struct userlist_head *head)
{
	  return head->next == head; //added by hs
	//return READ_ONCE(head->next) == head; //del by hs
}

/**
 * userlist_empty_careful - tests whether a userlist is empty and not being modified
 * @head: the userlist to test
 *
 * Description:
 * tests whether a userlist is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using userlist_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the userlist entry is userlist_del_init(). Eg. it cannot be used
 * if another CPU could re-userlist_add() it.
 */
static inline int userlist_empty_careful(const struct userlist_head *head)
{
	struct userlist_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * userlist_rotate_left - rotate the userlist to the left
 * @head: the head of the userlist
 */
static inline void userlist_rotate_left(struct userlist_head *head)
{
	struct userlist_head *first;

	if (!userlist_empty(head)) {
		first = head->next;
		userlist_move_tail(first, head);
	}
}

/**
 * userlist_is_singular - tests whether a userlist has just one entry.
 * @head: the userlist to test.
 */
static inline int userlist_is_singular(const struct userlist_head *head)
{
	return !userlist_empty(head) && (head->next == head->prev);
}

static inline void __userlist_cut_position(struct userlist_head *userlist,
		struct userlist_head *head, struct userlist_head *entry)
{
	struct userlist_head *new_first = entry->next;
	userlist->next = head->next;
	userlist->next->prev = userlist;
	userlist->prev = entry;
	entry->next = userlist;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * userlist_cut_position - cut a userlist into two
 * @userlist: a new userlist to add all removed entries
 * @head: a userlist with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the userlist
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @userlist. You should
 * pass on @entry an element you know is on @head. @userlist
 * should be an empty userlist or a userlist you do not care about
 * losing its data.
 *
 */
static inline void userlist_cut_position(struct userlist_head *userlist,
		struct userlist_head *head, struct userlist_head *entry)
{
	if (userlist_empty(head))
		return;
	if (userlist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_USERLIST_HEAD(userlist);
	else
		__userlist_cut_position(userlist, head, entry);
}

/**
 * userlist_cut_before - cut a userlist into two, before given entry
 * @userlist: a new userlist to add all removed entries
 * @head: a userlist with entries
 * @entry: an entry within head, could be the head itself
 *
 * This helper moves the initial part of @head, up to but
 * excluding @entry, from @head to @userlist.  You should pass
 * in @entry an element you know is on @head.  @userlist should
 * be an empty userlist or a userlist you do not care about losing
 * its data.
 * If @entry == @head, all entries on @head are moved to
 * @userlist.
 */
static inline void userlist_cut_before(struct userlist_head *userlist,
				   struct userlist_head *head,
				   struct userlist_head *entry)
{
	if (head->next == entry) {
		INIT_USERLIST_HEAD(userlist);
		return;
	}
	userlist->next = head->next;
	userlist->next->prev = userlist;
	userlist->prev = entry->prev;
	userlist->prev->next = userlist;
	head->next = entry;
	entry->prev = head;
}

static inline void __userlist_splice(const struct userlist_head *userlist,
				 struct userlist_head *prev,
				 struct userlist_head *next)
{
	struct userlist_head *first = userlist->next;
	struct userlist_head *last = userlist->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * userlist_splice - join two userlists, this is designed for stacks
 * @userlist: the new userlist to add.
 * @head: the place to add it in the first userlist.
 */
static inline void userlist_splice(const struct userlist_head *userlist,
				struct userlist_head *head)
{
	if (!userlist_empty(userlist))
		__userlist_splice(userlist, head, head->next);
}

/**
 * userlist_splice_tail - join two userlists, each userlist being a queue
 * @userlist: the new userlist to add.
 * @head: the place to add it in the first userlist.
 */
static inline void userlist_splice_tail(struct userlist_head *userlist,
				struct userlist_head *head)
{
	if (!userlist_empty(userlist))
		__userlist_splice(userlist, head->prev, head);
}

/**
 * userlist_splice_init - join two userlists and reinitialise the emptied userlist.
 * @userlist: the new userlist to add.
 * @head: the place to add it in the first userlist.
 *
 * The userlist at @userlist is reinitialised
 */
static inline void userlist_splice_init(struct userlist_head *userlist,
				    struct userlist_head *head)
{
	if (!userlist_empty(userlist)) {
		__userlist_splice(userlist, head, head->next);
		INIT_USERLIST_HEAD(userlist);
	}
}

/**
 * userlist_splice_tail_init - join two userlists and reinitialise the emptied userlist
 * @userlist: the new userlist to add.
 * @head: the place to add it in the first userlist.
 *
 * Each of the userlists is a queue.
 * The userlist at @userlist is reinitialised
 */
static inline void userlist_splice_tail_init(struct userlist_head *userlist,
					 struct userlist_head *head)
{
	if (!userlist_empty(userlist)) {
		__userlist_splice(userlist, head->prev, head);
		INIT_USERLIST_HEAD(userlist);
	}
}

/**
 * userlist_entry - get the struct for this entry
 * @ptr:	the &struct userlist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * userlist_first_entry - get the first element from a userlist
 * @ptr:	the userlist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the userlist_head within the struct.
 *
 * Note, that userlist is expected to be not empty.
 */
#define userlist_first_entry(ptr, type, member) \
	userlist_entry((ptr)->next, type, member)

/**
 * userlist_last_entry - get the last element from a userlist
 * @ptr:	the userlist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the userlist_head within the struct.
 *
 * Note, that userlist is expected to be not empty.
 */
#define userlist_last_entry(ptr, type, member) \
	userlist_entry((ptr)->prev, type, member)

/**
 * userlist_first_entry_or_null - get the first element from a userlist
 * @ptr:	the userlist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the userlist_head within the struct.
 *
 * Note that if the userlist is empty, it returns NULL.
 */
#define userlist_first_entry_or_null(ptr, type, member) ({ \
	struct userlist_head *head__ = (ptr); \
	struct userlist_head *pos__ = head__->next; \
	pos__ != head__ ? userlist_entry(pos__, type, member) : NULL; \
/*	struct userlist_head *pos__ = READ_ONCE(head__->next); \
	pos__ != head__ ? userlist_entry(pos__, type, member) : NULL; \*/
})

/**
 * userlist_next_entry - get the next element in userlist
 * @pos:	the type * to cursor
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_next_entry(pos, member) \
	userlist_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * userlist_prev_entry - get the prev element in userlist
 * @pos:	the type * to cursor
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_prev_entry(pos, member) \
	userlist_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * userlist_for_each	-	iterate over a userlist
 * @pos:	the &struct userlist_head to use as a loop cursor.
 * @head:	the head for your userlist.
 */
#define userlist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * userlist_for_each_prev	-	iterate over a userlist backwards
 * @pos:	the &struct userlist_head to use as a loop cursor.
 * @head:	the head for your userlist.
 */
#define userlist_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * userlist_for_each_safe - iterate over a userlist safe against removal of userlist entry
 * @pos:	the &struct userlist_head to use as a loop cursor.
 * @n:		another &struct userlist_head to use as temporary storage
 * @head:	the head for your userlist.
 */
#define userlist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * userlist_for_each_prev_safe - iterate over a userlist backwards safe against removal of userlist entry
 * @pos:	the &struct userlist_head to use as a loop cursor.
 * @n:		another &struct userlist_head to use as temporary storage
 * @head:	the head for your userlist.
 */
#define userlist_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * userlist_for_each_entry	-	iterate over userlist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_for_each_entry(pos, head, member)				\
	for (pos = userlist_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = userlist_next_entry(pos, member))

/**
 * userlist_for_each_entry_reverse - iterate backwards over userlist of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_for_each_entry_reverse(pos, head, member)			\
	for (pos = userlist_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = userlist_prev_entry(pos, member))

/**
 * userlist_prepare_entry - prepare a pos entry for use in userlist_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the userlist
 * @member:	the name of the userlist_head within the struct.
 *
 * Prepares a pos entry for use as a start point in userlist_for_each_entry_continue().
 */
#define userlist_prepare_entry(pos, head, member) \
	((pos) ? : userlist_entry(head, typeof(*pos), member))

/**
 * userlist_for_each_entry_continue - continue iteration over userlist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Continue to iterate over userlist of given type, continuing after
 * the current position.
 */
#define userlist_for_each_entry_continue(pos, head, member) 		\
	for (pos = userlist_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = userlist_next_entry(pos, member))

/**
 * userlist_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Start to iterate over userlist of given type backwards, continuing after
 * the current position.
 */
#define userlist_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = userlist_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = userlist_prev_entry(pos, member))

/**
 * userlist_for_each_entry_from - iterate over userlist of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Iterate over userlist of given type, continuing from current position.
 */
#define userlist_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = userlist_next_entry(pos, member))

/**
 * userlist_for_each_entry_from_reverse - iterate backwards over userlist of given type
 *                                    from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Iterate backwards over userlist of given type, continuing from current position.
 */
#define userlist_for_each_entry_from_reverse(pos, head, member)		\
	for (; &pos->member != (head);					\
	     pos = userlist_prev_entry(pos, member))

/**
 * userlist_for_each_entry_safe - iterate over userlist of given type safe against removal of userlist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 */
#define userlist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = userlist_first_entry(head, typeof(*pos), member),	\
		n = userlist_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = userlist_next_entry(n, member))

/**
 * userlist_for_each_entry_safe_continue - continue userlist iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Iterate over userlist of given type, continuing after current point,
 * safe against removal of userlist entry.
 */
#define userlist_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = userlist_next_entry(pos, member), 				\
		n = userlist_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = userlist_next_entry(n, member))

/**
 * userlist_for_each_entry_safe_from - iterate over userlist from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Iterate over userlist of given type from current point, safe against
 * removal of userlist entry.
 */
#define userlist_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = userlist_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = userlist_next_entry(n, member))

/**
 * userlist_for_each_entry_safe_reverse - iterate backwards over userlist safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your userlist.
 * @member:	the name of the userlist_head within the struct.
 *
 * Iterate backwards over userlist of given type, safe against removal
 * of userlist entry.
 */
#define userlist_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = userlist_last_entry(head, typeof(*pos), member),		\
		n = userlist_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = userlist_prev_entry(n, member))

/**
 * userlist_safe_reset_next - reset a stale userlist_for_each_entry_safe loop
 * @pos:	the loop cursor used in the userlist_for_each_entry_safe loop
 * @n:		temporary storage used in userlist_for_each_entry_safe
 * @member:	the name of the userlist_head within the struct.
 *
 * userlist_safe_reset_next is not safe to use in general if the userlist may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the userlist,
 * and userlist_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define userlist_safe_reset_next(pos, n, member)				\
	n = userlist_next_entry(pos, member)

/*
 * Double linked userlists with a single pointer userlist head.
 * Mostly useful for hash tables where the two pointer userlist head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HUSERLIST_HEAD_INIT { .first = NULL }
#define HUSERLIST_HEAD(name) struct huserlist_head name = {  .first = NULL }
#define INIT_HUSERLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HUSERLIST_NODE(struct huserlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int huserlist_unhashed(const struct huserlist_node *h)
{
	return !h->pprev;
}

static inline int huserlist_empty(const struct huserlist_head *h)
{
	return !h->first;//added by hs
	//return !READ_ONCE(h->first); // del by hs
}

static inline void __huserlist_del(struct huserlist_node *n)
{
	struct huserlist_node *next = n->next;
	struct huserlist_node **pprev = n->pprev;

	//WRITE_ONCE(*pprev, next);
	*pprev = next; //added by hs
	if (next)
		next->pprev = pprev;
}

static inline void huserlist_del(struct huserlist_node *n)
{
	__huserlist_del(n);
	n->next = USERLIST_POISON1;
	n->pprev = USERLIST_POISON2;
}

static inline void huserlist_del_init(struct huserlist_node *n)
{
	if (!huserlist_unhashed(n)) {
		__huserlist_del(n);
		INIT_HUSERLIST_NODE(n);
	}
}

static inline void huserlist_add_head(struct huserlist_node *n, struct huserlist_head *h)
{
	struct huserlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	//WRITE_ONCE(h->first, n);
	h->first = n; //added by hs
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void huserlist_add_before(struct huserlist_node *n,
					struct huserlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
	//WRITE_ONCE(*(n->pprev), n);
}

static inline void huserlist_add_behind(struct huserlist_node *n,
				    struct huserlist_node *prev)
{
	n->next = prev->next;
	prev->next = n; //added by hs
	//WRITE_ONCE(prev->next, n);
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev  = &n->next;
}

/* after that we'll appear to be on some huserlist and huserlist_del will work */
static inline void huserlist_add_fake(struct huserlist_node *n)
{
	n->pprev = &n->next;
}

static inline bool huserlist_fake(struct huserlist_node *h)
{
	return h->pprev == &h->next;
}

/*
 * Check whether the node is the only node of the head without
 * accessing head:
 */
static inline bool
huserlist_is_singular_node(struct huserlist_node *n, struct huserlist_head *h)
{
	return !n->next && n->pprev == &h->first;
}

/*
 * Move a userlist from one userlist head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void huserlist_move_userlist(struct huserlist_head *old,
				   struct huserlist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define huserlist_entry(ptr, type, member) container_of(ptr,type,member)

#define huserlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define huserlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define huserlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? huserlist_entry(____ptr, type, member) : NULL; \
	})

/**
 * huserlist_for_each_entry	- iterate over userlist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your userlist.
 * @member:	the name of the huserlist_node within the struct.
 */
#define huserlist_for_each_entry(pos, head, member)				\
	for (pos = huserlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = huserlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * huserlist_for_each_entry_continue - iterate over a huserlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the huserlist_node within the struct.
 */
#define huserlist_for_each_entry_continue(pos, member)			\
	for (pos = huserlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = huserlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * huserlist_for_each_entry_from - iterate over a huserlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the huserlist_node within the struct.
 */
#define huserlist_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = huserlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * huserlist_for_each_entry_safe - iterate over userlist of given type safe against removal of userlist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct huserlist_node to use as temporary storage
 * @head:	the head for your userlist.
 * @member:	the name of the huserlist_node within the struct.
 */
#define huserlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = huserlist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = huserlist_entry_safe(n, typeof(*pos), member))

#endif
