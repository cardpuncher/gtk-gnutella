/*
 * $Id$
 *
 * Copyright (c) 2001-2003, Raphael Manfredi
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
 * Gnutella Network Messages routing.
 */

#include "common.h"

RCSID("$Id$");

#include "search.h"		/* For search_passive. */
#include "routing.h"
#include "hosts.h"
#include "gmsg.h"
#include "nodes.h"
#include "gnet_stats.h"
#include "hostiles.h"
#include "guid.h"
#include "oob_proxy.h"

#include "if/gnet_property.h"
#include "if/gnet_property_priv.h"

#include "lib/atoms.h"
#include "lib/endian.h"
#include "lib/glib-missing.h"
#include "lib/walloc.h"
#include "lib/override.h"	/* Must be the last header included */

static struct gnutella_node *fake_node;		/* Our fake node */

/*
 * An entry in the routing table.
 *
 * Each entry is stored in the "message_array[]", to keep track of the
 * order used to create the routes, and in a hash table for quick lookup,
 * hashing being made based on the muid and the function.
 *
 * Query hit routes and push routes are precious, therefore they are
 * moved to the tail of the "message_array[]" when they get used to increase
 * their liftime.
 */
struct message {
	gchar muid[16];				/* Message UID */
	struct message **slot;		/* Place where we're referenced from */
	GSList *routes;	            /* route_data from where the message came */
	guint8 function;			/* Type of the message */
	guint8 ttl;					/* Max TTL we saw for this message */
};

/*
 * We don't store a list of nodes in the message structure, but a list of
 * route_data: the reason is that nodes can go away, but we don't want to
 * traverse the whole routing table to reclaim all the places where they
 * were referenced.
 *
 * The route_data structure points to a node and keeps track of the amount of
 * messages that it is used to track.  When a node disappears, the `node' field
 * in the associated route_data structure is set to NULL.  Dangling references
 * are removed only when needed.
 */
struct route_data {
	struct gnutella_node *node;
	gint32 saved_messages; 		/* # msg from this host in routing table */
};

static struct route_data fake_route;		/* Our fake route_data */

static const gchar *debug_msg[256];

/*
 * We're using the message table to store Query hit routes for Push requests,
 * but this is a temporary solution.  As we continuously refresh those
 * routes, we must make sure they stay alive for some time after having been
 * updated.  Given that we periodically supersede the message_array[] in a
 * round-robin fashion, it is not really appropriate.
 *		--RAM, 06/01/2002
 */
#define QUERY_HIT_ROUTE_SAVE	0	/* Function used to store QHit GUIDs */

/*
 * Routing table data structures.
 *
 * This is known as the message_array[].  It used to be a fixed-sized
 * array, but it is no more.  Instead, we use an array of chunks that
 * are dynamically allocated as needed.  The aim is to not cycle
 * back to the beginning of the table, loosing all the routing information
 * before at least TABLE_MIN_CYCLE seconds have elapsed or we have
 * allocated more than the amount of chunks we can tolerate.
 *
 * Each chunk contains pointers to dynamically allocated message entries,
 * each pointer being called a "slot" whilst the message structure is called
 * the "entry".
 */

#define CHUNK_BITS			14 		/* log2 of # messages stored  in a chunk */
#define MAX_CHUNKS			32		/* Max # of chunks */
#define TABLE_MIN_CYCLE		1800	/* 30 minutes at least */

#define CHUNK_MESSAGES		(1 << CHUNK_BITS)
#define CHUNK_INDEX(x)		(((x) & ~(CHUNK_MESSAGES - 1)) >> CHUNK_BITS)
#define ENTRY_INDEX(x)		((x) & (CHUNK_MESSAGES - 1))

static struct {
	struct message **chunks[MAX_CHUNKS];
	gint next_idx;					/* Next slot to use in "message_array[]" */
	gint capacity;					/* Capacity in terms of messages */
	gint count;						/* Amount really stored */
	GHashTable *messages_hashed;	/* All messages (key = struct message) */
	time_t last_rotation;			/* Last time we restarted from idx=0 */
} routing;

static gboolean find_message(
	const gchar *muid, guint8 function, struct message **m);

/*
 * "banned" GUIDs for push routing.
 *
 * The following GUIDs are so common that it does not make sense to
 * route pushes to them (i.e. they are are NOT unique on the network!).
 */
static const gchar * const banned_push[] = {
	"20d262ff0e6fd6119734004005a207b1",		/* Morpheus, 29/06/2002 */
};
GHashTable *ht_banned_push = NULL;

#define BANNED_PUSH_COUNT	G_N_ELEMENTS(banned_push)

/*
 * Push-proxy table.
 *
 * It maps a GUID to a node, so that we can easily send a push message
 * on behalf of a requesting node to the proper connection.
 */
GHashTable *ht_proxyfied = NULL;

/*
 * Routing logging.
 */
struct route_log {
	guint32 ip;					/* Sender's IP */
	guint16 port;				/* Sender's port */
	gchar muid[16];				/* Message ID */
	guint8 function;			/* Message function */
	guint8 hops;				/* Message hops */
	guint8 ttl;					/* Message ttl */
	gboolean handle;			/* Whether message will be handled */
	gboolean local;				/* Whether message originated locally */
	gboolean new;				/* Whether message is a new message */
	gchar extra[80];			/* Extra text for logging */
	struct route_dest dest;		/* Message destination */
};

extern gint guid_eq(gconstpointer a, gconstpointer b);

static void free_route_list(struct message *m);

/**
 * Record message parameters.
 */
static void
routing_log_init(struct route_log *log,
	struct gnutella_node *n,
	const gchar *muid, guint8 function, guint8 hops, guint8 ttl)
{
	if (routing_debug <= 7)
		return;

	if (n == NULL) {
		log->local = TRUE;
		log->ip = 0;
		log->port = 0;
	} else {
		log->local = FALSE;
		log->ip = n->ip;
		log->port = n->port;
	}

	log->function = function;
	log->hops = hops;
	log->ttl = ttl;
	memcpy(log->muid, muid, 16);

	log->extra[0] = '\0';
	log->handle = FALSE;
	log->new = FALSE;
	log->dest.type = ROUTE_NONE;
}

/**
 * Record message's route.
 */
static void
routing_log_set_route(struct route_log *log,
	struct route_dest *dest, gboolean handle)
{
	if (routing_debug <= 7)
		return;

	log->dest = *dest;		/* Struct copy */
	log->handle = handle;
}

/**
 * Mark message as being new.
 */
static void
routing_log_set_new(struct route_log *log)
{
	if (routing_debug <= 7)
		return;

	log->new = TRUE;
}

/**
 * Record extra logging information, appending to existing information.
 */
