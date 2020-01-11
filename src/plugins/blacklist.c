/* $Id$ */
/* Copyright (c) 2011-2018 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Phone */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <System.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "Phone.h"


/* Blacklist */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	GtkWidget * window;
	GtkListStore * store;
	GtkWidget * view;
} Blacklist;


/* prototypes */
static Blacklist * _blacklist_init(PhonePluginHelper * helper);
static void _blacklist_destroy(Blacklist * blacklist);
static int _blacklist_event(Blacklist * blacklist, PhoneEvent * event);
static void _blacklist_settings(Blacklist * blacklist);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Blacklist",
	"network-error",
	NULL,
	_blacklist_init,
	_blacklist_destroy,
	_blacklist_event,
	_blacklist_settings
};


/* private */
/* functions */
/* blacklist_init */
static void _init_foreach(char const * variable, char const * value,
		void * priv);

static Blacklist * _blacklist_init(PhonePluginHelper * helper)
{
	Blacklist * blacklist;

	if((blacklist = object_new(sizeof(*blacklist))) == NULL)
		return NULL;
	blacklist->helper = helper;
	blacklist->window = NULL;
	blacklist->store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	helper->config_foreach(helper->phone, "blacklist", _init_foreach,
			blacklist);
	return blacklist;
}

static void _init_foreach(char const * variable, char const * value,
		void * priv)
{
	Blacklist * blacklist = priv;
	GtkTreeIter iter;

	gtk_list_store_append(blacklist->store, &iter);
	gtk_list_store_set(blacklist->store, &iter, 0, variable, 1, value, -1);
}


/* blacklist_destroy */
static void _blacklist_destroy(Blacklist * blacklist)
{
	if(blacklist->window != NULL)
		gtk_widget_destroy(blacklist->window);
	object_delete(blacklist);
}


/* blacklist_event */
static int _blacklist_event(Blacklist * blacklist, PhoneEvent * event)
{
	PhonePluginHelper * helper = blacklist->helper;
	char const * number = NULL;
	char const * reason;

	switch(event->type)
	{
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			if(event->modem_event.event->type
					!= MODEM_EVENT_TYPE_CALL)
				break; /* FIXME many more events to handle */
			number = event->modem_event.event->call.number;
			break;
		default:
			return 0;
	}
	if(number == NULL)
		return 0;
	reason = helper->config_get(helper->phone, "blacklist", number);
	if(reason == NULL)
		return 0;
	return helper->error(helper->phone, reason, 1);
}


/* blacklist_settings */
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_delete(gpointer data);
static void _on_settings_new(gpointer data);
static void _on_settings_number_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data);
static void _on_settings_reason_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data);

static void _blacklist_settings(Blacklist * blacklist)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	if(blacklist->window != NULL)
	{
		gtk_window_present(GTK_WINDOW(blacklist->window));
		return;
	}
	blacklist->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(blacklist->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	/* XXX find something more appropriate */
	gtk_window_set_icon_name(GTK_WINDOW(blacklist->window), "blacklist");
#endif
	gtk_window_set_title(GTK_WINDOW(blacklist->window), "Blacklisting");
	g_signal_connect_swapped(blacklist->window, "delete-event", G_CALLBACK(
				_on_settings_closex), blacklist);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* toolbar */
	widget = gtk_toolbar_new();
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_on_settings_new), blacklist);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_on_settings_delete), blacklist);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	blacklist->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				blacklist->store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(blacklist->view), TRUE);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_on_settings_number_edited), blacklist);
	column = gtk_tree_view_column_new_with_attributes("Number",
			renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(blacklist->view), column);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_on_settings_reason_edited), blacklist);
	column = gtk_tree_view_column_new_with_attributes("Reason",
			renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(blacklist->view), column);
	gtk_container_add(GTK_CONTAINER(widget), blacklist->view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(blacklist->window), vbox);
	gtk_widget_show_all(blacklist->window);
}

static gboolean _on_settings_closex(gpointer data)
{
	Blacklist * blacklist = data;

	gtk_widget_hide(blacklist->window);
	return TRUE;
}

static void _on_settings_delete(gpointer data)
{
	Blacklist * blacklist = data;
	PhonePluginHelper * helper = blacklist->helper;
	GtkTreeSelection * treesel;
	GtkTreeIter iter;
	char * number = NULL;

	if((treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
						blacklist->view))) == NULL)
		return;
	if(gtk_tree_selection_get_selected(treesel, NULL, &iter) != TRUE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(blacklist->store), &iter, 0, &number,
			-1);
	if(number == NULL)
		return;
	helper->config_set(helper->phone, "blacklist", number, NULL);
	gtk_list_store_remove(blacklist->store, &iter);
	g_free(number);
}

static void _on_settings_new(gpointer data)
{
	Blacklist * blacklist = data;
	GtkTreeIter iter;

	gtk_list_store_append(blacklist->store, &iter);
	gtk_list_store_set(blacklist->store, &iter, 0, "number", -1);
}

static void _on_settings_number_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data)
{
	Blacklist * blacklist = data;
	PhonePluginHelper * helper = blacklist->helper;
	GtkTreeModel * model = GTK_TREE_MODEL(blacklist->store);
	GtkTreeIter iter;
	char * number = NULL;
	char const * reason;
	(void) renderer;

	if(gtk_tree_model_get_iter_from_string(model, &iter, arg1) == TRUE)
		gtk_tree_model_get(model, &iter, 0, &number, -1);
	if(number == NULL)
		return;
	/* FIXME check that there are no duplicates */
	reason = helper->config_get(helper->phone, "blacklist", number);
	if(helper->config_set(helper->phone, "blacklist", arg2, reason) == 0
			&& helper->config_set(helper->phone, "blacklist",
				number, NULL) == 0)
		gtk_list_store_set(blacklist->store, &iter, 0, arg2, -1);
	g_free(number);
}

static void _on_settings_reason_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data)
{
	Blacklist * blacklist = data;
	PhonePluginHelper * helper = blacklist->helper;
	GtkTreeModel * model = GTK_TREE_MODEL(blacklist->store);
	GtkTreeIter iter;
	char * number = NULL;
	(void) renderer;

	if(gtk_tree_model_get_iter_from_string(model, &iter, arg1) == TRUE)
		gtk_tree_model_get(model, &iter, 0, &number, -1);
	if(number == NULL)
		return;
	if(helper->config_set(helper->phone, "blacklist", number, arg2) == 0)
		gtk_list_store_set(blacklist->store, &iter, 1, arg2, -1);
	g_free(number);
}
