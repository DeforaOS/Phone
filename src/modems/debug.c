/* $Id$ */
/* Copyright (c) 2011-2020 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Phone/modem.h>
#include <gtk/gtk.h>
#include <System.h>
#include "../../config.h"


/* Debug */
/* private */
/* types */
typedef struct _ModemPlugin
{
	ModemPluginHelper * helper;

	guint source;

	/* widgets */
	GtkWidget * window;
	GtkWidget * status;
	GtkWidget * operator;
	GtkWidget * roaming;
	GtkWidget * ca_number;
	GtkWidget * ca_direction;
	GtkWidget * me_number;
	GtkWidget * me_folder;
	GtkWidget * me_message;
	GtkWidget * no_type;
	GtkWidget * no_title;
	GtkWidget * no_message;

	/* events */
	ModemEvent event_call;
	ModemEvent event_contact;
	ModemEvent event_message;
} Debug;


/* prototypes */
/* modem */
static ModemPlugin * _debug_init(ModemPluginHelper * helper);
static void _debug_destroy(ModemPlugin * modem);
static int _debug_start(ModemPlugin * modem, unsigned int retry);
static int _debug_stop(ModemPlugin * modem);
static int _debug_request(ModemPlugin * modem, ModemRequest * request);
static int _debug_trigger(ModemPlugin * modem, ModemEventType event);

/* accessors */
static void _debug_set_status(ModemPlugin * modem, char const * status);

/* callbacks */
static gboolean _debug_on_closex(gpointer data);
static void _debug_on_call(gpointer data);
static void _debug_on_message_send(gpointer data);
static void _debug_on_notification(gpointer data);
static void _debug_on_operator_set(gpointer data);


/* public */
/* variables */
ModemPluginDefinition plugin =
{
	"Debug",
	"applications-development",
	NULL,
	_debug_init,
	_debug_destroy,
	_debug_start,
	_debug_stop,
	_debug_request,
	_debug_trigger
};


/* private */
/* functions */
/* modem */
static ModemPlugin * _debug_init(ModemPluginHelper * helper)
{
	Debug * debug;
	GtkSizeGroup * group;
	GtkWidget * vbox;
	GtkWidget * notebook;
	GtkWidget * hbox;
	GtkWidget * widget;

	if((debug = object_new(sizeof(*debug))) == NULL)
		return NULL;
	debug->helper = helper;
	debug->source = 0;
	memset(&debug->event_call, 0, sizeof(debug->event_call));
	memset(&debug->event_contact, 0, sizeof(debug->event_contact));
	memset(&debug->event_message, 0, sizeof(debug->event_message));
	/* window */
	debug->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(debug->window), 4);
	gtk_window_set_title(GTK_WINDOW(debug->window), "Debug");
	g_signal_connect_swapped(G_OBJECT(debug->window), "delete-event",
			G_CALLBACK(_debug_on_closex), debug);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	notebook = gtk_notebook_new();
	/* status */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Status:");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->status = gtk_label_new("initialized");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(debug->status, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(debug->status), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), debug->status, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* operator */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Operator: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->operator = gtk_entry_new();
	g_signal_connect_swapped(debug->operator, "activate", G_CALLBACK(
				_debug_on_operator_set), debug);
	gtk_box_pack_start(GTK_BOX(hbox), debug->operator, TRUE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_debug_on_operator_set), debug);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(NULL);
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->roaming = gtk_check_button_new_with_mnemonic("_Roaming");
	gtk_box_pack_start(GTK_BOX(hbox), debug->roaming, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new("Status"));
	/* calls */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Number: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->ca_number = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), debug->ca_number, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Direction: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	debug->ca_direction = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->ca_direction), NULL,
			"Incoming");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->ca_direction), NULL,
			"Outgoing");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->ca_direction), NULL,
			"Established");
#else
	debug->ca_direction = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->ca_direction),
			"Incoming");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->ca_direction),
			"Outgoing");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->ca_direction),
			"Established");
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(debug->ca_direction), 1);
	gtk_box_pack_start(GTK_BOX(hbox), debug->ca_direction, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_button_new_with_label("Call");
	gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_icon_name(
				"call-start", GTK_ICON_SIZE_BUTTON));
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_debug_on_call), debug);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new("Calls"));
	/* messages */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Number: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->me_number = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), debug->me_number, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Folder: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	debug->me_folder = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->me_folder), NULL,
			"Unknown");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->me_folder), NULL,
			"Inbox");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->me_folder), NULL,
			"Sent");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->me_folder), NULL,
			"Drafts");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->me_folder), NULL,
			"Trash");
