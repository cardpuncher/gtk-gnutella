/*
 * $Id$
 *
 * Copyright (c) 2001-2002, Richard Eckart
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

#include <ctype.h> /* for isdigit() */
#include "gnet_stats_gui2.h"
#include "gnutella.h" /* for sizeof(struct gnutella_header) */

gchar *msg_type_str[MSG_TYPE_COUNT] = {
    "Unknown",
    "Ping",
    "Pong",
    "Bye",
    "QRP",
    "Vendor Spec.",
    "Vendor Std.",
    "Push",
    "Query",
    "Query Hit",
    "Total"
};

gchar *msg_drop_str[MSG_DROP_REASON_COUNT] = {
    "Bad size",
    "Too small",
    "Too large",
	"Way too large",
    "Unknown message type",
    "Message sent with TTL = 0",
    "Max TTL exceeded",
    "Ping throttle",
	"Unusable Pong",
    "Hard TTL limit reached",
    "Max hop count reached",
    "Unrequested reply",
    "Route lost",
    "No route",
    "Duplicate message",
    "Message to banned GUID",
    "Node shutting down",
    "Flow control",
    "Query text had no trailing NUL",
    "Query text too short",
    "Query had unnecessary overhead",
    "Malformed SHA1 Query",
    "Malformed UTF-8 Query",
    "Malformed Query Hit",
    "Query hit had bad SHA1"
};

gchar *general_type_str[GNR_TYPE_COUNT] = {
    "Routing errors",
    "Searches to local DB",
    "Hits on local DB",
    "Compacted queries",
    "Bytes saved by compacting",
    "UTF8 queries",
    "SHA1 queries"
};

gchar *msg_stats_label[] = {
	"Type",
	"Received",
	"Expired",
	"Dropped",
	"Relayed",
	"Generated"
};

#define FLOWC_MODE_HOPS(m)	((m) & 1)		/* columns represent hops */
#define FLOWC_MODE_TTL(m)	(!((m) & 1))	/*    "        "     TTL  */
#define FLOWC_MODE_BYTE(m)	(((m) & 2))		/* show volumes */
#define FLOWC_MODE_PKTS(m)	(!((m) & 2))	/* show number of packets */
#define FLOWC_MODE_REL(m)	(((m) & 4))		/* relative values */
#define FLOWC_MODE_ABS(m)	(!((m) & 4))	/* absolutes   " */
#define FLOWC_MODE_HDRS(m)	(((m) & 8))		/* add header size */
#define FLOWC_MODE_PAYL(m)	(!((m) & 8))	/* show payload only */

#define STATS_MSGS_MODE_PACKETS 1
#define STATS_MSGS_MODE_REL 2


static gint selected_type = MSG_TOTAL;
static gint selected_flowc = 0;
static gint gnet_stats_msgs_mode = 0;

static void column_set_hidden(
	GtkTreeView *treeview, const gchar *header_title, gboolean hidden)
{
	GList *list, *l;
	const gchar *title;

	g_assert(NULL != header_title); 
	list = gtk_tree_view_get_columns(treeview);
	g_assert(NULL != list); 

	for (l = list; NULL != l; l = g_list_next(l))
		if (NULL != l->data) {
			gtk_object_get(GTK_OBJECT(l->data), "title", &title, NULL);
			if (NULL != title && !strcmp(header_title, title)) {
				gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(l->data),
					hidden);
				break;
			}
		}

	g_list_free(list);
}

/***
 *** Callbacks
 ***/