static void
routing_log_extra(struct route_log *log, const gchar *fmt, ...)
{
	va_list args;
	gchar *buf;
	gint buflen;
	gint len;

	if (routing_debug <= 7)
		return;

	buf = log->extra;
	buflen = sizeof(log->extra);
	len = strlen(log->extra);

	/*
	 * If there was already a message recorded, append "; " before
	 * the new message.
	 */

	if (len) {
		buflen -= len;
		buf += len;

		if (buflen > 2) {
			gint seplen = gm_snprintf(buf, buflen, "; ");

			buflen -= seplen;
			buf += seplen;
		}
	}

	if (buflen <= 2)
		return;

	va_start(args, fmt);
	gm_vsnprintf(buf, buflen, fmt, args);
	va_end(args);
}

/**
 * Return string representation of message route, as pointer to static data.
 */
static gchar *
route_string(struct route_dest *dest, guint32 origin_ip)
{
	static gchar msg[80];

	switch (dest->type) {
	case ROUTE_NONE:
		gm_snprintf(msg, sizeof(msg), "stops here");
		break;
	case ROUTE_ONE:
		gm_snprintf(msg, sizeof(msg), "node %s", node_ip(dest->ur.u_node));
		break;
	case ROUTE_ALL_BUT_ONE:
		gm_snprintf(msg, sizeof(msg), "all but %s", ip_to_gchar(origin_ip));
		break;
	case ROUTE_MULTI:
		{
			gint count = g_slist_length(dest->ur.u_nodes);
			gm_snprintf(msg, sizeof(msg), "selected %u node%s",
				count, count == 1 ? "" : "s");
		}
		break;
	default:
		gm_snprintf(msg, sizeof(msg), "** BUG ** UNKNOWN ROUTE");
		break;
	}

	return msg;
}

/**
 * Emit log message.
 */
static void
routing_log_flush(struct route_log *log)
{
	if (routing_debug <= 7)
		return;

	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		"ROUTE %-21s %s %s %3d/%3d: [%c] %s%s%s-> %s",
		log->local ? "OURSELVES": ip_port_to_gchar(log->ip, log->port),
		debug_msg[log->function], guid_hex_str(log->muid),
		log->hops, log->ttl, log->handle ? 'H' : ' ',
		log->new ? "[NEW] " : "",
		log->extra, log->extra[0] == '\0' ? "" : " ",
		route_string(&log->dest, log->ip));
}

/**
 * Mangle OOB query MUID by zeroing the parts of the MUID where the IP:port
 * are recorded.
 *
 * @return copy of mangled MUID as pointer to static data.
 */
static const gchar *route_mangled_oob_muid(const gchar *muid)
{
	static gchar mangled[16];

	memcpy(mangled, muid, 16);

	mangled[0] = mangled[1] = mangled[2] = mangled[3] = 0;	/* Clear IP */
	mangled[13] = mangled[14] = 0;							/* Clear port */

	return mangled;
}

/**
 * Used to ensure type safety when accessing the routing_data field
 */
static struct route_data *
get_routing_data(struct gnutella_node *n)
{
	return (struct route_data *)(n->routing_data);
}

/**
 * If a node doesn't currently have routing data attached, this
 * creates and attaches some
 */
static void
init_routing_data(struct gnutella_node *node)
{
	struct route_data *route;

	g_assert(node->routing_data == NULL);
	
	/*
	 * Wow, this node hasn't sent any messages before.
	 * Allocate and link some routing data to it
	 */

	route = (struct route_data *) walloc(sizeof(struct route_data));

	route->node = node;
	route->saved_messages = 0;
	node->routing_data = route;
}

/**
 * Clean already allocated entry.
 */
static void
clean_entry(struct message *entry)
{
	g_hash_table_remove(routing.messages_hashed, entry);

	if (entry->routes != NULL)
		free_route_list(entry);

	entry->routes = NULL;
	entry->ttl = 0;
}

/**
 * Prepare entry, cleaning any old value we can find at the referenced slot.
 * We try to avoid re-allocating something if we can.
 *
 * @return message entry to use
 */
static struct message *
prepare_entry(struct message **entryp)
{
	struct message *entry = *entryp;

	if (entry == NULL) {
		*entryp = entry = walloc0(sizeof(*entry));
		entry->slot = entryp;
		routing.count++;
		goto done;
	}

	/*
	 * We cycled over the table, remove the message at the slot we're
	 * going to supersede.  We don't need to allocate anything, we'll
	 * reuse the old structure, which will be rehashed after being updated.
	 */

	clean_entry(entry);

done:
	g_assert(entry->slot == entryp);

	return entry;
}

/**
 * Fetch next routing table slot, a pointer to a routing entry.
 */
static struct message **
get_next_slot(void)
{
	guint idx;
	guint chunk_idx;
	struct message **chunk;
	struct message **slot = NULL;

	idx = routing.next_idx;
	chunk_idx = CHUNK_INDEX(idx);

	g_assert((gint) chunk_idx >= 0 && chunk_idx < MAX_CHUNKS);

	chunk = routing.chunks[chunk_idx];

	if (chunk == NULL) {
		time_t now = time((time_t) NULL);

		g_assert((gint) idx >= routing.capacity);

		/*
		 * Chunk does not exist yet, determine whether we should create
		 * it or recycle the table by going back to the start.
		 */

		if (
			idx > 0 &&
			delta_time(now, routing.last_rotation) > TABLE_MIN_CYCLE
		) {
			gint elapsed = now - routing.last_rotation;

			if (routing_debug)
				printf("RT cycling over table, elapsed=%d, holds %d / %d\n",
					elapsed, routing.count, routing.capacity);

			chunk_idx = 0;
			idx = routing.next_idx = 0;
			routing.last_rotation = now;
			slot = routing.chunks[0];
		} else {
			/*
			 * Allocate new chunk, extending the capacity of the table.
			 */

			g_assert(idx == 0 || chunk_idx > 0);

			routing.capacity += CHUNK_MESSAGES;
			routing.chunks[chunk_idx] = (struct message **)
				g_malloc0(CHUNK_MESSAGES * sizeof(struct message *));

			if (routing_debug)
				printf("RT created new chunk #%d, now holds %d / %d\n",
					chunk_idx, routing.count, routing.capacity);

			slot = routing.chunks[chunk_idx];	/* First slot in new chunk */
		}
	} else {
		slot = &chunk[ENTRY_INDEX(idx)];

		/*
		 * If we went back to the first index without allocating a chunk,
		 * it means we finally cycled over the table, in a forced way,
		 * because we have already allocated the maximum amount of chunks.
		 */

		if (idx == 0) {
			time_t now = time((time_t) NULL);
			gint elapsed = now - routing.last_rotation;

			if (routing_debug)
				printf("RT cycling over FORCED, elapsed=%d, holds %d / %d\n",
					elapsed, routing.count, routing.capacity);

			routing.last_rotation = now;
		}
	}

	g_assert(slot != NULL);
	g_assert(idx == (guint) routing.next_idx);
	g_assert((gint) idx >= 0 && idx < (guint) routing.capacity);

	/*
	 * It's OK to go beyond the last allocated chunk (a new chunk will
	 * be allocated next time) unless we already reached the last chunk.
	 */

	routing.next_idx++;

	if (CHUNK_INDEX(routing.next_idx) >= MAX_CHUNKS)
		routing.next_idx = 0;		/* Will force cycling over next time */

	return slot;
}

