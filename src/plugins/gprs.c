/* $Id$ */
/* Copyright (c) 2011-2018 Pierre Pronchery <khorben@defora.org> */
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
/* FIXME:
 * - check if it resets the modem on "OK" (and if so, avoid it) */



#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#ifdef PROGNAME_GPRS
# include <Desktop.h>
#endif
#include "Phone.h"
#include "../../config.h"
#define _(string) gettext(string)
#define N_(string) string

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef SYSCONFDIR
# define SYSCONFDIR	PREFIX "/etc"
#endif


/* GPRS */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	guint source;
	gboolean roaming;
	gboolean connected;
	size_t in;
	size_t out;
	size_t glin;
	size_t glout;
	char * _operator;

	gboolean active;
	GtkWidget * window;
	GtkWidget * notebook;
	GtkWidget * attach;
	GtkWidget * apn;
	GtkWidget * username;
	GtkWidget * password;
#ifndef PROGNAME_GPRS
	GtkWidget * defaults;
#endif
	GtkWidget * connect;
	GtkWidget * st_image;
	GtkWidget * st_label;
	GtkWidget * st_in;
	GtkWidget * st_out;
	GtkWidget * st_glin;
	GtkWidget * st_glout;
#if GTK_CHECK_VERSION(2, 10, 0)
	GtkWidget * systray;
	GtkStatusIcon * icon;
#endif
} GPRS;


/* prototypes */
/* plug-in */
static GPRS * _gprs_init(PhonePluginHelper * helper);
static void _gprs_destroy(GPRS * gprs);
static int _gprs_event(GPRS * gprs, PhoneEvent * event);
static void _gprs_settings(GPRS * gprs);

/* accessors */
static void _gprs_set_connected(GPRS * gprs, gboolean connected,
		char const * message, size_t in, size_t out);

/* useful */
static int _gprs_access_point(GPRS * gprs);
static int _gprs_connect(GPRS * gprs);

static void _gprs_counters_load(GPRS * gprs);
static void _gprs_counters_save(GPRS * gprs);

static int _gprs_disconnect(GPRS * gprs);

static int _gprs_load_defaults(GPRS * gprs);
static int _gprs_load_operator(GPRS * gprs, char const * _operator);

/* callbacks */
static void _gprs_on_activate(gpointer data);
#ifndef PROGNAME_GPRS
static void _gprs_on_load_defaults(gpointer data);
#endif
static void _gprs_on_popup_menu(GtkStatusIcon * icon, guint button,
		guint time, gpointer data);
static gboolean _gprs_on_timeout(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	N_("Dial-up networking"),
	"phone-gprs",
	NULL,
	_gprs_init,
	_gprs_destroy,
	_gprs_event,
	_gprs_settings
};


/* private */
/* functions */
/* plug-in */
/* gprs_init */
static GPRS * _gprs_init(PhonePluginHelper * helper)
{
	GPRS * gprs;
#if GTK_CHECK_VERSION(2, 10, 0)
	char const * p;
	gboolean active;
#endif

	if((gprs = object_new(sizeof(*gprs))) == NULL)
		return NULL;
	gprs->helper = helper;
	gprs->source = 0;
	gprs->roaming = FALSE;
	gprs->connected = FALSE;
	gprs->in = 0;
	gprs->out = 0;
	gprs->glin = 0;
	gprs->glout = 0;
	gprs->_operator = NULL;
	gprs->active = FALSE;
	gprs->window = NULL;
#if GTK_CHECK_VERSION(2, 10, 0)
	gprs->icon = gtk_status_icon_new_from_icon_name("phone-gprs");
# if GTK_CHECK_VERSION(2, 16, 0)
	gtk_status_icon_set_tooltip_text(gprs->icon, _("Not connected"));
# endif
# if GTK_CHECK_VERSION(2, 18, 0)
	gtk_status_icon_set_title(gprs->icon, _(plugin.name));
#  if GTK_CHECK_VERSION(2, 20, 0)
	gtk_status_icon_set_name(gprs->icon, "phone-gprs");
#  endif
# endif
	g_signal_connect_swapped(gprs->icon, "activate", G_CALLBACK(
				_gprs_on_activate), gprs);
	g_signal_connect(gprs->icon, "popup-menu", G_CALLBACK(
				_gprs_on_popup_menu), gprs);
	active = ((p = helper->config_get(helper->phone, "gprs", "systray"))
			!= NULL && strtoul(p, NULL, 10) != 0) ? TRUE : FALSE;
	gtk_status_icon_set_visible(gprs->icon, active);
#endif
	_gprs_counters_load(gprs);
	return gprs;
}


