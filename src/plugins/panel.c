/* $Id$ */
/* Copyright (c) 2011-2024 Pierre Pronchery <khorben@defora.org> */
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
/* FIXME:
 * - track battery and signal status only when online */



#include <System.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <gtk/gtk.h>
#if defined(GDK_WINDOWING_X11)
# if GTK_CHECK_VERSION(3, 0, 0)
#  include <gtk/gtkx.h>
# endif
#endif
#include <Desktop.h>
#include "Phone.h"


/* Panel */
/* private */
/* types */
typedef enum _PanelBattery
{
	PANEL_BATTERY_UNKNOWN = 0,
	PANEL_BATTERY_ERROR,
	PANEL_BATTERY_EMPTY,
	PANEL_BATTERY_CAUTION,
	PANEL_BATTERY_LOW,
	PANEL_BATTERY_GOOD,
	PANEL_BATTERY_FULL
} PanelBattery;
#define PANEL_BATTERY_LAST	PANEL_BATTERY_FULL
#define PANEL_BATTERY_COUNT	(PANEL_BATTERY_LAST + 1)

typedef enum _PanelSignal
{
	PANEL_SIGNAL_UNKNOWN,
	PANEL_SIGNAL_00,
	PANEL_SIGNAL_25,
	PANEL_SIGNAL_50,
	PANEL_SIGNAL_75,
	PANEL_SIGNAL_100
} PanelSignal;
#define PANEL_SIGNAL_LAST	PANEL_SIGNAL_100
#define PANEL_SIGNAL_COUNT	(PANEL_SIGNAL_LAST + 1)

typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

#if defined(GDK_WINDOWING_X11)
	guint timeout;
	GtkWidget * plug;
	GtkWidget * hbox;
	/* battery */
	PanelBattery battery_level;
	GtkWidget * battery_image;
	guint battery_timeout;
	/* connection status */
	GtkWidget * data;
	GtkWidget * roaming;
	/* signal */
	PanelSignal signal_level;
	GtkWidget * signal_image;
	/* operator */
	GtkWidget * operator;
	/* preferences */
	GtkWidget * window;
	GtkWidget * battery;
	GtkWidget * truncate;
#endif
} Panel;


/* prototypes */
static Panel * _panel_init(PhonePluginHelper * helper);
static void _panel_destroy(Panel * panel);
static int _panel_event(Panel * panel, PhoneEvent * event);
static void _panel_settings(Panel * panel);

#if defined(GDK_WINDOWING_X11)
static void _panel_set_battery_level(Panel * panel, gdouble level,
		gboolean charging);
static void _panel_set_operator(Panel * panel, ModemRegistrationStatus status,
		char const * _operator);
static void _panel_set_signal_level(Panel * panel, gdouble level);
static void _panel_set_status(Panel * panel, gboolean data, gboolean roaming);
#endif


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Panel",
	"gnome-monitor",
	NULL,
	_panel_init,
	_panel_destroy,
	_panel_event,
	_panel_settings
};


/* private */
/* functions */
/* panel_init */
#if defined(GDK_WINDOWING_X11)
static gboolean _on_plug_delete_event(gpointer data);
static void _on_plug_embedded(gpointer data);
static gboolean _on_battery_timeout(gpointer data);
#endif

