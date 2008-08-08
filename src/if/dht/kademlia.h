/*
 * $Id$
 *
 * Copyright (c) 2008, Raphael Manfredi
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

#ifndef _if_dht_kademlia_h_
#define _if_dht_kademlia_h_

#include "lib/endian.h"

#include "if/core/guid.h"

/*
 * Constants
 */

typedef enum kda_msg {
	KDA_MSG_PING_REQUEST			= 0x01,
	KDA_MSG_PING_RESPONSE			= 0x02,
	KDA_MSG_STORE_REQUEST			= 0x03,
	KDA_MSG_STORE_RESPONSE			= 0x04,
	KDA_MSG_FIND_NODE_REQUEST		= 0x05,
	KDA_MSG_FIND_NODE_RESPONSE		= 0x06,
	KDA_MSG_FIND_VALUE_REQUEST		= 0x07,
	KDA_MSG_FIND_VALUE_RESPONSE		= 0x08,
	KDA_MSG_STATS_REQUEST			= 0x09,	/**< DEPRECATED, UNSUPPORTED */
	KDA_MSG_STATS_RESPONSE			= 0x0a,	/**< DEPRECATED, UNSUPPORTED */
} kda_msg_t;

#define KDA_MSG_MAX_ID				  0x0a

#define MAKE_VAL_CODE(a,b,c,d) ( \
	((guint32) (a) << 24) | \
	((guint32) (b) << 16) | \
	((guint32) (c) << 8)  | \
	((guint32) (d)))

enum kda_val {
	KDA_VAL_BINARY		= 0x0,
	KDA_VAL_LIME		= MAKE_VAL_CODE('L','I','M','E'),
	KDA_VAL_TEXT		= MAKE_VAL_CODE('T','E','X','T'),
	KDA_VAL_TEST		= MAKE_VAL_CODE('T','E','S','T'),
	KDA_VAL_ANY			= MAKE_VAL_CODE('*','*','*','*'),
};

#undef MAKE_VAL_CODE

/*
 * Our version number.
 */

#define KDA_VERSION_MAJOR	0
#define KDA_VERSION_MINOR	0

/*
 * Kademlia network constants.
 */

#define KDA_ALPHA		3		/**< degree of concurrency */
#define KDA_K			20		/**< size of k-buckets */
#define KDA_B			4		/**< extra depth for bucket splits */

/*
 * Header flags (on 8 bits).
 */

#define KDA_MSG_F_FIREWALLED	(1 << 0)	/* Remote host is firewalled */
#define KDA_MSG_F_SHUTDOWNING	(1 << 1)	/* Host is shutdowning */

/*
 * Structures.
 */

#define KDA_HEADER_SIZE			61
#define KDA_IPv4_CONTACT_SIZE	33

typedef guint8 kademlia_header_t[KDA_HEADER_SIZE];
typedef guint8 kademlia_ipv4_contact_t[KDA_IPv4_CONTACT_SIZE];

/***
 *** IPv4 Contact.
 ***/

/*
 * Bytes 0-3: Vendor code. (not specified but assuming big-endian)
 */

static inline guint32
kademlia_ipv4_contact_get_vendor(const void *contact)
{
	const guint8 *u8 = (void *) contact;
	return peek_be32(&u8[0]);
}

static inline void
kademlia_ipv4_contact_set_vendor(const void *contact, guint32 vendor)
{
	guint8 *u8 = (void *) contact;
	poke_be32(&u8[0], vendor);
}

/*
 * Byte 4: the major version of the contact
 */

static inline guint8
kademlia_ipv4_contact_get_major_version(const void *contact)
{
	guint8 *u8 = (void *) contact;
	return u8[4];
}

static inline void
kademlia_ipv4_contact_set_major_version(const void *contact, guint8 major)
{
	guint8 *u8 = (void *) contact;
	u8[4] = major;
}

/*
 * Byte 5: the minor version of the contact.
 */

static inline guint8
kademlia_ipv4_contact_get_minor_version(const void *contact)
{
	guint8 *u8 = (void *) contact;
	return u8[5];
}

static inline void
kademlia_ipv4_contact_set_minor_version(const void *contact, guint8 minor)
{
	guint8 *u8 = (void *) contact;
	u8[5] = minor;
}

