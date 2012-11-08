/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Phone */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <System.h>
#include <Desktop.h>
#if 0 /* XXX avoid a dependency on Panel */
# include <Desktop/Panel.h>
#else
# define PANEL_CLIENT_MESSAGE	"DEFORAOS_DESKTOP_PANEL_CLIENT"
# define PANEL_MESSAGE_EMBED	1
#endif
#include "Phone.h"


/* Notify */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
} NotifyPhonePlugin;


/* prototypes */
/* plug-in */
static NotifyPhonePlugin * _notify_init(PhonePluginHelper * helper);
static void _notify_destroy(NotifyPhonePlugin * notify);
static int _notify_event(NotifyPhonePlugin * notify, PhoneEvent * event);

/* callbacks */
static void _notify_on_close(gpointer data);
static gboolean _notify_on_timeout(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Notifications",
	"gtk-dialog-info",
	NULL,
	_notify_init,
	_notify_destroy,
	_notify_event,
	NULL
};


/* private */
/* functions */
/* notify_init */
static NotifyPhonePlugin * _notify_init(PhonePluginHelper * helper)
{
	NotifyPhonePlugin * notify;

	if((notify = object_new(sizeof(*notify))) == NULL)
		return NULL;
	notify->helper = helper;
	return notify;
}


/* notify_destroy */
static void _notify_destroy(NotifyPhonePlugin * notify)
{
	object_delete(notify);
}


/* notify_event */
static int _event_notification(NotifyPhonePlugin * notify, PhoneEvent * event);

static int _notify_event(NotifyPhonePlugin * notify, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_NOTIFICATION:
			return _event_notification(notify, event);
		default:
			break;
	}
	return 0;
}

static int _event_notification(NotifyPhonePlugin * notify, PhoneEvent * event)
{
	PangoFontDescription * bold;
	char const * title = event->notification.title;
	char const * stock;
	GtkWidget * plug;
	GtkWidget * hbox;
	GtkWidget * vbox;
	GtkWidget * image;
	GtkWidget * widget;
	uint32_t xid;

	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
	switch(event->notification.ntype)
	{
		case PHONE_NOTIFICATION_TYPE_ERROR:
			stock = GTK_STOCK_DIALOG_ERROR;
			if(title == NULL)
				title = "Error";
			break;
		case PHONE_NOTIFICATION_TYPE_WARNING:
			stock = GTK_STOCK_DIALOG_WARNING;
			if(title == NULL)
				title = "Warning";
			break;
		case PHONE_NOTIFICATION_TYPE_INFO:
		default:
			stock = GTK_STOCK_DIALOG_INFO;
			if(title == NULL)
				title = "Information";
			break;
	}
	plug = gtk_plug_new(0);
	gtk_container_set_border_width(GTK_CONTAINER(plug), 4);
	hbox = gtk_hbox_new(FALSE, 4);
	/* icon */
	widget = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* title */
	vbox = gtk_vbox_new(FALSE, 4);
	widget = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.0);
	gtk_widget_modify_font(widget, bold);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* label */
	widget = gtk_label_new(event->notification.message);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	/* button */
	widget = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
			GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget), image);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_notify_on_close), plug);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(plug), hbox);
	gtk_widget_show_all(plug);
	xid = gtk_plug_get_id(GTK_PLUG(plug));
	desktop_message_send(PANEL_CLIENT_MESSAGE, PANEL_MESSAGE_EMBED, xid, 0);
	g_timeout_add(3000, _notify_on_timeout, plug);
	pango_font_description_free(bold);
	return 1;
}

static void _notify_on_close(gpointer data)
{
	GtkWidget * widget = data;

	/* FIXME also remove the timeout source */
	gtk_widget_destroy(widget);
}

static gboolean _notify_on_timeout(gpointer data)
{
	GtkWidget * widget = data;

	_notify_on_close(widget);
	return FALSE;
}