/* gprs_destroy */
static void _gprs_destroy(GPRS * gprs)
{
	free(gprs->_operator);
	_gprs_counters_save(gprs);
#if GTK_CHECK_VERSION(2, 10, 0)
	g_object_unref(gprs->icon);
#endif
	if(gprs->source != 0)
		g_source_remove(gprs->source);
	if(gprs->window != NULL)
		gtk_widget_destroy(gprs->window);
	object_delete(gprs);
}


/* gprs_event */
static int _gprs_event_modem(GPRS * gprs, ModemEvent * event);
static void _gprs_event_modem_operator(GPRS * gprs, char const * _operator);

static int _gprs_event(GPRS * gprs, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _gprs_event_modem(gprs,
					event->modem_event.event);
		case PHONE_EVENT_TYPE_OFFLINE:
		case PHONE_EVENT_TYPE_UNAVAILABLE:
			gprs->roaming = FALSE;
			return 0;
		default: /* not relevant */
			return 0;
	}
}

static int _gprs_event_modem(GPRS * gprs, ModemEvent * event)
{
	gboolean connected;

	switch(event->type)
	{
		case MODEM_EVENT_TYPE_CONNECTION:
			connected = event->connection.connected;
			_gprs_set_connected(gprs, connected, NULL,
					event->connection.in,
					event->connection.out);
			break;
		case MODEM_EVENT_TYPE_REGISTRATION:
			/* operator */
			_gprs_event_modem_operator(gprs,
					event->registration._operator);
			/* roaming */
			gprs->roaming = event->registration.roaming;
			/* status */
			if(gprs->active != FALSE)
				break;
			if(event->registration.status
					!= MODEM_REGISTRATION_STATUS_REGISTERED)
				break;
			gprs->active = TRUE;
			/* FIXME optionally force GPRS registration */
			break;
		default:
			break;
	}
	return 0;
}

static void _gprs_event_modem_operator(GPRS * gprs, char const * _operator)
{
	PhonePluginHelper * helper = gprs->helper;
	char const * p;

	free(gprs->_operator);
	gprs->_operator = (_operator != NULL) ? strdup(_operator) : NULL;
	if(gprs->window == NULL)
		return;
#ifndef PROGNAME_GPRS
	gtk_widget_set_sensitive(gprs->defaults, (gprs->_operator != NULL)
			? TRUE : FALSE);
#endif
	/* FIXME also load the defaults when creating the window if relevant */
	if(((p = gtk_entry_get_text(GTK_ENTRY(gprs->apn))) == NULL
				|| strlen(p) == 0)
			&& ((p = gtk_entry_get_text(GTK_ENTRY(gprs->username)))
				== NULL || strlen(p) == 0)
			&& ((p = gtk_entry_get_text(GTK_ENTRY(gprs->password)))
				== NULL || strlen(p) == 0)
			&& helper->config_get(helper->phone, "gprs", "apn")
			== NULL
			&& helper->config_get(helper->phone, "gprs", "username")
			== NULL
			&& helper->config_get(helper->phone, "gprs", "password")
			== NULL)
		_gprs_load_defaults(gprs);
}


/* gprs_settings */
static GtkWidget * _settings_preferences(GPRS * gprs);
static GtkWidget * _settings_status(GPRS * gprs);
/* callbacks */
static void _settings_on_apply(gpointer data);
static void _settings_on_cancel(gpointer data);
static gboolean _settings_on_closex(gpointer data);
static void _settings_on_connect(gpointer data);
#ifdef PROGNAME_GPRS
static void _settings_on_help(gpointer data);
#endif
static void _settings_on_ok(gpointer data);
static void _settings_on_reset(gpointer data);