static void on_gnet_stats_column_resized(
	GtkTreeViewColumn *column, gpointer data)
{
	const gchar *widget_name;
	const gchar *title;
	guint32 width;
	gint property;
	gint column_id;
	static gboolean lock = FALSE;

	if (lock)
		return;
	lock = TRUE;

	widget_name = gtk_widget_get_name(column->tree_view);
	title = gtk_tree_view_column_get_title(column);
 	width = gtk_tree_view_column_get_width(column);

#if 0
	g_message("%s: widget=\"%s\" title=\"%s\", width=%u",
		__FUNCTION__, widget_name, title, width);
#endif

	if (!(strcmp(title, "Type")))
		column_id = 0;
	else if (!(strcmp(title, "Count")))
		column_id = 1;
	else if (!(strcmp(title, "Received")))
		column_id = 1;
	else if (!(strcmp(title, "Expired")))
		column_id = 2;
	else if (!(strcmp(title, "Dropped")))
		column_id = 3;
	else if (!(strcmp(title, "Relayed")))
		column_id = 4;
	else if (!(strcmp(title, "Generated")))
		column_id = 5;
	else if (isdigit((guchar) title[0])) {
		column_id = (title[0] - '0') + 1;
		g_assert(column_id >= 1 && column_id <= 9);
	}
	else {
		column_id = -1;
		g_assert_not_reached();
	}

    if (!strcmp(widget_name, "treeview_gnet_stats_general"))
		property = PROP_GNET_STATS_GENERAL_COL_WIDTHS;
    else if (!strcmp(widget_name, "treeview_gnet_stats_drop_reasons"))
		property = PROP_GNET_STATS_DROP_REASONS_COL_WIDTHS;
    else if (!strcmp(widget_name, "treeview_gnet_stats_messages"))
		property = PROP_GNET_STATS_MSG_COL_WIDTHS;
    else if (!strcmp(widget_name, "treeview_gnet_stats_flowc"))
		property = PROP_GNET_STATS_FC_COL_WIDTHS;
	else {
		property = -1;
		g_assert_not_reached();
	}

	gui_prop_set_guint32(property, &width, column_id, 1);
	lock = FALSE;
}

static void on_gnet_stats_type_selected(GtkItem *i, gpointer data)
{
    selected_type = GPOINTER_TO_INT(data);
    gnet_stats_gui_update();
}

static void on_gnet_stats_msgs_toggled(GtkWidget *widget, gpointer data)
{
	gnet_stats_msgs_mode ^= STATS_MSGS_MODE_PACKETS;
    gnet_stats_gui_update();
}

static void on_gnet_stats_msgs_abs_toggled(GtkWidget *widget, gpointer data)
{
	gnet_stats_msgs_mode ^= STATS_MSGS_MODE_REL;
    gnet_stats_gui_update();
}

static void on_gnet_stats_fc_toggled(GtkWidget *widget, gpointer data)
{
	const gchar *name = gtk_widget_get_name(widget);
	/* FIXME: add properties to save settings */

	g_assert(NULL != name);
	if (!strcmp(name, "radio_fc_ttl") || !strcmp(name, "radio_fc_hops"))
		selected_flowc ^= 1;
	else if (!strcmp(name, "radio_fc_pkts") || !strcmp(name, "radio_fc_bytes"))
		selected_flowc ^= 2;
	else if (!strcmp(name, "radio_fc_abs") || !strcmp(name, "radio_fc_rel"))
		selected_flowc ^= 4;
	else if (!strcmp(name, "checkbutton_fc_headers"))
		selected_flowc ^= 8;
	else
		g_assert_not_reached();

	/* Hide column for TTL=0 */
	column_set_hidden(
		GTK_TREE_VIEW(lookup_widget(main_window, "treeview_gnet_stats_flowc")),
		"0",
		FLOWC_MODE_HOPS(selected_flowc));
	gnet_stats_gui_update();
}

/***
 *** Private functions
 ***/
G_INLINE_FUNC gchar *pkt_stat_str(const guint32 *val_tbl, gint type)
{
    static gchar strbuf[20];

    if (val_tbl[type] == 0)
        return "-";

    if (gnet_stats_msgs_mode & STATS_MSGS_MODE_REL)
        g_snprintf(strbuf, sizeof(strbuf), "%.2f%%", 
            (float)val_tbl[type]/val_tbl[MSG_TOTAL]*100.0);
    else
        g_snprintf(strbuf, sizeof(strbuf), "%u", val_tbl[type]);

    return strbuf;
}