static Panel * _panel_init(PhonePluginHelper * helper)
{
#if defined(GDK_WINDOWING_X11)
	Panel * panel;
	PangoFontDescription * bold;
	char const * p;

# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
# endif
	if((panel = object_new(sizeof(*panel))) == NULL)
		return NULL;
	panel->helper = helper;
	panel->timeout = 0;
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
	panel->plug = gtk_plug_new(0);
	g_signal_connect_swapped(panel->plug, "delete-event", G_CALLBACK(
				_on_plug_delete_event), panel);
	g_signal_connect_swapped(panel->plug, "embedded", G_CALLBACK(
				_on_plug_embedded), panel);
# if GTK_CHECK_VERSION(3, 0, 0)
	panel->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
# else
	panel->hbox = gtk_hbox_new(FALSE, 2);
# endif
	/* battery */
	panel->battery_timeout = 0;
	panel->battery_level = -1;
	panel->battery_image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(panel->hbox), panel->battery_image, FALSE,
			TRUE, 0);
	if((p = helper->config_get(helper->phone, "panel", "battery")) == NULL
			|| strtol(p, NULL, 10) == 0)
		gtk_widget_set_no_show_all(panel->battery_image, TRUE);
	else if(_on_battery_timeout(panel) == TRUE)
		panel->battery_timeout = g_timeout_add(5000,
				_on_battery_timeout, panel);
	/* signal */
	panel->signal_level = -1;
	panel->signal_image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(panel->hbox), panel->signal_image, FALSE,
			TRUE, 0);
	/* operator */
	panel->operator = gtk_label_new(NULL);
	if((p = helper->config_get(helper->phone, "panel", "truncate")) != NULL
			&& strtol(p, NULL, 10) != 0)
		gtk_label_set_ellipsize(GTK_LABEL(panel->operator),
				PANGO_ELLIPSIZE_END);
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_label_set_max_width_chars(GTK_LABEL(panel->operator), 12);
# endif
	gtk_widget_modify_font(panel->operator, bold);
	gtk_box_pack_start(GTK_BOX(panel->hbox), panel->operator, TRUE, TRUE,
			0);
	_panel_set_signal_level(panel, 0.0 / 0.0);
	/* connection status */
	panel->data = gtk_image_new_from_icon_name("stock_internet",
			GTK_ICON_SIZE_SMALL_TOOLBAR);
# if GTK_CHECK_VERSION(2, 12, 0)
	gtk_widget_set_tooltip_text(panel->data, "Connected to GPRS");
# endif
	gtk_widget_set_no_show_all(panel->data, TRUE);
	gtk_box_pack_start(GTK_BOX(panel->hbox), panel->data, FALSE, TRUE, 0);
	panel->roaming = gtk_image_new_from_icon_name("phone-roaming",
			GTK_ICON_SIZE_SMALL_TOOLBAR);
# if GTK_CHECK_VERSION(2, 12, 0)
	gtk_widget_set_tooltip_text(panel->roaming, "Roaming");
# endif
	gtk_widget_set_no_show_all(panel->roaming, TRUE);
	gtk_box_pack_start(GTK_BOX(panel->hbox), panel->roaming, FALSE, TRUE,
			0);
	gtk_container_add(GTK_CONTAINER(panel->plug), panel->hbox);
	gtk_widget_show_all(panel->hbox);
	/* preferences */
	panel->window = NULL;
	pango_font_description_free(bold);
	_on_plug_delete_event(panel);
	return panel;
#else
	(void) helper;

	error_set_code(-ENOSYS, "%s", "X11 support not detected");
	return NULL;
#endif
}

#if defined(GDK_WINDOWING_X11)
static gboolean _on_plug_delete_event(gpointer data)
{
	Panel * panel = data;

# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
# endif
	if(panel->timeout == 0)
		panel->timeout = g_timeout_add(5000, _on_plug_delete_event,
				panel);
	desktop_message_send(PHONE_EMBED_MESSAGE, gtk_plug_get_id(
				GTK_PLUG(panel->plug)), 0, 0);
	return TRUE;
}

static void _on_plug_embedded(gpointer data)
{
	Panel * panel = data;
	PhonePluginHelper * helper = panel->helper;

# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
# endif
	if(panel->timeout != 0)
		g_source_remove(panel->timeout);
	panel->timeout = 0;
	gtk_widget_show(panel->plug);
	helper->trigger(helper->phone, MODEM_EVENT_TYPE_REGISTRATION);
}

static gboolean _on_battery_timeout(gpointer data)
{
	Panel * panel = data;
	ModemRequest request;

	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_BATTERY_LEVEL;
	panel->helper->request(panel->helper->phone, &request);
	return TRUE;
}
#endif