/*
 * Byte 6-25: the Kademlia node ID of the contact.
 */

static inline const gchar *
kademlia_ipv4_contact_get_kuid(const void *contact)
{
	guint8 *u8 = (void *) contact;
	return (const gchar *) &u8[6];
}

static inline void
kademlia_ipv4_contact_set_kuid(void *contact, const guchar *kuid)
{
	guint8 *u8 = (void *) contact;
	memcpy(&u8[6], kuid, 20);
}

/*
 * Byte 26: length of the IP address, i.e. 4 here.
 * Byte 27-30: IP address (IPv4 format, assumed big-endian)
 * Byte 31-32: port number (assumed little-endian)
 */

static inline guint8
kademlia_ipv4_contact_get_addr_length(const void *contact)
{
	const guint8 *u8 = contact;
	return u8[26];
}

static inline guint32
kademlia_ipv4_contact_get_addr(const void *contact)
{
	const guint8 *u8 = contact;
	return peek_be32(&u8[27]);
}

static inline guint16
kademlia_ipv4_contact_get_port(const void *contact)
{
	const guint8 *u8 = contact;
	return peek_be16(&u8[31]);		/* Ports are big-endian in Kademlia */
}

static inline void
kademlia_ipv4_contact_set_addr_port(
	void *contact, guint32 addr, guint16 port)
{
	guint8 *u8 = contact;
	u8[26] = 4;
	poke_be32(&u8[27], addr);
	poke_be16(&u8[31], port);	/* Watch out: standard, unlike Gnutella... */
}

/***
 *** Message Header.
 ***/

/*
 * Bytes 0-15: Message ID (MUID)
 */

static inline const guid_t *
kademlia_header_get_muid(const kademlia_header_t *header)
{
	return (const guid_t *) header;
}

static inline void
kademlia_header_set_muid(kademlia_header_t *header, const guid_t *muid)
{
	memcpy(header, muid->v, 16);
}

/*
 * Byte 16: always set to 0x44 ('D' in ASCII)
 * Byte 17: version major of message
 * Byte 18: version minor of message
 */

static inline void
kademlia_header_set_dht(kademlia_header_t *header, guint8 major, guint8 minor)
{
	guint8 *u8 = (void *) header;
	u8[16] = 0x44;
	u8[17] = major;
	u8[18] = minor;
}

static inline guint8
kademlia_header_get_major_version(const void *header)
{
	const guint8 *u8 = header;
	return u8[17];
}

static inline guint8
kademlia_header_get_minor_version(const void *header)
{
	const guint8 *u8 = header;
	return u8[18];
}

/*
 * Bytes 19-22: length of the whole payload, viewed as a Gnutella message.
 * Stored in little endian.
 */

static inline guint32
kademlia_header_get_gnutella_size(const void *data)
{
	const guint8 *u8 = data;
	return peek_le32(&u8[19]);
}

static inline void
kademlia_header_set_gnutella_size(const void *header, guint32 size)
{
	guint8 *u8 = (void *) header;
	poke_le32(&u8[19], size);
}

/*
 * Bytes 19-22 interpreted externally as the payload length of a Kademlia
 * message with its header of 61 bytes.
 */

static inline guint32
kademlia_header_get_size(const void *data)
{
	const guint8 *u8 = data;
	return peek_le32(&u8[19]) + 23 - KDA_HEADER_SIZE;
}

static inline void
kademlia_header_set_size(void *header, guint32 size)
{
	guint8 *u8 = (void *) header;
	poke_le32(&u8[19], size + KDA_HEADER_SIZE - 23);
}

/*
 * Byte 23: the Kademlia message function.
 */

static inline guint8
kademlia_header_get_function(const void *header)
{
	const guint8 *u8 = header;
	return u8[23];
}

static inline void
kademlia_header_set_function(kademlia_header_t *header, guint8 function)
{
	guint8 *u8 = (void *) header;
	u8[23] = function;
}

/*
 * Bytes 24-56: Contact, the node that created the message, with its address
 * architected to an IPv4 one.
 */

static inline guint32
kademlia_header_get_contact_vendor(const void *header)
{
	const guint8 *u8 = header;
	return kademlia_ipv4_contact_get_vendor(&u8[24]);
}