G_INLINE_FUNC const gchar *byte_stat_str(const guint32 *val_tbl, gint type)
{
    static gchar strbuf[20];

    if (val_tbl[type] == 0)
        return  "-";

    if (gnet_stats_msgs_mode & STATS_MSGS_MODE_REL) {
        g_snprintf(strbuf, sizeof(strbuf), "%.2f%%", 
            (float)val_tbl[type]/val_tbl[MSG_TOTAL]*100.0);
        return strbuf;
    } else
        return compact_size(val_tbl[type]);
}

G_INLINE_FUNC const gchar *drop_stat_str(const gnet_stats_t *stats, gint reason)
{
    static gchar strbuf[20];
    guint32 total = stats->pkg.dropped[MSG_TOTAL];

    if (stats->drop_reason[reason][selected_type] == 0)
        return gnet_stats_drop_perc ? "-  " : "-";

    if (gnet_stats_drop_perc)
        g_snprintf(strbuf, sizeof(strbuf), "%.2f%%", 
            (float)stats->drop_reason[reason][selected_type]/total*100);
    else
        g_snprintf(strbuf, sizeof(strbuf), "%u", 
            stats->drop_reason[reason][selected_type]);

    return strbuf;
}

G_INLINE_FUNC const gchar *general_stat_str(
	const gnet_stats_t *stats, gint type)
{
    static gchar strbuf[20];

    if (stats->general[type] == 0)
        return "-";

    if (type == GNR_QUERY_COMPACT_SIZE) {
        return compact_size(stats->general[type]);
    } else {
        g_snprintf(strbuf, sizeof(strbuf), "%u", stats->general[type]);
        return strbuf;
    }
}

G_INLINE_FUNC const gchar *flowc_stat_str(gulong value, gulong total)
{
    static gchar strbuf[20];

	if (value == 0 || total == 0)
		return "-";

	if (FLOWC_MODE_ABS(selected_flowc)) {
		if (FLOWC_MODE_BYTE(selected_flowc))	/* byte mode */
			return compact_size(value);
		else									/* packet mode */
       		g_snprintf(strbuf, sizeof(strbuf), "%lu", (gulong) value);
	} else 
		g_snprintf(strbuf, sizeof(strbuf), "%.2f%%", (float) value/total*100.0);

    return strbuf;
}

static void add_column(
	GtkTreeView *treeview, gint column_id, const gchar *label)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
					label, renderer, "text", column_id, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column(treeview, column);
	g_object_notify(G_OBJECT(column), "width");
	g_signal_connect(G_OBJECT(column), "notify::width",
		(gpointer) on_gnet_stats_column_resized, NULL);
}

/***
 *** Public functions
 ***/

