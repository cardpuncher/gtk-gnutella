/*
 * $Id$
 *
 * Copyright (c) 2003, Christian Biere
 *
 *----------------------------------------------------------------------
 * This file is part of gtk-gnutella.
 *
 *  gtk-gnutella is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gtk-gnutella is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gtk-gnutella; if not, write to the Free Software
 *  Foundation, Inc.:
 *      59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *----------------------------------------------------------------------
 */

/**
 * @file
 *
 * An hashlist is a dual structure where data are both stored in a two-way
 * list, preserving ordering, and indexed in a hash table.
 *
 * This structure can quickly determine whether it contains some piece of
 * data, as well as quickly remove data.  It can be iterated over, in the
 * order of the items or in reverse order.
 */

#include "common.h"

RCSID("$Id$");

#include "hashlist.h"
#include "glib-missing.h"
#include "walloc.h"
#include "override.h"		/* Must be the last header included */

typedef enum {
	HASH_LIST_MAGIC = 0x338954fdU
} hash_list_magic_t;

struct hash_list {
	GList *l;
	GHashTable *ht; 
	GList *last;
	gint len;
	gint refcount;
	guint stamp;
	hash_list_magic_t magic;
};

struct hash_list_iter {
	hash_list_t *hl; 
	GList *l;
	gint pos;
	gint move;
	guint stamp;
};

#if 0
#define USE_HASH_LIST_REGRESSION 1
#endif

#ifdef USE_HASH_LIST_REGRESSION
static void inline hash_list_regression(const hash_list_t *hl)
{
	g_assert(HASH_LIST_MAGIC == hl->magic);
	g_assert(NULL != hl->ht);
	g_assert(g_list_first(hl->l) == hl->l);
	g_assert(g_list_first(hl->last) == hl->l);
	g_assert(g_list_last(hl->l) == hl->last);
	g_assert(g_list_length(hl->l) == hl->len);
	g_assert(g_hash_table_size(hl->ht) == hl->len);
}
#else
#define hash_list_regression(hl)
#endif

/*
 * With TRACK_MALLOC, the routines hash_list_new() and hash_list_free()
 * are trapped by macros, but the routines need to be defined here,
 * since they are called directly from within malloc.c.
 */
#ifdef TRACK_MALLOC
#undef hash_list_new
#undef hash_list_free
#endif

/*
 * If walloc() and wfree() are remapped to malloc routines and they enabled
 * TRACK_MALLOC as well, then hash_list_new() and hash_list_free() are
 * wrapped within malloc.c, and the recording of the allocated descriptors
 * happens there.  So we must NOT use g_malloc() and g_free() but use
 * raw malloc() and free() instead, to avoid duplicate tracking.
 */
#if defined(REMAP_ZALLOC) && defined(TRACK_MALLOC)
#undef walloc
#undef wfree
#undef malloc
#undef free
#define walloc(s)	malloc(s)
#define wfree(p,s)	free(p)
#endif

/**
 * Create a new hash list.
 */
hash_list_t *hash_list_new(void)
{
	hash_list_t *hl = walloc(sizeof(*hl));
	hl->l = NULL;
	hl->ht = g_hash_table_new(NULL, NULL);
	hl->last = NULL;
	hl->refcount = 1;
	hl->len = 0;
	hl->stamp = HASH_LIST_MAGIC + 1;
	hl->magic = HASH_LIST_MAGIC;
	hash_list_regression(hl);

	return hl;
}

/**
 * Dispose of the data structure, but not of the items it holds.
 */
void hash_list_free(hash_list_t *hl)
{
	g_assert(NULL != hl);
	hash_list_regression(hl);

	if (--hl->refcount != 0) {
		g_warning("hash_list_free: hash list is still referenced! "
			"(hl=%p, hl->refcount=%d)", hl, hl->refcount);
	}
	g_hash_table_destroy(hl->ht);
	g_list_free(hl->l);
	hl->ht = NULL; 
	hl->l = NULL;
	hl->len = 0;

	wfree(hl, sizeof(*hl));
}

/**
 * Append `data' to the list.
 */
void hash_list_append(hash_list_t *hl, gpointer data)
{
	g_assert(NULL != data);
	g_assert(NULL != hl);
	g_assert(1 == hl->refcount);
	hash_list_regression(hl);

	hl->last = g_list_last(g_list_append(hl->last, data));
	if (NULL == hl->l)
		hl->l = hl->last;
	g_assert(NULL == g_hash_table_lookup(hl->ht, data));
	g_hash_table_insert(hl->ht, data, hl->last);
	hl->len++;
	hl->stamp++;

	hash_list_regression(hl);
}

/**
 * Prepend `data' to the list.
 */
void hash_list_prepend(hash_list_t *hl, gpointer data)
{
	g_assert(NULL != data);
	g_assert(NULL != hl);
	g_assert(1 == hl->refcount);
	hash_list_regression(hl);

	hl->l = g_list_prepend(hl->l, data);
	if (NULL == hl->last)
		hl->last = hl->l;
	g_assert(NULL == g_hash_table_lookup(hl->ht, data));
	g_hash_table_insert(hl->ht, data, hl->l);
	hl->len++;
	hl->stamp++;

	hash_list_regression(hl);
}

/**
 * Remove `data' from the list.
 */