#else
	debug->me_folder = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->me_folder), "Unknown");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->me_folder), "Inbox");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->me_folder), "Sent");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->me_folder), "Drafts");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->me_folder), "Trash");
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(debug->me_folder), 1);
	gtk_box_pack_start(GTK_BOX(hbox), debug->me_folder, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Message: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START,
			"valign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.0);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_ETCHED_IN);
	debug->me_message = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(widget), debug->me_message);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_button_new_with_label("Send");
	gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_icon_name(
				"mail-send", GTK_ICON_SIZE_BUTTON));
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_debug_on_message_send), debug);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new("Messages"));
	/* notification */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Type: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	debug->no_type = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->no_type), NULL,
			"Information");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->no_type), NULL,
			"Error");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(debug->no_type), NULL,
			"Warning");
#else
	debug->no_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->no_type), "Information");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->no_type), "Error");
	gtk_combo_box_append_text(GTK_COMBO_BOX(debug->no_type), "Warning");
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(debug->no_type), 0);
	gtk_box_pack_start(GTK_BOX(hbox), debug->no_type, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* notification: title */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Title: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	debug->no_title = gtk_entry_new();
	g_signal_connect_swapped(debug->no_title, "activate", G_CALLBACK(
				_debug_on_notification), debug);
	gtk_box_pack_start(GTK_BOX(hbox), debug->no_title, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* notification: message */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Message: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START,
			"valign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.0);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_ETCHED_IN);
	debug->no_message = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(widget), debug->no_message);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	/* notification: send */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_button_new_with_label("Send");
	gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_icon_name(
				"mail-send", GTK_ICON_SIZE_BUTTON));
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_debug_on_notification), debug);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new("Notifications"));
	gtk_container_add(GTK_CONTAINER(debug->window), notebook);
	gtk_widget_show_all(debug->window);
	return debug;
}


/* debug_destroy */
static void _debug_destroy(ModemPlugin * modem)
{
	Debug * debug = modem;

	if(debug->source != 0)
		g_source_remove(debug->source);
	gtk_widget_destroy(debug->window);
	object_delete(debug);
}


/* debug_start */
static gboolean _start_on_idle(gpointer data);

static int _debug_start(ModemPlugin * modem, unsigned int retry)
{
	Debug * debug = modem;
	(void) retry;

	_debug_set_status(modem, "starting");
	if(debug->source != 0)
		g_source_remove(debug->source);
	debug->source = g_idle_add(_start_on_idle, modem);
	return 0;
}

static gboolean _start_on_idle(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;

	debug->source = 0;
	_debug_set_status(modem, "started");
	return FALSE;
}


/* debug_stop */
static gboolean _stop_on_idle(gpointer data);

static int _debug_stop(ModemPlugin * modem)
{
	Debug * debug = modem;

	_debug_set_status(modem, "stopping");
	if(debug->source != 0)
		g_source_remove(debug->source);
	debug->source = g_idle_add(_stop_on_idle, modem);
	return 0;
}

static gboolean _stop_on_idle(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;

	debug->source = 0;
	_debug_set_status(modem, "stopped");
	return FALSE;
}


/* debug_request */
static int _debug_request(ModemPlugin * modem, ModemRequest * request)
{
	Debug * debug = modem;
	ModemPluginHelper * helper = debug->helper;
	ModemEvent event;
	unsigned int u;
	char buf[32];

	if(request == NULL)
		return -1;
	memset(&event, 0, sizeof(event));
	switch(request->type)
	{
		case MODEM_REQUEST_CONTACT_DELETE:
			event.type = MODEM_EVENT_TYPE_CONTACT_DELETED;
			event.contact_deleted.id = request->contact_delete.id;
			helper->event(helper->modem, &event);
			break;
		case MODEM_REQUEST_CONTACT_EDIT:
			debug->event_contact.type = MODEM_EVENT_TYPE_CONTACT;
			u = debug->event_contact.contact.id;
			debug->event_contact.contact.id
				= request->contact_edit.id;
			debug->event_contact.contact.status
				= rand() % MODEM_CONTACT_STATUS_COUNT;
			debug->event_contact.contact.name
				= request->contact_edit.name;
			debug->event_contact.contact.number
				= request->contact_edit.number;
			helper->event(helper->modem, &debug->event_contact);
			debug->event_contact.contact.id = u;
			break;
		case MODEM_REQUEST_CONTACT_NEW:
			debug->event_contact.type = MODEM_EVENT_TYPE_CONTACT;
			debug->event_contact.contact.id++;
			debug->event_contact.contact.status
				= rand() % MODEM_CONTACT_STATUS_COUNT;
			debug->event_contact.contact.name
				= request->contact_new.name;
			debug->event_contact.contact.number
				= request->contact_new.number;
			helper->event(helper->modem, &debug->event_contact);
			break;
		case MODEM_REQUEST_DTMF_SEND:
			u = request->dtmf_send.dtmf;
			snprintf(buf, sizeof(buf), "Sending DTMF '%c'\n", u);
			event.type = MODEM_EVENT_TYPE_NOTIFICATION;
			event.notification.content = buf;
			debug->helper->event(debug->helper->modem, &event);
			break;
		case MODEM_REQUEST_MESSAGE_DELETE:
			event.type = MODEM_EVENT_TYPE_MESSAGE_DELETED;
			event.message_deleted.id = request->message_delete.id;
			helper->event(helper->modem, &event);
			break;
		default:
			break;
	}
	return 0;
}


