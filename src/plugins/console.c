/* $Id$ */
/* Copyright (c) 2016 Pierre Pronchery <khorben@defora.org> */
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
#include <time.h>
#include <gtk/gtk.h>
#include "Phone.h"


/* Console */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	GtkWidget * window;
	GtkListStore * events;
	GtkWidget * view;
} Console;

typedef enum _ConsoleColumn
{
	CC_TYPE = 0,
	CC_ICON,
	CC_DATE,
	CC_DATE_DISPLAY,
	CC_TITLE,
	CC_MESSAGE
} ConsoleColumn;
#define CC_LAST CC_MESSAGE
#define CC_COUNT (CC_LAST + 1)


/* prototypes */
/* plug-in */
static Console * _console_init(PhonePluginHelper * helper);
static void _console_destroy(Console * console);
static int _console_event(Console * console, PhoneEvent * event);
static void _console_settings(Console * console);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Console",
	"gnome-monitor",
	NULL,
	_console_init,
	_console_destroy,
	_console_event,
	_console_settings
};


/* private */
/* functions */
/* plug-in */
/* console_init */
static gboolean _console_on_closex(gpointer data);

static Console * _console_init(PhonePluginHelper * helper)
{
	Console * console;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	if((console = object_new(sizeof(*console))) == NULL)
		return NULL;
	console->helper = helper;
	console->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(console->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(console->window), plugin.icon);
#endif
	gtk_window_set_title(GTK_WINDOW(console->window), plugin.name);
	g_signal_connect_swapped(console->window, "delete-event", G_CALLBACK(
				_console_on_closex), console);
	/* vbox */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* events */
	console->events = gtk_list_store_new(CC_COUNT, G_TYPE_UINT,
			GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	console->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				console->events));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(console->view), TRUE);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer,
			"pixbuf", CC_ICON, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CC_TYPE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(console->view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Time", renderer,
			"text", CC_DATE_DISPLAY, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CC_DATE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(console->view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title", renderer,
			"text", CC_TITLE, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CC_TITLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(console->view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Message", renderer,
			"text", CC_MESSAGE, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CC_MESSAGE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(console->view), column);
	gtk_container_add(GTK_CONTAINER(widget), console->view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(console->window), vbox);
	gtk_widget_show_all(console->window);
	return console;
}

static gboolean _console_on_closex(gpointer data)
{
	Console * console = data;

	gtk_widget_hide(console->window);
	return TRUE;
}


/* console_destroy */
static void _console_destroy(Console * console)
{
	gtk_widget_destroy(console->window);
	object_delete(console);
}


/* console_event */
static int _event_notification();

static int _console_event(Console * console, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_NOTIFICATION:
			return _event_notification(console, event);
		default:
			break;
	}
	return 0;
}

static int _event_notification(Console * console, PhoneEvent * event)
{
	const unsigned int flags = 0;
	GtkIconTheme * icontheme;
	char const * p = "dialog-question";
	GdkPixbuf * icon;
	time_t date;
	struct tm t;
	char tbuf[32];
	GtkTreeIter iter;

	icontheme = gtk_icon_theme_get_default();
	switch(event->notification.ntype)
	{
		case PHONE_NOTIFICATION_TYPE_ERROR:
			p = "dialog-error";
			break;
		case PHONE_NOTIFICATION_TYPE_INFO:
			p = "dialog-information";
			break;
		case PHONE_NOTIFICATION_TYPE_WARNING:
			p = "dialog-warning";
			break;
	}
	icon = gtk_icon_theme_load_icon(icontheme, p, 16, flags, NULL);
	date = time(NULL);
	localtime_r(&date, &t);
	strftime(tbuf, sizeof(tbuf), "%d/%m/%Y %H:%M:%S", &t);
	gtk_list_store_append(console->events, &iter);
	gtk_list_store_set(console->events, &iter,
			CC_TYPE, event->notification.ntype, CC_ICON, icon,
			CC_DATE, date, CC_DATE_DISPLAY, tbuf,
			CC_TITLE, event->notification.title,
			CC_MESSAGE, event->notification.message, -1);
	if(icon != NULL)
		g_object_unref(icon);
	return 0;
}


/* console_settings */
static void _console_settings(Console * console)
{
	gtk_window_present(GTK_WINDOW(console->window));
}