void gnet_stats_gui_init(void)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkCombo *combo;
    gint n;

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_messages"));
	model = GTK_TREE_MODEL(gtk_list_store_new(6,
							G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
							G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));

	for (n = 0; n < MSG_TYPE_COUNT; n++) {
		GtkTreeIter iter;
		gint i;

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			for (i = 0; i < 6; i++)
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, i,
					i == 0 ? msg_type_str[n] : "-", -1);
	}

	for (n = 0; n < 6; n++)
		add_column(treeview, n, msg_stats_label[n]);

    gtk_tree_view_set_model(treeview, model);
	g_object_unref(model);

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_flowc"));
	model = GTK_TREE_MODEL(
		gtk_list_store_new(STATS_FLOWC_COLUMNS,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING));

	for (n = 0; n < STATS_FLOWC_COLUMNS; n++) {
    	gchar buf[16];

		g_snprintf(buf, sizeof(buf), "%d%c", n-1,
				(n+1) < STATS_FLOWC_COLUMNS ? '\0' : '+');
		add_column(treeview, n, n == 0 ? "Type" : buf);
	}

	for (n = 0; n < MSG_TYPE_COUNT; n++) {
		GtkTreeIter iter;
		gint i;

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			for (i = 0; i < STATS_FLOWC_COLUMNS; i++)
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, i,
					i == 0 ? msg_type_str[n] : "-", -1);
	}

    gtk_tree_view_set_model(treeview, model);
	g_object_unref(model);

    /*
     * Initialize stats tables.
     */

    combo = GTK_COMBO(
        lookup_widget(main_window, "combo_gnet_stats_type"));

    for (n = 0; n < MSG_TYPE_COUNT; n ++) {
        GtkWidget *list_item;
        GList *l;

        list_item = gtk_list_item_new_with_label(msg_type_str[n]);
        gtk_widget_show(list_item);

        g_signal_connect(
            GTK_OBJECT(list_item), "select",
            G_CALLBACK(on_gnet_stats_type_selected),
            GINT_TO_POINTER(n));

        l = g_list_prepend(NULL, (gpointer) list_item);
        gtk_list_append_items(GTK_LIST(GTK_COMBO(combo)->list), l);

        if (n == MSG_TOTAL)
            gtk_list_select_child(GTK_LIST(GTK_COMBO(combo)->list), list_item);
    }


	/* ----------------------------------------- */

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_drop_reasons"));
	model = GTK_TREE_MODEL(
		gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING));

	for (n = 0; n < 2; n++) {
		GtkTreeIter iter;
		gint i;

		for (i = 0; n == 0 && i < MSG_DROP_REASON_COUNT; i++) {
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				0, msg_drop_str[i], 1, "-", -1);
		}

		add_column(treeview, n, n == 0 ? "Type" : "Count");
	}

    gtk_tree_view_set_model(treeview, model);
	g_object_unref(model);

	/* ----------------------------------------- */

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_general"));
	model = GTK_TREE_MODEL(
		gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING));

	for (n = 0; n < 2; n++) {
		GtkTreeIter iter;
		gint i;

		for (i = 0; n == 0 && i < GNR_TYPE_COUNT; i++) {
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				0, general_type_str[i], 1, "-", -1);
		}
		add_column(treeview, n, n == 0 ? "Type" : "Count");
	}

    gtk_tree_view_set_model(treeview, model);
	g_object_unref(model);

	/* Install signal handlers for view selection buttons */

	g_signal_connect(
		GTK_RADIO_BUTTON(lookup_widget(main_window, "radio_fc_ttl")),
		"toggled", G_CALLBACK(on_gnet_stats_fc_toggled), NULL);
	g_signal_connect(
		GTK_RADIO_BUTTON(lookup_widget(main_window, "radio_fc_pkts")),
		"toggled", G_CALLBACK(on_gnet_stats_fc_toggled), NULL);
	g_signal_connect(
		GTK_RADIO_BUTTON(lookup_widget(main_window, "radio_fc_abs")),
		"toggled", G_CALLBACK(on_gnet_stats_fc_toggled), NULL);
	g_signal_connect(
		GTK_CHECK_BUTTON(lookup_widget(main_window, "checkbutton_fc_headers")),
		"toggled", G_CALLBACK(on_gnet_stats_fc_toggled), NULL);

	g_signal_connect(GTK_RADIO_BUTTON(
		lookup_widget(main_window, "gnet_stats_msgs_radio_byte")),
		"toggled", G_CALLBACK(on_gnet_stats_msgs_toggled), NULL);
	g_signal_connect(GTK_RADIO_BUTTON(
		lookup_widget(main_window, "gnet_stats_msgs_radio_abs")),
		"toggled", G_CALLBACK(on_gnet_stats_msgs_abs_toggled), NULL);
}