static inline void
kademlia_header_set_contact_vendor(kademlia_header_t *header, guint32 vendor)
{
	guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_vendor(&u8[24], vendor);
}

static inline guint8
kademlia_header_get_contact_major_version(const kademlia_header_t *header)
{
	const guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_major_version(&u8[24]);
}

static inline guint8
kademlia_header_get_contact_minor_version(const kademlia_header_t *header)
{
	const guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_minor_version(&u8[24]);
}

static inline void
kademlia_header_set_contact_version(kademlia_header_t *header,
	guint8 major, guint8 minor)
{
	const guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_major_version(&u8[24], major);
	kademlia_ipv4_contact_set_minor_version(&u8[24], minor);
}

static inline void
kademlia_header_set_contact_major_version(void *header, guint8 major)
{
	const guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_major_version(&u8[24], major);
}

static inline guint8
kademlia_header_get_contact_version(const kademlia_header_t *header)
{
	const guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_minor_version(&u8[24]);
}

static inline void
kademlia_header_set_contact_minor_version(void *header, guint8 minor)
{
	const guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_minor_version(&u8[24], minor);
}

static inline const gchar *
kademlia_header_get_contact_kuid(const void *header)
{
	guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_kuid(&u8[24]);
}

static inline void
kademlia_header_set_contact_kuid(void *header, const guchar *kuid)
{
	guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_kuid(&u8[24], kuid);
}

static inline void *
kademlia_header_contact_kuid_pointer(void *header)
{
	guint8 *u8 = (void *) header;
	return &u8[24];
}

static inline guint32
kademlia_header_get_contact_addr(const void *header)
{
	guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_addr(&u8[24]);
}

static inline guint16
kademlia_header_get_contact_port(const void *header)
{
	guint8 *u8 = (void *) header;
	return kademlia_ipv4_contact_get_port(&u8[24]);
}

static inline void
kademlia_header_set_contact_addr_port(
	const void *header, guint32 addr, guint16 port)
{
	guint8 *u8 = (void *) header;
	kademlia_ipv4_contact_set_addr_port(&u8[24], addr, port);
}

/*
 * Byte 57: contact's instance ID. (what is that?)
 */

static inline guint8
kademlia_header_get_contact_instance(const kademlia_header_t *header)
{
	guint8 *u8 = (void *) header;
	return u8[57];
}

static inline void
kademlia_header_set_contact_instance(kademlia_header_t *header, guint8 instance)
{
	guint8 *u8 = (void *) header;
	u8[57] = instance;
}

/*
 * Byte 58: various flags about the contact (the sender of the message).
 */

static inline guint8
kademlia_header_get_contact_flags(const kademlia_header_t *header)
{
	guint8 *u8 = (void *) header;
	return u8[58];
}

static inline void
kademlia_header_set_contact_flags(kademlia_header_t *header, guint8 flags)
{
	guint8 *u8 = (void *) header;
	u8[58] = flags;
}

/*
 * Bytes 59-60: length of option extended header (assuming big-endian)
 */

static inline guint16
kademlia_header_get_extended_length(const void *header)
{
	const guint8 *u8 = header;
	return peek_be16(&u8[59]);
}

static inline void
kademlia_header_set_extended_length(kademlia_header_t *header, guint16 len)
{
	guint8 *u8 = (void *) header;
	poke_be16(&u8[59], len);
}

/*
 * Basic check of constants in a valid Kademlia header:
 * Byte 16 is always 0x44.
 * The length of the IP address of the contact is always 4.
 */

static inline gboolean
kademlia_header_constants_ok(void *header)
{
	const guint8 *u8 = header;
	return
		0x44 == u8[16] && 
		4 == kademlia_ipv4_contact_get_addr_length(&u8[24]);
}

/**
 * End of Kademlia regular header (first byte after header).
 */
static inline const void *
kademlia_header_end(const kademlia_header_t *header)
{
	const guint8 *u8 = (void *) header;
	return &u8[KDA_HEADER_SIZE];
}

#endif /* _if_dht_kademlia_h_ */

/* vi: set ts=4 sw=4 cindent: */
