/*
 * $Id$
 *
 * Copyright (c) 2004, Raphael Manfredi
 *
 * Proxied Out-of-band queries.
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

#ifndef _core_oob_proxy_h_
#define _core_oob_proxy_h_

#include <glib.h>

/*
 * Public interface.
 */

struct gnutella_node;

void oob_proxy_init(void);
void oob_proxy_close(void);

void oob_proxy_create(struct gnutella_node *n);
gboolean oob_proxy_pending_results(
	struct gnutella_node *n, gchar *muid, gint hits, gboolean udp_firewalled);
gboolean oob_proxy_got_results(struct gnutella_node *n, guint results);
gboolean oob_proxy_muid_proxied(gchar *muid);

#endif /* _core_oob_proxy_h_ */

/* vi: set ts=4: */