/**
 * Fetch next routing table entry to be able to store routing information.
 */
static struct message *
get_next_entry(void)
{
	struct message **slot;

	slot = get_next_slot();
	return prepare_entry(slot);
}

/**
 * When a precious route (for query hit or push) is used, revitalize the
 * entry by moving it to the end of the "message_array[]", thereby making
 * it unlikely that it expires soon.
 *
 * @return the new location of the revitalized entry
 */
void
revitalize_entry(struct message *entry, gboolean force)
{
	struct message **relocated;
	struct message *prev;

	/*
	 * Leaves don't route anything, so we usually don't revitalize its
	 * entries.  The only exception is when it makes use of the recorded
	 * PUSH routes, i.e. when it initiates a PUSH.
	 */

	if (!force && current_peermode == NODE_P_LEAF)
		return;

	/*
	 * Relocate at the end of the table, preventing early expiration.
	 */

	relocated = get_next_slot();

	if (entry->slot == relocated)			/* Same slot being used */
		return;

	/*
	 * Clean and reclaim new slot content, if present.
	 */

	prev = *relocated;

	if (prev != NULL) {
		clean_entry(prev);
		wfree(prev, sizeof(*prev));
		routing.count--;
	}

	/*
	 * Move `entry' to this new slot.
	 */

	*relocated = entry;
	*(entry->slot) = NULL;					/* Old slot "freed" */
	entry->slot = relocated;				/* Entry now at new slot */
}

/**
 * Did node send the message?
 */
static gboolean
node_sent_message(struct gnutella_node *n, struct message *m)
{
	GSList *l;
	struct route_data * route;

	if (n == fake_node)
		route = &fake_route;
	else
		route = get_routing_data(n);
	
	/*
	 * If we've never routed a message from this person before,
	 * it can't be a duplicate.
	 */

	if (route == NULL)
		return FALSE;
	
	for (l = m->routes; l; l = l->next) {
		if (route == ((struct route_data *) l->data))
			return TRUE;
	}

	return FALSE;
}

/**
 * compares two message structures
 */
static gint
message_compare_func(gconstpointer a, gconstpointer b)
{
	return
		0 == memcmp(((const struct message *) a)->muid,
			((const struct message *) b)->muid, 16)
		&& ((const struct message *) a)->function ==
			((const struct message *) b)->function;
}

/**
 * Hashes message structures for storage in a hash table
 */
static guint
message_hash_func(gconstpointer key)
{
	int count;
	guint hash = 0;
	
	for (count = 0; count <= 12; count += 4) {
		guint hashadd =
			( (const struct message *) key)->muid[count]            |
			(((const struct message *) key)->muid[count + 1] << 8)  |
			(((const struct message *) key)->muid[count + 2] << 16) |
			(((const struct message *) key)->muid[count + 3] << 24);

		hash ^= hashadd;
	}

	hash ^= (guint) ((const struct message *) key)->function;

	return hash;
}

/**
 * Init function
 */
void
routing_init(void)
{
    gchar guid_buf[16];
	guint32 i;
	gboolean need_guid = TRUE;
	extern guint guid_hash(gconstpointer key);		/* from atoms.c */

	/*
	 * Make sure it segfaults if we try to access it, but it must be
	 * distinct from NULL.
	 */
	fake_node = (struct gnutella_node *) 0x01;
	fake_route.saved_messages = 0;
	fake_route.node = fake_node;

	/*
	 * Initialize the banned GUID hash.
	 */

	ht_banned_push = g_hash_table_new(guid_hash, guid_eq);

	for (i = 0; i < BANNED_PUSH_COUNT; i++) {
		gchar g[16];
		const gchar *hex = banned_push[i];

		g_assert(strlen(hex) == 2 * sizeof(g));

		(void) hex_to_guid(hex, g);
		g_hash_table_insert(ht_banned_push, atom_guid_get(g), (gpointer) 1);
	}

	/*
	 * Only generate a new GUID for this servent if all entries are 0.
	 * The empty initialization happens in config_init(), but it can be
	 * overridden by the GUID read from the configuration file
	 * (persistent GUID).
	 *		--RAM, 08/03/2002
	 */

    gnet_prop_get_storage(PROP_GUID, (guint8 *) guid_buf, sizeof(guid_buf));

	for (i = 0; i < 15; i++) {		/* Not 16 since byte #15 is a marker */
		if (guid_buf[i]) {
			need_guid = FALSE;
			break;
		}
	}

	if (!guid_is_gtkg(guid_buf, NULL, NULL, NULL))
		need_guid = TRUE;

retry:
	if (need_guid)
		guid_ping_muid(guid_buf);		/* We want a "modern" GUID */

	/*
	 * If by extraordinary, we have generated a banned GUID, retry.
	 */

	if (g_hash_table_lookup(ht_banned_push, guid_buf)) {
		need_guid = TRUE;
		goto retry;
	}

    gnet_prop_set_storage(PROP_GUID, (guint8 *) guid_buf, sizeof(guid_buf));
	g_assert(guid_is_gtkg(guid_buf, NULL, NULL, NULL));

	/*
	 * Initialize message type array for routing logs.
	 */

	for (i = 0; i < 256; i++)
		debug_msg[i] = "UNKN ";

	debug_msg[GTA_MSG_INIT]           = "Ping ";
	debug_msg[GTA_MSG_INIT_RESPONSE]  = "Pong ";
	debug_msg[GTA_MSG_SEARCH]         = "Query";
	debug_msg[GTA_MSG_SEARCH_RESULTS] = "Q-Hit";
	debug_msg[GTA_MSG_PUSH_REQUEST]   = "Push ";
	debug_msg[GTA_MSG_VENDOR]         = "Vndor";
	debug_msg[GTA_MSG_STANDARD]       = "Vstd ";
	debug_msg[GTA_MSG_QRP]            = "QRP  ";
	debug_msg[GTA_MSG_HSEP_DATA]      = "HSEP ";

	/*
	 * Should be around for life of program, so should *never*
	 * need to be deallocated
	 */

	routing.messages_hashed =
		g_hash_table_new(message_hash_func, message_compare_func);
	routing.next_idx = 0;
	routing.capacity = 0;
	routing.last_rotation = time(NULL);

	/*
	 * Push proxification.
	 */

	ht_proxyfied = g_hash_table_new(guid_hash, guid_eq);
}