static void _gprs_settings(GPRS * gprs)
{
	GtkWidget * vbox;
	GtkWidget * bbox;
	GtkWidget * widget;

	if(gprs->window != NULL)
	{
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gprs->notebook), 0);
		gtk_window_present(GTK_WINDOW(gprs->window));
		return;
	}
	gprs->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(gprs->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(gprs->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(gprs->window), "phone-gprs");
#endif
	gtk_window_set_title(GTK_WINDOW(gprs->window), _(plugin.name));
	g_signal_connect_swapped(gprs->window, "delete-event", G_CALLBACK(
				_settings_on_closex), gprs);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	gprs->notebook = gtk_notebook_new();
	/* preferences */
	widget = _settings_preferences(gprs);
	gtk_notebook_append_page(GTK_NOTEBOOK(gprs->notebook), widget,
			gtk_label_new(_("Preferences")));
	/* status */
	widget = _settings_status(gprs);
	gtk_notebook_append_page(GTK_NOTEBOOK(gprs->notebook), widget,
			gtk_label_new(_("Status")));
	gtk_box_pack_start(GTK_BOX(vbox), gprs->notebook, TRUE, TRUE, 0);
	/* button box */
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	bbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 4);
#ifdef PROGNAME_GPRS
	widget = gtk_button_new_from_stock(GTK_STOCK_HELP);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_settings_on_help), gprs);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
#endif
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_settings_on_cancel), gprs);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_settings_on_ok),
			gprs);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(gprs->window), vbox);
	_settings_on_cancel(gprs);
	_gprs_on_timeout(gprs);
	gtk_widget_show_all(gprs->window);
}

static GtkWidget * _settings_preferences(GPRS * gprs)
{
	GtkSizeGroup * group;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * frame;
	GtkWidget * vbox2;
	GtkWidget * widget;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	/* attachment */
	gprs->attach = gtk_check_button_new_with_label(
			_("Force GPRS registration"));
	gtk_box_pack_start(GTK_BOX(vbox), gprs->attach, FALSE, TRUE, 0);
	/* systray */
	gprs->systray = gtk_check_button_new_with_label(
			_("Show in system tray"));
	gtk_box_pack_start(GTK_BOX(vbox), gprs->systray, FALSE, TRUE, 0);
	/* credentials */
	frame = gtk_frame_new(_("Credentials"));
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox2 = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/* access point */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Access point:"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gprs->apn = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), gprs->apn, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* username */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Username:"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gprs->username = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), gprs->username, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* password */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Password:"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gprs->password = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(gprs->password), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), gprs->password, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
#ifndef PROGNAME_GPRS
	/* defaults */
# if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
# else
	hbox = gtk_hbox_new(FALSE, 4);
# endif
	gprs->defaults = gtk_button_new_with_label(_("Load defaults"));
	gtk_widget_set_sensitive(gprs->defaults, (gprs->_operator != NULL)
			? TRUE : FALSE);
	g_signal_connect_swapped(gprs->defaults, "clicked", G_CALLBACK(
				_gprs_on_load_defaults), gprs);
	gtk_box_pack_end(GTK_BOX(hbox), gprs->defaults, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
#endif
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	return vbox;
}

static GtkWidget * _settings_status(GPRS * gprs)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkWidget * bbox;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	/* details */
	widget = gtk_frame_new(_("Details"));
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	bbox = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(bbox), 4);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	gprs->st_image = gtk_image_new_from_icon_name(GTK_STOCK_DISCONNECT,
			GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(hbox), gprs->st_image, FALSE, TRUE, 0);
	gprs->st_label = gtk_label_new(_("Not connected"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(gprs->st_label, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(gprs->st_label), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gprs->st_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bbox), hbox, FALSE, TRUE, 0);
	gprs->st_in = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(gprs->st_in, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(gprs->st_in), 0.0, 0.5);
#endif
	gtk_widget_set_no_show_all(gprs->st_in, TRUE);
	gtk_box_pack_start(GTK_BOX(bbox), gprs->st_in, FALSE, TRUE, 0);
	gprs->st_out = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(gprs->st_out, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(gprs->st_out), 0.0, 0.5);
#endif
	gtk_widget_set_no_show_all(gprs->st_out, TRUE);
	gtk_box_pack_start(GTK_BOX(bbox), gprs->st_out, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(widget), bbox);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* connect */
	gprs->connect = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
	g_signal_connect_swapped(gprs->connect, "clicked", G_CALLBACK(
				_settings_on_connect), gprs);
	gtk_box_pack_start(GTK_BOX(vbox), gprs->connect, FALSE, TRUE, 0);
	/* counters */
	widget = gtk_frame_new(_("Counters"));
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	hbox = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gprs->st_glin = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(gprs->st_glin, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(gprs->st_glin), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gprs->st_glin, FALSE, TRUE, 0);
	gprs->st_glout = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(gprs->st_glout, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(gprs->st_glout), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gprs->st_glout, FALSE, TRUE, 0);
	bbox = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	g_signal_connect_swapped(bbox, "clicked", G_CALLBACK(
				_settings_on_reset), gprs);
	gtk_box_pack_start(GTK_BOX(hbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(widget), hbox);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	return vbox;
}

static void _settings_on_apply(gpointer data)
{
	GPRS * gprs = data;
	PhonePluginHelper * helper = gprs->helper;
	gboolean active;
	char const * p;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gprs->attach));
	helper->config_set(helper->phone, "gprs", "attach", active ? "1" : "0");
	p = gtk_entry_get_text(GTK_ENTRY(gprs->apn));
	helper->config_set(helper->phone, "gprs", "apn", p);
	p = gtk_entry_get_text(GTK_ENTRY(gprs->username));
	helper->config_set(helper->phone, "gprs", "username", p);
	p = gtk_entry_get_text(GTK_ENTRY(gprs->password));
	helper->config_set(helper->phone, "gprs", "password", p);