/* debug_trigger */
static int _debug_trigger(ModemPlugin * modem, ModemEventType event)
{
	Debug * debug = modem;
	ModemEvent e;

	memset(&e, 0, sizeof(e));
	switch(event)
	{
		case MODEM_EVENT_TYPE_MODEL:
			e.type = MODEM_EVENT_TYPE_MODEL;
			e.model.vendor = "Phone";
			e.model.name = PACKAGE;
			e.model.version = VERSION;
			e.model.serial = "SERIAL-NUMBER";
			debug->helper->event(debug->helper->modem, &e);
			return 0;
		default:
			return 1;
	}
}


/* accessors */
/* debug_set_status */
static void _debug_set_status(ModemPlugin * modem, char const * status)
{
	Debug * debug = modem;

	gtk_label_set_text(GTK_LABEL(debug->status), status);
}


/* callbacks */
/* debug_on_closex */
static gboolean _debug_on_closex(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;

	gtk_widget_hide(debug->window);
	gtk_main_quit();
	return TRUE;
}


/* debug_on_call */
static void _debug_on_call(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;
	int d;

	debug->event_call.call.type = MODEM_EVENT_TYPE_CALL;
	debug->event_call.call.call_type = MODEM_CALL_TYPE_VOICE;
	d = gtk_combo_box_get_active(GTK_COMBO_BOX(debug->ca_direction));
	switch(d)
	{
		case 0:
			debug->event_call.call.direction
				= MODEM_CALL_DIRECTION_INCOMING;
			debug->event_call.call.status
				= MODEM_CALL_STATUS_RINGING;
			break;
		case 1:
			debug->event_call.call.direction
				= MODEM_CALL_DIRECTION_OUTGOING;
			debug->event_call.call.status
				= MODEM_CALL_STATUS_RINGING;
			break;
		case 2:
			debug->event_call.call.direction
				= MODEM_CALL_DIRECTION_NONE;
			debug->event_call.call.status
				= MODEM_CALL_STATUS_ACTIVE;
			break;
		default:
			return;
	}
	debug->event_call.call.number = gtk_entry_get_text(GTK_ENTRY(
				debug->ca_number));
	debug->helper->event(debug->helper->modem, &debug->event_call);
}


/* debug_on_message_send */
static void _debug_on_message_send(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;
	gchar * content;

	debug->event_message.message.type = MODEM_EVENT_TYPE_MESSAGE;
	debug->event_message.message.id++;
	debug->event_message.message.date = time(NULL);
	debug->event_message.message.number = gtk_entry_get_text(GTK_ENTRY(
				debug->me_number));
	debug->event_message.message.folder = gtk_combo_box_get_active(
			GTK_COMBO_BOX(debug->me_folder));
	debug->event_message.message.status = MODEM_MESSAGE_STATUS_NEW;
	debug->event_message.message.encoding = MODEM_MESSAGE_ENCODING_UTF8;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(debug->me_message));
	gtk_text_buffer_get_start_iter(tbuf, &start);
	gtk_text_buffer_get_end_iter(tbuf, &end);
	content = gtk_text_buffer_get_text(tbuf, &start, &end, FALSE);
	debug->event_message.message.length = strlen(content);
	debug->event_message.message.content = content;
	debug->helper->event(debug->helper->modem, &debug->event_message);
	g_free(content);
}


/* debug_on_notification */
static void _debug_on_notification(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;
	ModemEvent event;
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;
	gchar * p;

	memset(&event, 0, sizeof(event));
	event.type = MODEM_EVENT_TYPE_NOTIFICATION;
	event.notification.ntype = gtk_combo_box_get_active(
			GTK_COMBO_BOX(debug->no_type));
	event.notification.title = gtk_entry_get_text(GTK_ENTRY(
				debug->no_title));
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(debug->no_message));
	gtk_text_buffer_get_start_iter(tbuf, &start);
	gtk_text_buffer_get_end_iter(tbuf, &end);
	p = gtk_text_buffer_get_text(tbuf, &start, &end, FALSE);
	event.notification.content = p;
	debug->helper->event(debug->helper->modem, &event);
	g_free(p);
}


/* debug_on_operator_set */
static void _debug_on_operator_set(gpointer data)
{
	ModemPlugin * modem = data;
	Debug * debug = modem;
	ModemEvent event;
	char const * p;

	memset(&event, 0, sizeof(event));
	p = gtk_entry_get_text(GTK_ENTRY(debug->operator));
	event.type = MODEM_EVENT_TYPE_REGISTRATION;
	event.registration.status = MODEM_REGISTRATION_STATUS_REGISTERED;
	event.registration._operator = p;
	event.registration.roaming = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(debug->roaming));
	debug->helper->event(debug->helper->modem, &event);
}