/**
 * Generate a new muid and put it in a message header.
 */
void
message_set_muid(struct gnutella_header *header, guint8 function)
{
	switch (function) {
	case GTA_MSG_PUSH_REQUEST:
	case GTA_MSG_BYE:
	case GTA_MSG_QRP:
	case GTA_MSG_HSEP_DATA:
	case GTA_MSG_STANDARD:
	case GTA_MSG_VENDOR:		/* When a non-blank random GUID is needed */
		guid_random_muid(header->muid);
		break;
	case GTA_MSG_INIT:
		guid_ping_muid(header->muid);
		break;
	default:
		g_error("unexpected message type %d", function);
	}
}

/**
 * The route references one less message.
 *
 * If the amount of messages referenced reaches 0 and the associated node
 * was removed, free the route structure.
 */
static void
remove_one_message_reference(struct route_data *rd)
{
	g_assert(rd);

	if (rd->node != fake_node) {
		g_assert(rd != &fake_route);
		g_assert(rd->saved_messages > 0);

		rd->saved_messages--;

		/*
		 * If we have no more messages from this node, and our
		 *  node has already died, wipe its routing data
		 */

		if (rd->node == NULL && rd->saved_messages == 0)
			wfree(rd, sizeof(*rd));
	} else
		g_assert(rd == &fake_route);
}

/**
 * Dispose of route list in message.
 */
static void
free_route_list(struct message *m)
{
	GSList *l;

	g_assert(m);
	
	for (l = m->routes; l; l = l->next)
		remove_one_message_reference((struct route_data *) l->data);

	g_slist_free(m->routes);
	m->routes = NULL;
}

/**
 * Erase a node from the routing tables
 */
void
routing_node_remove(struct gnutella_node *node)
{
	struct route_data *route = get_routing_data(node);

	g_assert(route);
	g_assert(route->node == node);

	route->node->routing_data = NULL;

	/*
	 * Make sure that any future references to this routing
	 * data know that we are not connected to a node.
	 */

	route->node = NULL;
	
	/*
	 * If no messages remain, we have no reason to keep the
	 * route_data around any more.
	 */

	if (route->saved_messages == 0)
		wfree(route, sizeof(*route));
}

/**
 * Adds a new message in the routing tables.
 *
 * @param muid is the message MUID
 * @param function is the message type
 * @param node is the node from which we got the message, NULL if we are the
 * node emitting it.
 */
void
message_add(const gchar *muid, guint8 function, struct gnutella_node *node)
{
	struct route_data *route;
	struct message *entry;
	struct message *m;
	gboolean found;

	found = find_message(muid, function, &m);

	if (!node) {
		struct route_log log;
		gboolean already_recorded = FALSE;

		routing_log_init(&log, NULL, muid, function, 0, my_ttl);

		if (found) {
			/*
			 * It is possible that we insert the message in the routing table,
			 * then it gets "garbage collected" through a cycling, and then
			 * we receive our own message back from the network, at which
			 * time it is re-inserted into the table.  Therefore, despite our
			 * re-issuing of our own (search) message, there might not
			 * actually be any entry for us.
			 *		--RAM, 21/02/2002
			 */

			if (node_sent_message(fake_node, m)) {
				routing_log_extra(&log, "already sent");
				already_recorded = TRUE;
			} else
				routing_log_extra(&log, "forgot we sent it");
		}

		route = &fake_route;
		node = fake_node;		/* We are the sender of the message */

		routing_log_flush(&log);

		if (already_recorded)
			return;
	} else {
		if (node->routing_data == NULL)
			init_routing_data(node);
		route = get_routing_data(node);
	}

	if (found)			/* Dup message forwarded due to higher TTL */
		entry = m;		/* Reuse existing entry */
	else {
		entry = get_next_entry();
		g_assert(entry->routes == NULL);

		/* fill in that storage space */
		memcpy(entry->muid, muid, 16);
		entry->function = function;
	}
	
	g_assert(route != NULL);

	/*
	 * We have to account for the reception of a duplicate message from
	 * the same node, but with a higher TTL. Hence the test for
	 * node_sent_message() if the message was already seen.
	 *		--RAM, 2004-08-28
	 */

	if (!found || !node_sent_message(node, m)) {
		route->saved_messages++;	
		entry->routes = g_slist_append(entry->routes, route);
	}

	if (found)
		return;

	/*
	 * New message entry.
	 */

	if (node != fake_node)
		entry->ttl = node->header.ttl;

	/* insert the new message into the hash table */
	g_hash_table_insert(routing.messages_hashed, entry, entry);
}

/**
 * Remove references to routing data that is no longer associated with
 * a node, within the route list of the message.
 */
static void
purge_dangling_references(struct message *m)
{
	GSList *l;

	for (l = m->routes; l; /* empty */) {
		struct route_data *rd = (struct route_data *) l->data;

		if (rd->node == NULL) {
			GSList *next = l->next;
			m->routes = g_slist_remove_link(m->routes, l);
			remove_one_message_reference(rd);
			g_slist_free_1(l);
			l = next;
		} else
			l = l->next;
	}
}

/**
 * Look for a particular message in the routing tables.
 *
 * If we find the message, returns true, otherwise false.
 *
 * If none of the nodes that sent us the message are still present, then
 * m->routes will be NULL.
 */
static gboolean
find_message(const gchar *muid, guint8 function, struct message **m)
{
	/* Returns TRUE if the message is found */
	/* Set *node to node if there is a connected node associated
	   with the message found */
	struct message dummyMessage;
	struct message * found_message;

	memcpy(dummyMessage.muid, muid, 16);
	dummyMessage.function = function;
	
	found_message = (struct message *)
		g_hash_table_lookup(routing.messages_hashed, &dummyMessage);

	if (!found_message) {
		*m = NULL;
		return FALSE;		/* We don't remember anything about this message */
	} else {
		/* wipe out dead references to old nodes */
		purge_dangling_references(found_message);

		*m = found_message;
		return TRUE;		/* Message was seen */
	}
}

/**
 * Ensure sane hops and TTL counts.
 *
 * @return TRUE if we can continue, FALSE if we should not forward the
 * message.
 */