#if GTK_CHECK_VERSION(2, 10, 0)
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gprs->systray));
	helper->config_set(helper->phone, "gprs", "systray", active
			? "1" : "0");
	gtk_status_icon_set_visible(gprs->icon, active);
#endif
	_gprs_access_point(gprs);
	_gprs_counters_save(gprs);
	gprs->active = FALSE;
}

static void _settings_on_cancel(gpointer data)
{
	GPRS * gprs = data;
	PhonePluginHelper * helper = gprs->helper;
	char const * p;
	gboolean active;

	gtk_widget_hide(gprs->window);
	active = ((p = helper->config_get(helper->phone, "gprs", "attach"))
			!= NULL && strtoul(p, NULL, 10) != 0) ? TRUE : FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gprs->attach), active);
	if((p = helper->config_get(helper->phone, "gprs", "apn")) == NULL)
		p = "";
	gtk_entry_set_text(GTK_ENTRY(gprs->apn), p);
	if((p = helper->config_get(helper->phone, "gprs", "username")) == NULL)
		p = "";
	gtk_entry_set_text(GTK_ENTRY(gprs->username), p);
	if((p = helper->config_get(helper->phone, "gprs", "password")) == NULL)
		p = "";
	gtk_entry_set_text(GTK_ENTRY(gprs->password), p);
#if GTK_CHECK_VERSION(2, 10, 0)
	active = ((p = helper->config_get(helper->phone, "gprs", "systray"))
			!= NULL && strtoul(p, NULL, 10) != 0) ? TRUE : FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gprs->systray), active);
#endif
	_gprs_set_connected(gprs, gprs->connected, NULL, gprs->in, gprs->out);
}

static gboolean _settings_on_closex(gpointer data)
{
	GPRS * gprs = data;

	_settings_on_cancel(gprs);
	return TRUE;
}

static void _settings_on_connect(gpointer data)
{
	GPRS * gprs = data;
	int res;

	_settings_on_apply(gprs);
	res = gprs->connected ? _gprs_disconnect(gprs) : _gprs_connect(gprs);
	if(res != 0)
		gprs->helper->error(gprs->helper->phone, error_get(NULL), 1);
}

#ifdef PROGNAME_GPRS
static void _settings_on_help(gpointer data)
{
	desktop_help_contents(PACKAGE, PROGNAME_GPRS);
}
#endif

static void _settings_on_ok(gpointer data)
{
	GPRS * gprs = data;

	gtk_widget_hide(gprs->window);
	_settings_on_apply(gprs);
}

static void _settings_on_reset(gpointer data)
{
	GPRS * gprs = data;

	gprs->glin = 0;
	gprs->glout = 0;
	/* refresh the dialog box */
	_gprs_set_connected(gprs, gprs->connected, NULL, gprs->in, gprs->out);
	/* save in the configuration */
	_gprs_counters_save(gprs);
}