void gnet_stats_gui_update(void)
{
    GtkTreeView *treeview;
    GtkListStore *store;
    GtkTreeIter iter;
    gint n;
    gnet_stats_t stats;
	static gboolean lock = FALSE;
	gchar *str[MSG_TYPE_COUNT];
	guint32 (*byte_counters)[MSG_TYPE_COUNT];
	guint32 (*pkg_counters)[MSG_TYPE_COUNT];
    gint current_page;

	if (lock)
		return;

	lock = TRUE;

    current_page = gtk_notebook_get_current_page(
        GTK_NOTEBOOK(lookup_widget(main_window, "notebook_main")));
    if (current_page != nb_main_page_gnet_stats) {
		lock = FALSE;
        return;
	}

    gnet_stats_get(&stats);

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_messages"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

    for (n = 0; n < MSG_TYPE_COUNT; n ++)
		if (gnet_stats_msgs_mode & STATS_MSGS_MODE_PACKETS) {
			gtk_list_store_set(store, &iter,
				c_gs_received, pkt_stat_str(stats.pkg.received, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_generated, pkt_stat_str(stats.pkg.generated, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_dropped, pkt_stat_str(stats.pkg.dropped, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_expired, pkt_stat_str(stats.pkg.expired, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_relayed, pkt_stat_str(stats.pkg.relayed, n), -1);
			if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
				break;
		} else { /* STATS_MSGS_MODE_BYTES */
			gtk_list_store_set(store, &iter,
				c_gs_received, byte_stat_str(stats.byte.received, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_generated, byte_stat_str(stats.byte.generated, n), -1);
			gtk_list_store_set(store, &iter,
						c_gs_dropped, byte_stat_str(stats.byte.dropped, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_expired, byte_stat_str(stats.byte.expired, n), -1);
			gtk_list_store_set(store, &iter,
				c_gs_relayed, byte_stat_str(stats.byte.relayed, n), -1);
			if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
				break;
    	}

	if (FLOWC_MODE_HOPS(selected_flowc)) {
		/* Hops mode */
		pkg_counters = stats.pkg.flowc_hops;
		byte_counters = stats.byte.flowc_hops;
	} else {
		/* TTL mode */
		pkg_counters = stats.pkg.flowc_ttl;
		byte_counters = stats.byte.flowc_ttl;
	}

	/* ------------------------------------------------------------ */

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_flowc"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for (n = 0; n < MSG_TYPE_COUNT; n++) {
		gint i;

		if (FLOWC_MODE_PKTS(selected_flowc))	/* packet mode */
			for (i = 0; i < STATS_FLOWC_COLUMNS; i++)
				str[i] = g_strdup(flowc_stat_str(
					(gulong) pkg_counters[i][n],
					(gulong) pkg_counters[i][MSG_TOTAL]));
		else									/* byte mode */
			for (i = 0; i < STATS_FLOWC_COLUMNS; i++)
				str[i] = g_strdup(FLOWC_MODE_HDRS(selected_flowc)
					?  flowc_stat_str(		/* add headers */
							(gulong) byte_counters[i][n]
								+ (gulong) pkg_counters[i][n]
									* sizeof(struct gnutella_header),
							(gulong) byte_counters[i][MSG_TOTAL]
								+ (gulong) pkg_counters[i][MSG_TOTAL]
									* sizeof(struct gnutella_header))

					:  flowc_stat_str(		/* show payload only */ 
						(gulong) byte_counters[i][n],
						(gulong) byte_counters[i][MSG_TOTAL]));

		for (i = 1; i < STATS_FLOWC_COLUMNS; i++)
			gtk_list_store_set(store, &iter, i, str[i-1], -1);
#if 0		
		g_message("%-12s %-4s %-4s %-4s %-4s %-4s %-4s %-4s",
			msg_type_str[n],
			str[0], str[1], str[2], str[3], str[4], str[5], str[6]);
#endif

		for (i = 0; i < STATS_FLOWC_COLUMNS; i++)
			G_FREE_NULL(str[i]);
		if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
			break;
	}

#if 0
	g_message(" ");
#endif

	/* ------------------------------------------------------------ */

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_drop_reasons"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for (n = 0; n < MSG_DROP_REASON_COUNT; n++) {
		gtk_list_store_set(store, &iter, 1, drop_stat_str(&stats, n++), -1);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}

	/* ------------------------------------------------------------ */

    treeview = GTK_TREE_VIEW(
        lookup_widget(main_window, "treeview_gnet_stats_general"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for (n = 0; n < GNR_TYPE_COUNT; n++) {
		gtk_list_store_set(store, &iter, 1, general_stat_str(&stats, n++), -1);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}

	lock = FALSE;
}