static gboolean
check_hops_ttl(struct route_log *log, struct gnutella_node *sender)
{
	/*
	 * Can't forward a message with 255 hops: we can't increase the
	 * counter.  This should never happen, even for a routed message
	 * due to network constraints.
	 *		--RAM, 04/07/2002
	 */

	if (sender->header.hops == 255) {
		routing_log_extra(log, "max hop count reached");
		gnet_stats_count_dropped(sender, MSG_DROP_MAX_HOP_COUNT);
		sender->n_bad++;
		if (routing_debug)
			gmsg_log_bad(sender, "message with HOPS=255!");
		return FALSE;	/* Don't route, something is wrong */
	}

	if (sender->header.ttl == 0) {
		routing_log_extra(log, "TTL was 0");
		node_sent_ttl0(sender);
		return FALSE;	/* Don't route */
	}

	return TRUE;
}

/**
 * Forwards message to one node if `target' is non-NULL, or to all nodes but
 * the sender otherwise.  If we kick the node, then *node is set to NULL.
 * The message is not physically sent yet, but the `dest' structure is filled
 * with proper routing information.
 *
 * `routes' is normally NULL unless we're forwarding a PUSH request.  In that
 * case, it must be sent to the whole list of routes we have, and `target' will
 * be NULL.
 *
 * NB: we're just *recording* routing information for the message into `dest',
 * we are not physically forwarding the message on the wire.
 *
 * Returns whether we should handle the message after routing.
 */
static gboolean
forward_message(
	struct route_log *log,
	struct gnutella_node **node,
	struct gnutella_node *target, struct route_dest *dest, GSList *routes)
{
	struct gnutella_node *sender = *node;

	g_assert(routes == NULL || target == NULL);
	g_assert(current_peermode != NODE_P_LEAF);

	routing_log_set_new(log);

	/* Drop messages that would travel way too many nodes --RAM */
	if ((guint32) sender->header.ttl + sender->header.hops > hard_ttl_limit) {
		routing_log_extra(log, "hard TTL limit reached");

		/*
		 * When close neighboors of that node send messages we drop
		 * that way, they may try to flood the network.	Disconnect
		 * after too many offenses, which should have given the
		 * relaying node ample time to kick the offender out,
		 * according to our standards.
		 *				--RAM, 08/09/2001
		 */

		/* XXX max_high_ttl_radius & max_high_ttl_msg XXX */

		sender->n_hard_ttl++;
        gnet_stats_count_dropped(sender, MSG_DROP_HARD_TTL_LIMIT);

		if (
			sender->header.hops <= max_high_ttl_radius &&
			sender->n_hard_ttl > max_high_ttl_msg &&
			!NODE_IS_UDP(sender)
		) {
			node_bye(sender, 403, "Relayed %d high TTL (>%d) messages",
				sender->n_hard_ttl, max_high_ttl_msg);
			*node = NULL;
			return FALSE;
		}

		return TRUE;
	}

	if (!check_hops_ttl(log, sender))
		return TRUE;

	if (sender->header.ttl == 1) {
		/* TTL expired, message stops here */
		routing_log_extra(log, "TTL expired");
		gnet_stats_count_expired(sender);
	} else {
		/*
		 * Forward message to all others nodes, or the the ones specified
		 * by the `routes' parameter if not NULL.
		 */

		if (routes != NULL) {
			GSList *l;
			GSList *nodes = NULL;
			gint count;

			g_assert(sender->header.function == GTA_MSG_PUSH_REQUEST);
			
			for (l = routes, count = 0; l; l = g_slist_next(l), count++) {
				struct route_data *rd = (struct route_data *) l->data;
				if (rd->node == sender)
					continue;
				nodes = g_slist_prepend(nodes, rd->node);
			}

			/*
			 * The `nodes' list will be freed by node_parse().
			 */

			dest->type = ROUTE_MULTI;
			dest->ur.u_nodes = nodes;

			if (count > 1)
				gnet_stats_count_general(GNR_BROADCASTED_PUSHES, 1);

		} else if (target != NULL) {
			dest->type = ROUTE_ONE;
			dest->ur.u_node = target;
		} else {
			/*
			 * This message is broadcasted, ensure its TTL is "reasonable".
			 * Trim down if excessively large.
			 * NB: Account for the fact that we haven't decremented it yet.
			 */

			if (sender->header.ttl > max_ttl + 1) {	/* TTL too large */
				sender->header.ttl = max_ttl + 1;	/* Trim down */
				routing_log_extra(log, "TTL trimmed down to %d ", max_ttl);
			}

			dest->type = ROUTE_ALL_BUT_ONE;
			dest->ur.u_node = sender;
		}
	}

	return TRUE;
}

/**
 * Lookup message in the routing table and check whether we have a duplicate.
 *
 * @param log is a structure recording to-be-logged information for debug
 * @param node is a pointer to the variable holding the node, and it can be
 * set to NULL if we removed the node.
 * @param mangled is an alternate MUID with bytes 0-3 and 13-14 zeroed
 * which should be tested for duplication as well.  Only set for OOB queries.
 * @param mp is set on output to the message if found in the routing table.
 *
 * @return whether we should route the message.  If `*mp' is not NULL, then
 * the message was a duplicate and it should not be handled locally.
 */
static gboolean
check_duplicate(struct route_log *log,
	struct gnutella_node **node, const gchar *mangled, struct message **mp)
{
	struct gnutella_node *sender = *node;

	if (find_message(sender->header.muid, sender->header.function, mp)) {
		struct message *m = *mp;

		g_assert(m != NULL);		/* find_message() succeeded */

		/*
		 * This is a duplicated message, which we might drop.
		 *
		 * We don't drop queries/pushes that come to us with a higher TTL
		 * as we have previously seen.  In that case, we forward them but
		 * don't handle them, since this was done when we saw them the
		 * very first time.
		 *		--RAM, 2004-08-28
		 */

		if (sender->header.ttl > m->ttl) {
			routing_log_extra(log, "dup message with higher ttl");

			gnet_stats_count_general(GNR_DUPS_WITH_HIGHER_TTL, 1);

			m->ttl = sender->header.ttl;	/* Remember highest TTL */

			return TRUE;			/* Forward but don't handle */
		}

		gnet_stats_count_dropped(sender, MSG_DROP_DUPLICATE);

		if (m->routes && node_sent_message(sender, m)) {
			/* The same node has sent us a message twice ! */
			routing_log_extra(log, "dup message (from the same node!)");

			/*
			 * That is a really good reason to kick the offender
			 * But do so only if killing this node would not bring
			 * us too low in node count, and if they have sent enough
			 * dups to be sure it's not bad luck in MUID generation.
			 * Finally, check the ratio of dups on received messages,
			 * because a dup once in a while is nothing.
			 *				--RAM, 08/09/2001
			 */

			/* XXX max_dup_msg & max_dup_ratio XXX ***/

			if (++(sender->n_dups) > min_dup_msg &&
				!NODE_IS_UDP(sender) &&
				connected_nodes() > MAX(2, up_connections) &&
				sender->n_dups > (guint16) 
					(((float)min_dup_ratio) / 10000.0 * sender->received)
			) {
				node_mark_bad_vendor(sender);
				node_bye(sender, 401, "Sent %d dups (%.1f%% of RX)",
					sender->n_dups, sender->received ?
						100.0 * sender->n_dups / sender->received :
						0.0);
				*node = NULL;
			} else {
				if (routing_debug > 2)
					gmsg_log_bad(sender, "dup message ID %s from same node",
						guid_hex_str(sender->header.muid));
			}
		} else {
			if (m->routes == NULL)
				routing_log_extra(log, "dup message, original route lost");
			else
				routing_log_extra(log, "dup message");
		}

		return FALSE;			/* Don't forward and don't handle */
	}