/* accessors */
/* gprs_set_connected */
static void _gprs_set_connected(GPRS * gprs, gboolean connected,
		char const * message, size_t in, size_t out)
{
	char buf[64];

	gprs->connected = connected;
	if(message == NULL)
		message = connected ? _("Connected") : _("Not connected");
	if(gprs->window == NULL)
		return;
	gtk_image_set_from_icon_name(GTK_IMAGE(gprs->st_image), connected
			? GTK_STOCK_CONNECT : GTK_STOCK_DISCONNECT,
			GTK_ICON_SIZE_BUTTON);
	gtk_label_set_text(GTK_LABEL(gprs->st_label), message);
	gtk_button_set_label(GTK_BUTTON(gprs->connect), connected
			? GTK_STOCK_DISCONNECT : GTK_STOCK_CONNECT);
	if(connected)
	{
		snprintf(buf, sizeof(buf), _("Received: %zu kB (%zu kB/s)"),
				in / 1024, (in - gprs->in) / 1024);
		gtk_label_set_text(GTK_LABEL(gprs->st_in), buf);
		snprintf(buf, sizeof(buf), _("Sent: %zu kB (%zu kB/s)"),
				out / 1024, (out - gprs->out) / 1024);
		gtk_label_set_text(GTK_LABEL(gprs->st_out), buf);
		gtk_widget_show(gprs->st_in);
		gtk_widget_show(gprs->st_out);
#if GTK_CHECK_VERSION(2, 16, 0)
		snprintf(buf, sizeof(buf),
				_("%s\nReceived: %zu kB\nSent: %zu kB"),
				message, in / 1024, out / 1024);
		gtk_status_icon_set_tooltip_text(gprs->icon, buf);
#endif
		gprs->in = in;
		gprs->out = out;
		if(gprs->source == 0)
			gprs->source = g_timeout_add(1000, _gprs_on_timeout,
					gprs);
	}
	else
	{
		if(gprs->source != 0)
			g_source_remove(gprs->source);
		gprs->source = 0;
		gtk_widget_hide(gprs->st_in);
		gtk_widget_hide(gprs->st_out);
#if GTK_CHECK_VERSION(2, 16, 0)
		gtk_status_icon_set_tooltip_text(gprs->icon, message);
#endif
		/* add the last known data to the global counters */
		gprs->glin += gprs->in;
		gprs->glout += gprs->out;
		/* save in the configuration */
		_gprs_counters_save(gprs);
		/* reset the known data */
		gprs->in = 0;
		gprs->out = 0;
	}
	/* counters */
	snprintf(buf, sizeof(buf), _("Received: %zu kB"),
			(gprs->glin + in) / 1024);
	gtk_label_set_text(GTK_LABEL(gprs->st_glin), buf);
	snprintf(buf, sizeof(buf), _("Sent: %zu kB"),
			(gprs->glout + out) / 1024);
	gtk_label_set_text(GTK_LABEL(gprs->st_glout), buf);
}


/* useful */
/* gprs_access_point */
static int _gprs_access_point(GPRS * gprs)
{
	int ret = 0;
	PhonePluginHelper * helper = gprs->helper;
	char const * p;
	ModemRequest request;

	if((p = helper->config_get(helper->phone, "gprs", "apn")) == NULL)
		return 0;
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_AUTHENTICATE;
	/* set the access point */
	request.authenticate.name = "APN";
	request.authenticate.username = "IP";
	request.authenticate.password = p;
	ret |= helper->request(helper->phone, &request);
	/* set the credentials */
	request.authenticate.name = "GPRS";
	p = helper->config_get(helper->phone, "gprs", "username");
	request.authenticate.username = p;
	p = helper->config_get(helper->phone, "gprs", "password");
	request.authenticate.password = p;
	ret |= helper->request(helper->phone, &request);
	return ret;
}


/* gprs_connect */
static int _gprs_connect(GPRS * gprs)
{
	GtkDialogFlags flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;
	char const message[] = N_("You are currently roaming, and additional"
		" charges are therefore likely to apply.\n"
		"Do you really want to connect?");
	GtkWidget * widget;
	int res;
	ModemRequest request;

	if(_gprs_access_point(gprs) != 0)
		return -1;
	if(gprs->roaming)
	{
		widget = gtk_message_dialog_new(GTK_WINDOW(gprs->window), flags,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
				"%s", _("Warning"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					widget),
#endif
				"%s", _(message));
		gtk_window_set_title(GTK_WINDOW(widget), _("Warning"));
		res = gtk_dialog_run(GTK_DIALOG(widget));
		gtk_widget_destroy(widget);
		if(res != GTK_RESPONSE_YES)
			return 0;
	}
	_gprs_set_connected(gprs, TRUE, _("Connecting..."), 0, 0);
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_CALL;
	request.call.call_type = MODEM_CALL_TYPE_DATA;
	if(gprs->helper->request(gprs->helper->phone, &request) != 0)
		return -gprs->helper->error(gprs->helper->phone,
				error_get(NULL), 1);
	return 0;
}