void hash_list_remove(hash_list_t *hl, gpointer data)
{
	GList *l;

	g_assert(NULL != data);
	g_assert(1 == hl->refcount);
	hash_list_regression(hl);

	l = (GList *) g_hash_table_lookup(hl->ht, data);
	g_assert(NULL != l);
	if (hl->last == l) {
		hl->last = g_list_previous(hl->last);
	}
	hl->l = g_list_delete_link(hl->l, l);
	g_hash_table_remove(hl->ht, data);
	hl->len--;
	hl->stamp++;

	hash_list_regression(hl);
}

/**
 * Returns the last item of the list, or NULL if none.
 */
gpointer hash_list_last(const hash_list_t *hl)
{
	g_assert(NULL != hl);
	g_assert(hl->refcount > 0);
	hash_list_regression(hl);

	return NULL != hl->last ? hl->last->data : NULL;
} 

/**
 * Returns the first item of the list, or NULL if none.
 */
gpointer hash_list_first(const hash_list_t *hl)
{
	g_assert(NULL != hl);
	g_assert(hl->refcount > 0);
	hash_list_regression(hl);

	return NULL != hl->l ? hl->l->data : NULL;
} 

/**
 * Returns the length of the list.
 */
gint hash_list_length(const hash_list_t *hl)
{
	g_assert(NULL != hl);
	g_assert(hl->refcount > 0);
	hash_list_regression(hl);

	return hl->len;
}

/**
 * Get an iterator on the list, positionned before first item.
 * Get items with hash_list_next().
 */
hash_list_iter_t *hash_list_iterator(hash_list_t *hl)
{
	hash_list_iter_t *i;

	g_assert(NULL != hl);
	g_assert(hl->refcount > 0);

	i = walloc(sizeof(*i));

	i->hl = hl;
	i->l = hl->l;
	i->stamp = hl->stamp;
	i->pos = -1;				/* Before first item */
	i->move = +1;
	hl->refcount++;

	return i;
}

/**
 * Get an iterator on the list, positionned after last item.
 * Get items with hash_list_previous().
 */
hash_list_iter_t *hash_list_iterator_last(hash_list_t *hl)
{
	hash_list_iter_t *i;

	g_assert(NULL != hl);
	g_assert(hl->refcount > 0);

	i = walloc(sizeof(*i));
	i->hl = hl;
	i->l = hl->last;
	i->stamp = hl->stamp;
	i->pos = hl->len;			/* After last item */
	i->move = -1;
	hl->refcount++;

	return i;
}

/**
 * Get the next data item from the iterator, or NULL if none.
 */
gpointer hash_list_next(hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);
	g_assert(i->hl->refcount > 0);
	g_assert(i->hl->stamp == i->stamp);
	g_assert(i->pos < i->hl->len);

	if (i->pos++ >= 0)					/* Special case if "before" first */
		i->l = g_list_next(i->l);

	return NULL != i->l ? i->l->data : NULL;
}

/**
 * Checks whether there is a next item to be iterated over.
 */
gboolean hash_list_has_next(const hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);
	g_assert(i->hl->refcount > 0);
	g_assert(i->hl->stamp == i->stamp);

	return i->hl->len && i->pos < (i->hl->len - 1);
}

/**
 * Get the previous data item from the iterator, or NULL if none.
 */
gpointer hash_list_previous(hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);
	g_assert(i->hl->refcount > 0);
	g_assert(i->hl->stamp == i->stamp);
	g_assert(i->pos >= 0);

	if (i->pos-- < i->hl->len)			/* Special case if "after" last */
		i->l = g_list_previous(i->l);

	return NULL != i->l ? i->l->data : NULL;
}

/**
 * Checks whether there is a previous item in the iterator.
 */
gboolean hash_list_has_previous(const hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);
	g_assert(i->hl->refcount > 0);
	g_assert(i->hl->stamp == i->stamp);

	return i->pos > 0 && i->hl->len;
}

/**
 * Move to next item in the direction of the iterator.
 */
gpointer hash_list_follower(hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);

	return i->move > 0 ? hash_list_next(i) : hash_list_previous(i);
}

/**
 * Checks whether there is a following item in the iterator, in the
 * direction chosen at creation time.
 */
gboolean hash_list_has_follower(const hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(NULL != i->hl);

	return i->move > 0 ? hash_list_has_next(i) : hash_list_has_previous(i);
}

/**
 * Release the iterator once we're done with it.
 */
void hash_list_release(hash_list_iter_t *i)
{
	g_assert(NULL != i);
	g_assert(i->hl->refcount > 0);
	i->hl->refcount--;

	wfree(i, sizeof(*i));
}

/**
 * Check whether hashlist contains the `data'.
 */
gboolean hash_list_contains(hash_list_t *hl, gpointer data)
{
	GList *l;
	g_assert(NULL != hl);
	g_assert(NULL != hl->ht);
	g_assert(NULL != data);
	g_assert(hl->refcount > 0);
	hash_list_regression(hl);

	l = g_hash_table_lookup(hl->ht, data);
	return NULL != l && l->data == data;
}

/**
 * Apply `func' to all the items in the structure.
 */
void hash_list_foreach(const hash_list_t *hl, GFunc func, gpointer user_data)
{
	g_assert(NULL != hl);
	g_assert(NULL != func);
	g_assert(hl->refcount > 0);
	hash_list_regression(hl);

	G_LIST_FOREACH(hl->l, func, user_data);

	hash_list_regression(hl);
}