	/*
	 * If we have a mangled MUID to test against, we have to look at whether
	 * we also have an entry for it in the routing table.  Indeed, due to
	 * OOB-proxying of queries performed by ultra nodes, the same query can
	 * be proxied from different nodes with different IP:port.
	 *
	 * The mangled MUID is the query MUID where the IP:port have been zeroed
	 * out.  If we get a match, it means we saw this OOB query under a
	 * different incarnation already.  We'll only forward it if it has a
	 * higher TTL as we saw before.
	 *
	 *		--RAM, 2004-09-19
	 *
	 * The code in the following if() almost mirrors the one in the preceding
	 * if() with two exceptions: we don't kick offenders, and we increment
	 * the global stats counter of duplicate OOB queries.
	 *
	 * XXX factorize this code.
	 */

	if (mangled && find_message(mangled, sender->header.function, mp)) {
		struct message *m = *mp;

		g_assert(m != NULL);		/* find_message() succeeded */

		gnet_stats_count_general(GNR_QUERY_OOB_PROXIED_DUPS, 1);

		if (sender->header.ttl > m->ttl) {
			routing_log_extra(log, "dup OOB query with higher ttl");

			gnet_stats_count_general(GNR_DUPS_WITH_HIGHER_TTL, 1);

			m->ttl = sender->header.ttl;	/* Remember highest TTL */

			return TRUE;			/* Forward but don't handle */
		}

		gnet_stats_count_dropped(sender, MSG_DROP_DUPLICATE);

		if (m->routes && node_sent_message(sender, m)) {
			/* The same node has sent us a message twice ! */
			routing_log_extra(log, "dup OOB query (from the same node!)");
		} else {
			if (m->routes == NULL)
				routing_log_extra(log, "dup OOB query, original route lost");
			else
				routing_log_extra(log, "dup OOB query");
		}

		return FALSE;			/* Don't forward and don't handle */
	}

	g_assert(*mp == NULL);

	return TRUE;				/* Forward and handle (new message) */
}

/**
 * Route a push message.
 *
 * @return whether message should be handled
 */
static gboolean
route_push(struct route_log *log,
	struct gnutella_node **node, struct route_dest *dest)
{
	struct gnutella_node *sender = *node;
	struct message *m;
	guint32 ip;

	/*
	 * A Push request is not broadcasted as other requests, it is routed
	 * back along the nodes that have seen Query Hits from the target
	 * servent of the Push.
	 */

	g_assert(sender->size > 16);	/* Must be a valid push */

	/*
	 * Is it for us?
	 */

	if (0 == memcmp(guid, sender->data, 16)) {
		routing_log_extra(log, "we are the target");
		return TRUE;
	}

	if (current_peermode == NODE_P_LEAF)
		return FALSE;				/* Not for us, and we can't relay */

	/*
	 * If the GUID is banned, drop it immediately.
	 */

	if (g_hash_table_lookup(ht_banned_push, sender->data)) {
		if (routing_debug > 3)
			gmsg_log_dropped(&sender->header,
				"from %s, banned GUID %s",
				node_ip(sender), guid_hex_str(sender->data));

		gnet_stats_count_dropped(sender, MSG_DROP_BANNED);

		return FALSE;
	}

	/*
	 * If IP address is among the hostile set, drop.
	 */

	READ_GUINT32_BE(sender->data + 20, ip);

	if (hostiles_check(ip)) {
		gnet_stats_count_dropped(sender, MSG_DROP_HOSTILE_IP);
		return FALSE;
	}

	/*
	 * The GUID of the target are the leading bytes of the Push
	 * message, hence we pass `sender->data' to find_message().
	 */

	if (
		!find_message(sender->data, QUERY_HIT_ROUTE_SAVE, &m) ||
		m->routes == NULL
	) {
		if (m && m->routes == NULL) {
			routing_log_extra(log, "route to target GUID %s gone",
				guid_hex_str(sender->data));
			gnet_stats_count_dropped(sender, MSG_DROP_ROUTE_LOST);
		} else {
			routing_log_extra(log, "no route to target GUID %s",
				guid_hex_str(sender->data));
			gnet_stats_count_dropped(sender, MSG_DROP_NO_ROUTE);
		}

		return FALSE;
	}

	/*
	 * By revitalizing the entry, we'll remember the route for
	 * at least TABLE_MIN_CYCLE secs more after seeing this PUSH.
	 */

	revitalize_entry(m, FALSE);
	forward_message(log, node, NULL, dest, m->routes);

	return FALSE;		/* We are not the target, don't handle it */
}

/**
 * Route a query message.
 *
 * @return whether message should be handled
 */
static gboolean
route_query(struct route_log *log,
	struct gnutella_node **node, struct route_dest *dest)
{
	struct gnutella_node *sender = *node;
	gboolean is_oob_query;

	if (current_peermode == NODE_P_LEAF)
		return TRUE;

	/*
	 * If the message comes from UDP, it's not going to go anywhere.
	 */

	if (NODE_IS_UDP(sender))
		return TRUE;			/* Process it, but don't route */

	is_oob_query = gmsg_split_is_oob_query(&sender->header, sender->data);

	/*
	 * If node is shutdown, it won't be there to get query hits back.
	 * This check is useless for OOB queries since hits may flow out-of-band.
	 */

	if (!NODE_IS_READABLE(sender) && !is_oob_query) {
		gnet_stats_count_dropped(sender, MSG_DROP_SHUTDOWN);
		return FALSE;
	}

	/*
	 * If the node is flow-controlled on TX, then it is preferable
	 * to drop queries immediately: the traffic the replies may
	 * generate could pile up and make the queue reach its maximum
	 * size.  It is hoped that the flow control condition will not
	 * last too long.
	 *
	 * We do that here, at the lowest level, because we do not
	 * want to record the query as seen: if it comes from another
	 * route, we'll handle it.
	 *
	 *		--RAM, 02/02/2002
	 *
	 * Let OOB queries pass through naturally, as most likely the replies
	 * generated will flow back out-of-band.
	 *
	 *		--RAM, 2004-08-29
	 */

