/* $Id$ */
/* Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include <System.h>
#include <Desktop.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
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
typedef struct _PhonePlugin NotifyPhonePlugin;

typedef struct _NotifyWidget
{
	NotifyPhonePlugin * notify;
	GtkWidget * widget;
	guint source;
} NotifyWidget;

struct _PhonePlugin
{
	PhonePluginHelper * helper;
	NotifyWidget ** widgets;
	size_t widgets_cnt;
};


/* prototypes */
/* plug-in */
static NotifyPhonePlugin * _notify_init(PhonePluginHelper * helper);
static void _notify_destroy(NotifyPhonePlugin * notify);
static int _notify_event(NotifyPhonePlugin * notify, PhoneEvent * event);

static NotifyWidget * _notify_widget_add(NotifyPhonePlugin * notify);
static void _notify_widget_remove(NotifyPhonePlugin * notify,
		NotifyWidget * widget);

/* widgets */
static NotifyWidget * _notifywidget_new(NotifyPhonePlugin * notify);
static void _notifywidget_delete(NotifyWidget * widget);

/* callbacks */
static void _notifywidget_on_close(gpointer data);
static gboolean _notifywidget_on_timeout(gpointer data);


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
	notify->widgets = NULL;
	notify->widgets_cnt = 0;
	return notify;
}


/* notify_destroy */
static void _notify_destroy(NotifyPhonePlugin * notify)
{
	size_t i;

	for(i = 0; i < notify->widgets_cnt; i++)
		if(notify->widgets[i] != NULL)
			_notifywidget_delete(notify->widgets[i]);
	free(notify->widgets);
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
	NotifyWidget * widget;
	PangoFontDescription * bold;
	char const * title = event->notification.title;
	char const * stock;
	GtkWidget * hbox;
	GtkWidget * vbox;
	GtkWidget * image;
	GtkWidget * label;
	GtkWidget * button;
	uint32_t xid;

	if((widget = _notify_widget_add(notify)) == NULL)
		return 0;
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
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	/* icon */
	image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);
	/* title */
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_widget_modify_font(label, bold);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	/* label */
	label = gtk_label_new(event->notification.message);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	/* button */
	button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
			GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(
				_notifywidget_on_close), widget);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(widget->widget), hbox);
	gtk_widget_show_all(widget->widget);
	xid = gtk_plug_get_id(GTK_PLUG(widget->widget));
	desktop_message_send(PANEL_CLIENT_MESSAGE, PANEL_MESSAGE_EMBED, xid, 0);
	pango_font_description_free(bold);
	return 1;
}


/* widgets */
/* notify_widget_add */
static NotifyWidget * _notify_widget_add(NotifyPhonePlugin * notify)
{
	size_t i;
	NotifyWidget ** p;

	for(i = 0; i < notify->widgets_cnt; i++)
		if(notify->widgets[i] == NULL)
			break;
	if(i == notify->widgets_cnt)
	{
		if((p = realloc(notify->widgets, (notify->widgets_cnt + 1)
						* sizeof(*p))) == NULL)
			return NULL;
		notify->widgets = p;
		notify->widgets[notify->widgets_cnt++] = NULL;
	}
	if((notify->widgets[i] = _notifywidget_new(notify)) == NULL)
		return NULL;
	return notify->widgets[i];
}


/* notify_widget_remove */
static void _notify_widget_remove(NotifyPhonePlugin * notify,
		NotifyWidget * widget)
{
	size_t i;
	NotifyWidget ** p;

	for(i = 0; i < notify->widgets_cnt; i++)
		if(widget == notify->widgets[i])
			break;
	if(i == notify->widgets_cnt)
		return;
	_notifywidget_delete(widget);
	notify->widgets[i] = NULL;
	/* free memory if we were the last element */
	if(i != notify->widgets_cnt - 1)
		return;
	for(; i > 0 && notify->widgets[i - 1] == NULL; i--);
	if((p = realloc(notify->widgets, (i - 1) * sizeof(*p))) == NULL
			&& (i - 1) != 0)
		return;
	notify->widgets = p;
	notify->widgets_cnt = i - 1;
}


/* notifywidget_new */
static NotifyWidget *  _notifywidget_new(NotifyPhonePlugin * notify)
{
	NotifyWidget * widget;

	if((widget = malloc(sizeof(*widget))) == NULL)
		return NULL;
	widget->notify = notify;
	widget->widget = gtk_plug_new(0);
	gtk_container_set_border_width(GTK_CONTAINER(widget->widget), 4);
	widget->source = g_timeout_add(3000, _notifywidget_on_timeout, widget);
	return widget;
}


/* notifywidget_delete */
static void _notifywidget_delete(NotifyWidget * widget)
{
	if(widget->source != 0)
		g_source_remove(widget->source);
	if(widget->widget != NULL)
		gtk_widget_destroy(widget->widget);
	free(widget);
}


/* callbacks */
static void _notifywidget_on_close(gpointer data)
{
	NotifyWidget * widget = data;

	_notify_widget_remove(widget->notify, widget);
}

static gboolean _notifywidget_on_timeout(gpointer data)
{
	NotifyWidget * widget = data;

	_notifywidget_on_close(widget);
	return FALSE;
}
