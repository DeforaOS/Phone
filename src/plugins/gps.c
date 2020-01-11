/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <string.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"
#include "hayes.h"


/* GPS */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	/* widgets */
	GtkWidget * window;
} GPS;


/* prototypes */
/* plugins */
static GPS * _gps_init(PhonePluginHelper * helper);
static void _gps_destroy(GPS * gps);
static void _gps_settings(GPS * gps);

static void _gps_queue(GPS * gps, char const * command);

/* callbacks */
static void _gps_on_settings_close(gpointer data);
static void _gps_on_settings_start(gpointer data);
static void _gps_on_settings_stop(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"GPS",
	"network-wireless",
	NULL,
	_gps_init,
	_gps_destroy,
	NULL,
	_gps_settings
};


/* private */
/* functions */
/* gps_init */
static GPS * _gps_init(PhonePluginHelper * helper)
{
	GPS * gps;

	if((gps = object_new(sizeof(*gps))) == NULL)
		return NULL;
	gps->helper = helper;
	gps->window = NULL;
	return gps;
}


/* gps_destroy */
static void _gps_destroy(GPS * gps)
{
	if(gps->window != NULL)
		gtk_widget_destroy(gps->window);
	object_delete(gps);
}


/* gps_settings */
static void _settings_window(GPS * gps);

static void _gps_settings(GPS * gps)
{
	if(gps->window == NULL)
		_settings_window(gps);
	gtk_window_present(GTK_WINDOW(gps->window));
}

static void _settings_window(GPS * gps)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;

	gps->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(gps->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(gps->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(gps->window), "gnome-settings");
#endif
	gtk_window_set_title(GTK_WINDOW(gps->window), "GPS");
	g_signal_connect(gps->window, "delete-event", G_CALLBACK(
				gtk_widget_hide), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	/* controls */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	widget = gtk_button_new_with_label("Start");
	g_signal_connect_swapped(widget, "clicked",
			G_CALLBACK(_gps_on_settings_start), gps);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	widget = gtk_button_new_with_label("Stop");
	g_signal_connect_swapped(widget, "clicked",
			G_CALLBACK(_gps_on_settings_stop), gps);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* button box */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_gps_on_settings_close), gps);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(gps->window), vbox);
	gtk_widget_show_all(vbox);
}


/* gps_queue */
static void _gps_queue(GPS * gps, char const * command)
{
	ModemRequest request;
	HayesRequest hrequest;

	request.type = MODEM_REQUEST_UNSUPPORTED;
	request.unsupported.modem = "Hayes";
	request.unsupported.request_type = HAYES_REQUEST_COMMAND_QUEUE;
	request.unsupported.request = &hrequest;
	request.unsupported.size = sizeof(hrequest);
	hrequest.command_queue.command = command;
	gps->helper->request(gps->helper->phone, &request);
}


/* callbacks */
/* gps_on_settings_close */
static void _gps_on_settings_close(gpointer data)
{
	GPS * gps = data;

	gtk_widget_hide(gps->window);
}


/* gps_on_settings_start */
static void _gps_on_settings_start(gpointer data)
{
	GPS * gps = data;

	_gps_queue(gps, "AT^WPDGP");
}


/* gps_on_settings_stop */
static void _gps_on_settings_stop(gpointer data)
{
	GPS * gps = data;

	_gps_queue(gps, "AT^WPDEND");
}