	if (!is_oob_query && NODE_IN_TX_FLOW_CONTROL(sender)) {
		gnet_stats_count_dropped(sender, MSG_DROP_FLOW_CONTROL);
		return FALSE;
	}

	return forward_message(log, node, NULL, dest, NULL);	/* Broadcast */
}

/**
 * Route a query hit message.
 *
 * @return whether message should be handled
 */
static gboolean
route_query_hit(struct route_log *log,
	struct gnutella_node **node, struct route_dest *dest)
{
	GSList *l;
	struct gnutella_node *sender = *node;
	struct message *m;
	gboolean node_is_target = FALSE;
	struct gnutella_node *found;
	gboolean is_oob_proxied;

	/*
	 * We have to record we have seen a hit reply from the GUID held at
	 * the tail of the packet.  This information is used to later route
	 * back Push messages.
	 *		--RAM, 06/01/2002
	 */

	if (!NODE_IS_UDP(sender)) {
		gchar *guid = sender->data + sender->size - 16;
		g_assert(sender->size >= 16);

		if (!find_message(guid, QUERY_HIT_ROUTE_SAVE, &m)) {
			/*
			 * We've never seen any Query Hit from that servent.
			 * Ensure it's not a banned GUID though.
			 */

			if (!g_hash_table_lookup(ht_banned_push, guid))
				message_add(guid, QUERY_HIT_ROUTE_SAVE, sender);
		} else if (m->routes == NULL || !node_sent_message(sender, m)) {
			struct route_data *route;

			/*
			 * Either we have no more nodes that sent us any query hit
			 * from that GUID, or we have never received any such hit
			 * from the sender.
			 */

			route = get_routing_data(sender);
			
			g_assert(route != NULL);
			
			m->routes = g_slist_append(m->routes, route);
			route->saved_messages++;

			/*
			 * We just made use of this routing data: make it persist
			 * as long as we can by revitalizing the entry.  This will
			 * allow us to route back PUSH requests for a longer time,
			 * at least TABLE_MIN_CYCLE seconds after seeing the latest
			 * query hit flow by.
			 */

			revitalize_entry(m, FALSE);
		}
	}

	/*
	 * It's important to handle query hits for OOB-proxied queries
	 * differently: they appear to come from ourselves, but they are
	 * not destined to us, and we'll forward them to the leaf who sent
	 * the initial query, regardless of the hops/TTL value of the hit.
	 */
	
	is_oob_proxied = oob_proxy_muid_proxied(sender->header.muid);

	if (!is_oob_proxied && !check_hops_ttl(log, sender))
		goto handle;				/* We will handle the hit nonetheless */

	if (!find_message(sender->header.muid, GTA_MSG_SEARCH, &m)) {
		/* We have never seen any request matching this reply ! */

		routing_log_extra(log, "no request matching the reply!");

		gnet_stats_count_dropped(sender, MSG_DROP_NO_ROUTE);
		sender->n_bad++;	/* Node shouldn't have forwarded this message */

		if (routing_debug)
			gmsg_log_bad(sender, "got reply without matching request %s%s",
				guid_hex_str(sender->header.muid),
				is_oob_proxied ? " (OOB-proxied)" : "");

		goto handle;
	}

	g_assert(m);		/* Or find_message() would have returned FALSE */
	
	/*
	 * Since this routing data is used, relocate it at the end of
	 * the "message_array[]" to augment its lifetime.
	 */

	revitalize_entry(m, FALSE);

	/*
	 * If `m->routes' is NULL, we have seen the request, but unfortunately
	 * none of the nodes that sent us the request is connected any more.
	 */

	if (m->routes == NULL)
		goto route_lost;

	if (node_sent_message(fake_node, m)) {
		node_is_target = TRUE;		/* We are the target of the reply */
		if (is_oob_proxied)
			gnet_stats_count_general(GNR_OOB_PROXIED_QUERY_HITS, 1);
		else
			gnet_stats_count_general(GNR_LOCAL_QUERY_HITS, 1);
		goto handle;
	}

	g_assert(!is_oob_proxied);		/* Or we would have sent the message */

	/*
	 * Look for a route different from the one we received the
	 * message from.
	 * XXX should remember hops from queries and choose the lowest hop
	 * XXX route for relaying. --RAM, 2004-08-29
	 */

	for (found = NULL, l = m->routes; l; l = g_slist_next(l)) {
		struct route_data *route = (struct route_data *) l->data;
		if (route->node != sender) {
			found = route->node;
			break;
		}
	}

	if (found == NULL)
		goto route_lost;

	/*
	 * If the TTL expired, drop the message, unless the target is a
	 * leaf node, in which case we'll forward it the reply, or we
	 * are a leaf node, in which case we won't route the message!.
	 */

	if (sender->header.ttl == 1) {
		if (NODE_IS_LEAF(found)) {
			/* TTL expired, but target is a leaf node */
			routing_log_extra(log, "expired TTL bumped");
			sender->header.ttl++;
		} else {
			/* TTL expired, message stops here in any case */
			if (current_peermode != NODE_P_LEAF) {
				routing_log_extra(log, "TTL expired");
				gnet_stats_count_expired(sender);
			}
			goto handle;
		}
	}

	dest->type = ROUTE_ONE;
	dest->ur.u_node = found;
	
	goto handle;

route_lost:
	routing_log_extra(log, "route to target lost");

	gnet_stats_count_dropped(sender, MSG_DROP_ROUTE_LOST);
	goto final;

handle:
	if (node_is_target)
		routing_log_extra(log, "we are the target%s",
			is_oob_proxied ? " (OOB-proxy)" : "");

	/* FALL THROUGH */
final:
	/*
	 * We apply the TTL limits differently for replies.
	 *
	 * Indeed, replies are forwarded to ONE node, and are not
	 * broadcasted.	It is therefore important to make sure the
	 * reply will reach the issuing host.
	 *
	 * So we don't compare the header's TLL to `max_ttl' but to
	 * `hard_ttl_limit', and if above the limit, we don't drop
	 * the message but trim the TTL down to something acceptable.
	 *
	 *				--RAM, 15/09/2001
	 */

	if (sender->header.ttl > hard_ttl_limit + 1) {	/* TTL too large */
		routing_log_extra(log, "TTL adjusted");
		sender->header.ttl = hard_ttl_limit + 1;
	}

	return TRUE;
}


/**
 * Main route computation function.
 *
 * Source of message is passed by reference as `node', because it can be
 * nullified when the node is disconnected from.
 *
 * The destination of the message is computed in `dest', but the message is
 * not physically sent.  The gmsg_sendto_route() will have to be called
 * for that.
 *
 * Returns whether the message is to be handled locally.
 */