/* panel_destroy */
static void _panel_destroy(Panel * panel)
{
#if defined(GDK_WINDOWING_X11)
	if(panel->battery_timeout != 0)
		g_source_remove(panel->battery_timeout);
	if(panel->timeout != 0)
		g_source_remove(panel->timeout);
	gtk_widget_destroy(panel->hbox);
	object_delete(panel);
#else
	(void) panel;
#endif
}


/* panel_event */
#if defined(GDK_WINDOWING_X11)
static int _event_modem_event(Panel * panel, ModemEvent * event);
#endif

static int _panel_event(Panel * panel, PhoneEvent * event)
{
#if defined(GDK_WINDOWING_X11)
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _event_modem_event(panel,
					event->modem_event.event);
		case PHONE_EVENT_TYPE_OFFLINE:
			_panel_set_operator(panel, -1, "Offline");
			_panel_set_signal_level(panel, 0.0 / 0.0);
			_panel_set_status(panel, FALSE, FALSE);
			break;
		case PHONE_EVENT_TYPE_STARTING:
			_panel_set_operator(panel, -1, "Connecting...");
			_panel_set_signal_level(panel, 0.0 / 0.0);
			_panel_set_status(panel, FALSE, FALSE);
			break;
		case PHONE_EVENT_TYPE_STOPPED:
			_panel_set_operator(panel, -1, "Disconnected");
			_panel_set_signal_level(panel, 0.0 / 0.0);
			_panel_set_status(panel, FALSE, FALSE);
			break;
		case PHONE_EVENT_TYPE_UNAVAILABLE:
			_panel_set_operator(panel, -1, "Unavailable");
			_panel_set_signal_level(panel, 0.0 / 0.0);
			_panel_set_status(panel, FALSE, FALSE);
			break;
		default:
			break;
	}
#else
	(void) panel;
	(void) event;

#endif
	return 0;
}

#if defined(GDK_WINDOWING_X11)
static int _event_modem_event(Panel * panel, ModemEvent * event)
{
	char const * media = "";
	gboolean data;

	switch(event->type)
	{
		case MODEM_EVENT_TYPE_BATTERY_LEVEL:
			_panel_set_battery_level(panel,
					event->battery_level.level,
					event->battery_level.charging);
			break;
		case MODEM_EVENT_TYPE_REGISTRATION:
			media = event->registration.media;
			data = (media != NULL && strcmp("GPRS", media) == 0)
				? TRUE : FALSE;
			_panel_set_operator(panel, event->registration.status,
					event->registration._operator);
			_panel_set_signal_level(panel,
					event->registration.signal);
			_panel_set_status(panel, data,
					event->registration.roaming);
			break;
		default:
			break;
	}
	return 0;
}
#endif


#if defined(GDK_WINDOWING_X11)
/* panel_set_battery_level */
static void _set_battery_image(Panel * panel, PanelBattery battery,
		gboolean charging);

static void _panel_set_battery_level(Panel * panel, gdouble level,
		gboolean charging)
{
# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(panel, %f)\n", __func__, level);
# endif
# if GTK_CHECK_VERSION(2, 12, 0)
	char buf[32];

	snprintf(buf, sizeof(buf), "%.0f%%%s", level * 100.0, charging
			? " (charging)" : "");
# endif
	if(level < 0.0)
	{
# if GTK_CHECK_VERSION(2, 12, 0)
		snprintf(buf, sizeof(buf), "%s", "Unknown status");
# endif
		_set_battery_image(panel, PANEL_BATTERY_UNKNOWN, charging);
	}
	else if(level <= 0.01)
		_set_battery_image(panel, PANEL_BATTERY_EMPTY, charging);
	else if(level <= 0.1)
		_set_battery_image(panel, PANEL_BATTERY_CAUTION, charging);
	else if(level <= 0.2)
		_set_battery_image(panel, PANEL_BATTERY_LOW, charging);
	else if(level <= 0.75)
		_set_battery_image(panel, PANEL_BATTERY_GOOD, charging);
	else if(level <= 1.0)
		_set_battery_image(panel, PANEL_BATTERY_FULL, charging);
	else
	{
# if GTK_CHECK_VERSION(2, 12, 0)
		snprintf(buf, sizeof(buf), "%s", "Error");
# endif
		_set_battery_image(panel, PANEL_BATTERY_ERROR, FALSE);
	}
# if GTK_CHECK_VERSION(2, 12, 0)
	gtk_widget_set_tooltip_text(panel->battery_image, buf);
# endif
}