/* gprs_counters_load */
static void _gprs_counters_load(GPRS * gprs)
{
	PhonePluginHelper * helper = gprs->helper;
	char const * p;

	gprs->glin = 0;
	if((p = helper->config_get(helper->phone, "gprs", "in")) != NULL)
		gprs->glin = strtol(p, NULL, 10);
	gprs->glout = 0;
	if((p = helper->config_get(helper->phone, "gprs", "out")) != NULL)
		gprs->glout = strtol(p, NULL, 10);
}


/* gprs_counters_save */
static void _gprs_counters_save(GPRS * gprs)
{
	PhonePluginHelper * helper = gprs->helper;
	char buf[16];

	snprintf(buf, sizeof(buf), "%lu", gprs->glin);
	helper->config_set(helper->phone, "gprs", "in", buf);
	snprintf(buf, sizeof(buf), "%lu", gprs->glout);
	helper->config_set(helper->phone, "gprs", "out", buf);
}


/* gprs_disconnect */
static int _gprs_disconnect(GPRS * gprs)
{
	ModemRequest request;

	if(_gprs_access_point(gprs) != 0)
		return -1;
	_gprs_set_connected(gprs, TRUE, _("Disconnecting..."), 0, 0);
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_CALL_HANGUP;
	return gprs->helper->request(gprs->helper->phone, &request);
}


/* gprs_load_defaults */
static int _gprs_load_defaults(GPRS * gprs)
{
	return _gprs_load_operator(gprs, gprs->_operator);
}


/* gprs_load_operator */
static int _gprs_load_operator(GPRS * gprs, char const * _operator)
{
	Config * config;
	String const * p;

	if((config = config_new()) == NULL)
		return -1;
	if(config_load(config, SYSCONFDIR "/" PACKAGE "/gprs.conf") != 0
			|| (p = config_get(config, _operator, "apn")) == NULL)
	{
		config_delete(config);
		return -1;
	}
	gtk_entry_set_text(GTK_ENTRY(gprs->apn), p);
	if((p = config_get(config, _operator, "username")) == NULL)
		p = "";
	gtk_entry_set_text(GTK_ENTRY(gprs->username), p);
	if((p = config_get(config, _operator, "password")) == NULL)
		p = "";
	gtk_entry_set_text(GTK_ENTRY(gprs->password), p);
	config_delete(config);
	return 0;
}


/* callbacks */
/* gprs_on_activate */
static void _gprs_on_activate(gpointer data)
{
	GPRS * gprs = data;

	_gprs_settings(gprs);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gprs->notebook), 1);
	gtk_window_present(GTK_WINDOW(gprs->window));
}


#ifndef PROGNAME_GPRS
/* gprs_on_load_defaults */
static void _gprs_on_load_defaults(gpointer data)
{
	GPRS * gprs = data;
	GtkWidget * widget;
	const GtkDialogFlags flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;

	if(_gprs_load_defaults(gprs) == 0)
		return;
	widget = gtk_message_dialog_new(GTK_WINDOW(gprs->window),
			flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					widget),
# endif
			"%s", _("No defaults known for the current operator"));
	gtk_window_set_title(GTK_WINDOW(widget), _("Error"));
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}
#endif


/* gprs_on_popup_menu */
static void _gprs_on_popup_menu(GtkStatusIcon * icon, guint button,
		guint time, gpointer data)
{
	GPRS * gprs = data;
	GtkWidget * menu;
	GtkWidget * menuitem;
	GtkWidget * image;

	menu = gtk_menu_new();
	/* status */
	menuitem = gtk_image_menu_item_new_with_mnemonic("_Status");
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_gprs_on_activate), gprs);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* connection */
	menuitem = gtk_image_menu_item_new_with_mnemonic(gprs->connected
			? _("_Disconnect") : _("_Connect"));
	image = gtk_image_new_from_stock(gprs->connected ? GTK_STOCK_DISCONNECT
			: GTK_STOCK_CONNECT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				gprs->connected ? _gprs_disconnect
				: _gprs_connect), gprs);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* preferences */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,
			NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_gprs_settings), gprs);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#ifdef PROGNAME_GPRS
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* quit */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	g_signal_connect(menuitem, "activate", G_CALLBACK(gtk_main_quit), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#endif
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, time);
}


/* gprs_on_timeout */
static gboolean _gprs_on_timeout(gpointer data)
{
	GPRS * gprs = data;

	gprs->helper->trigger(gprs->helper->phone, MODEM_EVENT_TYPE_CONNECTION);
	return TRUE;
}