gboolean
route_message(struct gnutella_node **node, struct route_dest *dest)
{
	gboolean handle_it = FALSE;
	struct gnutella_node *sender = *node;
	struct message *m;
	gboolean duplicate = FALSE;
	gboolean route_it = FALSE;
	struct route_log log;
	const gchar *mangled = NULL;

	/* Ensure we never get something bearing our special GUID route marker */
	g_assert(sender->header.function != QUERY_HIT_ROUTE_SAVE);

	dest->type = ROUTE_NONE;

	/* If we haven't allocated routing data for this node yet, do so */
	if (sender->routing_data == NULL)
		init_routing_data(sender);

	routing_log_init(&log, sender, sender->header.muid,
		sender->header.function, sender->header.hops, sender->header.ttl);

	/*
	 * For OOB queries, we have to mangle the MUID to detect duplicates
	 * when the query is proxied from different ultra nodes.
	 */

	if (
		sender->header.function == GTA_MSG_SEARCH &&
		gmsg_split_is_oob_query(&sender->header, sender->data)
	) {
		mangled = route_mangled_oob_muid(sender->header.muid);
		gnet_stats_count_general(GNR_OOB_QUERIES, 1);
	}

	/*
	 * For routed messages, we check whether we get a duplicate and
	 * whether the message should be handled locally.
	 */

	switch (sender->header.function) {
	case GTA_MSG_PUSH_REQUEST:
	case GTA_MSG_SEARCH:
		route_it = check_duplicate(&log, node, mangled, &m);
		duplicate = (m != NULL);

		/*
		 * Record the message in the routing table.
		 *
		 * If it's a duplicate, we'll record the additional route, which
		 * will enable us to find alternate routes for query hits should
		 * the original one fail due to a node disconnection.
		 */

		message_add(sender->header.muid, sender->header.function, sender);

		/*
		 * Unfortunately, to be able to detect duplicate OOB-proxied queries
		 * we need to insert two entries in the routing table for queries
		 * marked with the OOB flag: one for the original MUID, and one for
		 * the mangled MUID with the IP:port section zeroed.
		 *		--RAM, 2004-09-19
		 */

		if (mangled)
			message_add(mangled, sender->header.function, sender);

		if (!route_it)
			goto done;
		break;
	default:
		break;
	}

	/*
	 * Compute the route, determine if we should handle the message.
	 */

	switch (sender->header.function) {
	case GTA_MSG_PUSH_REQUEST:
		handle_it = route_push(&log, node, dest);
		break;
	case GTA_MSG_SEARCH:
		handle_it = route_query(&log, node, dest);
		break;
	case GTA_MSG_SEARCH_RESULTS:
		handle_it = route_query_hit(&log, node, dest);
		break;
	default:
		g_error("messages 0x%x \"%s\" should not be routed",
			sender->header.function, debug_msg[sender->header.function]);
	}

	routing_log_set_route(&log, dest, handle_it);

done:
	routing_log_flush(&log);

	sender->header.hops++;				/* Mark passage through our node */
	if (sender->header.ttl)
		sender->header.ttl--;

	return !duplicate && handle_it;		/* Don't handle duplicates */
}

/**
 * Check whether we have a route for the reply that would be generated
 * for this request.
 *
 * Returns boolean indicating whether we have such a route.
 */
gboolean
route_exists_for_reply(gchar *muid, guint8 function)
{
	struct message *m;

	if (!find_message(muid, function & ~0x01, &m) || m->routes == NULL)
		return FALSE;

	return TRUE;
}

/**
 * Check whether we have a route to the given GUID, in order to send
 * pushes.
 *
 * Returns NULL if we have no such route, or a list of  node to which we should
 * send the packet otherwise.  It is up to the caller to free that list.
 */
GSList *
route_towards_guid(const gchar *guid)
{
	struct message *m;
	GSList *nodes = NULL;
	GSList *l;

	if (g_hash_table_lookup(ht_banned_push, guid))	/* Banned for PUSH */
		return NULL;

	if (!find_message(guid, QUERY_HIT_ROUTE_SAVE, &m) || m->routes == NULL)
		return NULL;

	revitalize_entry(m, TRUE);

	for (l = m->routes; l; l = g_slist_next(l)) {
		struct route_data *rd = (struct route_data *) l->data;
		nodes = g_slist_prepend(nodes, rd->node);
	}

	return nodes;
}

/**
 * Remove push-proxy entry indexed by GUID.
 */
void
route_proxy_remove(gchar *guid)
{
 	/*
	 * The GUID atom is still referred to by the node,
	 * so don't clear anything.
	 */

	g_hash_table_remove(ht_proxyfied, guid);
}

/**
 * Add push-proxy route to GUID `guid', which is node `n'.
 * Returns TRUE on success, FALSE if there is a GUID conflict.
 *
 * NB: assumes `guid' is already an atom linked somehow to `n'.
 */
gboolean
route_proxy_add(gchar *guid, struct gnutella_node *n)
{
	if (NULL != g_hash_table_lookup(ht_proxyfied, guid))
		return FALSE;

	g_hash_table_insert(ht_proxyfied, guid, n);
	return TRUE;
}

/**
 * Find node to which we are connected with supplied GUID and who requested
 * that we act as its push-proxy.
 *
 * Returns node address if we found it, or NULL if we aren't connected to
 * that node directly.
 */
struct gnutella_node *
route_proxy_find(gchar *guid)
{
	return (struct gnutella_node *) g_hash_table_lookup(ht_proxyfied, guid);
}

/**
 * Frees the banned GUID atom keys.
 */
static void
free_banned_push(gpointer key, gpointer unused_value, gpointer unused_udata)
{
	(void) unused_value;
	(void) unused_udata;
	atom_guid_free((gchar *) key);
}

/**
 * Destroy routing data structures.
 */
void
routing_close(void)
{
	guint cnt;

	g_assert(routing.messages_hashed);
	
	g_hash_table_destroy(routing.messages_hashed);
	routing.messages_hashed = NULL;

	for (cnt = 0; cnt < MAX_CHUNKS; cnt++) {
		struct message **chunk = routing.chunks[cnt];
		if (chunk != NULL) {
			gint i;
			for (i = 0; i < CHUNK_MESSAGES; i++) {
				struct message *m = chunk[i];
				if (m != NULL) {
					free_route_list(m);
					wfree(m, sizeof(*m));
				}
			}
			G_FREE_NULL(chunk);
		}
	}

	g_hash_table_foreach(ht_banned_push, free_banned_push, NULL);
	g_hash_table_destroy(ht_banned_push);
	ht_banned_push = NULL;

	cnt = g_hash_table_size(ht_proxyfied);
	if (cnt != 0)
		g_warning("push-proxification table still holds %u node%s",
			cnt, cnt == 1 ? "" : "s");

	g_hash_table_destroy(ht_proxyfied);
	ht_proxyfied = NULL;
}

/* vi: set ts=4: */