static void _set_battery_image(Panel * panel, PanelBattery battery,
		gboolean charging)
{
	struct
	{
		char const * icon;
		char const * charging;
	} icons[PANEL_BATTERY_COUNT] =
	{
		{ "stock_dialog-question", "stock_dialog-question" },
		{ "battery-missing", "battery-missing" },
		{ "battery-empty", "battery-caution-charging" },
		{ "battery-caution", "battery-caution-charging" },
		{ "battery-low", "battery-low-charging" },
		{ "battery-good", "battery-good-charging" },
		{ "battery-full", "battery-full-charging" }
	};

	if(panel->battery_level == battery)
		return;
	if((panel->battery_level = battery) == PANEL_BATTERY_ERROR)
		gtk_widget_hide(panel->battery_image);
	else
	{
		/* XXX may not be the correct size */
		gtk_image_set_from_icon_name(GTK_IMAGE(panel->battery_image),
				charging ? icons[battery].charging
				: icons[battery].icon,
				GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_widget_show(panel->battery_image);
	}
}


/* panel_set_operator */
static void _panel_set_operator(Panel * panel, ModemRegistrationStatus status,
		char const * _operator)
{
	switch(status)
	{
		case MODEM_REGISTRATION_STATUS_DENIED:
			_operator = "Denied";
			break;
		case MODEM_REGISTRATION_STATUS_NOT_SEARCHING:
			_operator = "Not searching";
			break;
		case MODEM_REGISTRATION_STATUS_SEARCHING:
			_operator = "Searching...";
			break;
		case MODEM_REGISTRATION_STATUS_UNKNOWN:
			_operator = "Unknown";
			break;
		case MODEM_REGISTRATION_STATUS_REGISTERED:
			if(_operator == NULL)
				_operator = "Registered";
		default:
			break;
	}
	gtk_label_set_text(GTK_LABEL(panel->operator), _operator);
}


/* panel_set_signal_level */
static void _signal_level_set_image(Panel * panel, PanelSignal signal);

static void _panel_set_signal_level(Panel * panel, gdouble level)
{
	if(level <= 0.0)
		_signal_level_set_image(panel, PANEL_SIGNAL_00);
	else if(level < 0.25)
		_signal_level_set_image(panel, PANEL_SIGNAL_25);
	else if(level < 0.50)
		_signal_level_set_image(panel, PANEL_SIGNAL_50);
	else if(level < 0.75)
		_signal_level_set_image(panel, PANEL_SIGNAL_75);
	else if(level <= 1.0)
		_signal_level_set_image(panel, PANEL_SIGNAL_100);
	else
		_signal_level_set_image(panel, PANEL_SIGNAL_UNKNOWN);
}

static void _signal_level_set_image(Panel * panel, PanelSignal signal)
{
	char const * icons[PANEL_SIGNAL_COUNT] =
	{
		"network-wireless",
		"phone-signal-00",
		"phone-signal-25",
		"phone-signal-50",
		"phone-signal-75",
		"phone-signal-100"
	};

	if(panel->signal_level == signal)
		return;
	panel->signal_level = signal;
	/* XXX may not be the correct size */
	gtk_image_set_from_icon_name(GTK_IMAGE(panel->signal_image),
			icons[signal], GTK_ICON_SIZE_SMALL_TOOLBAR);
}


/* panel_set_status */
static void _panel_set_status(Panel * panel, gboolean data, gboolean roaming)
{
	if(data)
		gtk_widget_show(panel->data);
	else
		gtk_widget_hide(panel->data);
	if(roaming)
		gtk_widget_show(panel->roaming);
	else
		gtk_widget_hide(panel->roaming);
}
#endif


/* panel_settings */
#if defined(GDK_WINDOWING_X11)
static void _on_settings_cancel(gpointer data);
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_ok(gpointer data);
#endif

static void _panel_settings(Panel * panel)
{
#if defined(GDK_WINDOWING_X11)
	GtkWidget * vbox;
	GtkWidget * bbox;
	GtkWidget * widget;

	if(panel->window != NULL)
	{
		gtk_window_present(GTK_WINDOW(panel->window));
		return;
	}
	panel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(panel->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(panel->window), 200, 300);
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(panel->window), "gnome-settings");
# endif
	gtk_window_set_title(GTK_WINDOW(panel->window), "Panel preferences");
	g_signal_connect_swapped(panel->window, "delete-event", G_CALLBACK(
				_on_settings_closex), panel);
# if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
# else
	vbox = gtk_vbox_new(FALSE, 0);
# endif
	/* check button */
	panel->battery = gtk_check_button_new_with_label(
			"Monitor battery activity");
	gtk_box_pack_start(GTK_BOX(vbox), panel->battery, FALSE, TRUE, 0);
	panel->truncate = gtk_check_button_new_with_label(
			"Shorten the operator name");
	gtk_box_pack_start(GTK_BOX(vbox), panel->truncate, FALSE, TRUE, 0);
	/* button box */
# if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
# else
	bbox = gtk_hbutton_box_new();
# endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_settings_cancel), panel);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_on_settings_ok),
			panel);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(panel->window), vbox);
	_on_settings_cancel(panel);
	gtk_widget_show_all(panel->window);
#else
	(void) panel;

#endif
}

#if defined(GDK_WINDOWING_X11)
static void _on_settings_cancel(gpointer data)
{
	Panel * panel = data;
	PhonePluginHelper * helper = panel->helper;
	char const * p;
	gboolean active;

	active = ((p = helper->config_get(helper->phone, "panel", "battery"))
			!= NULL && strtoul(p, NULL, 10) != 0) ? TRUE : FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel->battery), active);
	active = ((p = helper->config_get(helper->phone, "panel", "truncate"))
			!= NULL && strtoul(p, NULL, 10) != 0) ? TRUE : FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel->truncate),
			active);
	gtk_widget_hide(panel->window);
}

static gboolean _on_settings_closex(gpointer data)
{
	Panel * panel = data;

	gtk_widget_hide(panel->window);
	return TRUE;
}

static void _on_settings_ok(gpointer data)
{
	Panel * panel = data;
	PhonePluginHelper * helper = panel->helper;
	gboolean value;

	gtk_widget_hide(panel->window);
	/* battery */
	if((value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						panel->battery))) == TRUE)
	{
		if(panel->battery_timeout == 0)
			panel->battery_timeout = g_timeout_add(5000,
					_on_battery_timeout, panel);
		_on_battery_timeout(panel);
		gtk_widget_show(panel->battery_image);
	}
	else
	{
		gtk_widget_hide(panel->battery_image);
		if(panel->battery_timeout != 0)
			g_source_remove(panel->battery_timeout);
		panel->battery_timeout = 0;
	}
	gtk_widget_set_no_show_all(panel->battery_image, !value);
	helper->config_set(helper->phone, "panel", "battery", value
			? "1" : "0");
	/* truncate */
	value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				panel->truncate));
	gtk_label_set_ellipsize(GTK_LABEL(panel->operator), value
			? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_NONE);
	helper->config_set(helper->phone, "panel", "truncate", value
			? "1" : "0");
}
#endif
