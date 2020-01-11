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



#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"
#include "../../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef SYSCONFDIR
# define SYSCONFDIR	PREFIX "/etc"
#endif


/* USSD */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	Config * config;
	char * _operator;
	/* widgets */
	GtkWidget * window;
	GtkWidget * operators;
	GtkWidget * codes;
} USSD;

typedef enum _USSDOperator
{
	UO_OPERATOR = 0,
	UO_DISPLAY
} USSDOperator;
#define UO_LAST UO_DISPLAY
#define UO_COUNT (UO_LAST + 1)

typedef enum _USSDCode
{
	UC_CODE = 0,
	UC_DISPLAY
} USSDCode;
#define UC_LAST UC_DISPLAY
#define UC_COUNT (UC_LAST + 1)


/* prototypes */
/* plugins */
static USSD * _ussd_init(PhonePluginHelper * helper);
static void _ussd_destroy(USSD * ussd);
static int _ussd_event(USSD * ussd, PhoneEvent * event);
static void _ussd_settings(USSD * ussd);

/* useful */
static int _ussd_load_operator(USSD * ussd, char const * name);

/* callbacks */
static void _ussd_on_operators_changed(gpointer data);
static void _ussd_on_settings_close(gpointer data);
static void _ussd_on_settings_send(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"USSD",
	"user-info",
	NULL,
	_ussd_init,
	_ussd_destroy,
	_ussd_event,
	_ussd_settings
};


/* private */
/* functions */
/* ussd_init */
static USSD * _ussd_init(PhonePluginHelper * helper)
{
	USSD * ussd;

	if((ussd = object_new(sizeof(*ussd))) == NULL)
		return NULL;
	ussd->config = config_new();
	ussd->_operator = NULL;
	ussd->helper = helper;
	ussd->window = NULL;
	ussd->operators = NULL;
	ussd->codes = NULL;
	/* check for errors */
	if(ussd->config == NULL || config_load(ussd->config,
				SYSCONFDIR "/" PACKAGE "/ussd.conf") != 0)
		helper->error(helper->phone, error_get(NULL), 1);
	return ussd;
}


/* ussd_destroy */
static void _ussd_destroy(USSD * ussd)
{
	if(ussd->window != NULL)
		gtk_widget_destroy(ussd->window);
	free(ussd->_operator);
	if(ussd->config != NULL)
		config_delete(ussd->config);
	object_delete(ussd);
}


/* ussd_event */
static int _event_modem(USSD * ussd, ModemEvent * event);

static int _ussd_event(USSD * ussd, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _event_modem(ussd, event->modem_event.event);
		default:
			/* not relevant */
			return 0;
	}
}

static int _event_modem(USSD * ussd, ModemEvent * event)
{
	switch(event->type)
	{
		case MODEM_EVENT_TYPE_REGISTRATION:
			_ussd_load_operator(ussd,
					event->registration._operator);
			/* XXX ignore errors */
			return 0;
		default:
			/* not relevant */
			return 0;
	}
}


/* ussd_settings */
static void _settings_window(USSD * ussd);
static void _settings_window_operators(String const * section, void * data);

static void _ussd_settings(USSD * ussd)
{
	if(ussd->window == NULL)
		_settings_window(ussd);
	gtk_window_present(GTK_WINDOW(ussd->window));
}

static void _settings_window(USSD * ussd)
{
	GtkSizeGroup * group;
	GtkListStore * model;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * image;
	GtkWidget * widget;
	GtkCellRenderer * renderer;

	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	ussd->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(ussd->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(ussd->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(ussd->window), "gnome-settings");
#endif
	gtk_window_set_title(GTK_WINDOW(ussd->window), "USSD");
	g_signal_connect(ussd->window, "delete-event", G_CALLBACK(
				gtk_widget_hide), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	/* operators */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Operator:");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	model = gtk_list_store_new(UO_COUNT, G_TYPE_STRING, G_TYPE_STRING);
	ussd->operators = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ussd->operators), renderer,
			TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ussd->operators),
			renderer, "text", UO_DISPLAY, NULL);
	g_signal_connect_swapped(ussd->operators, "changed", G_CALLBACK(
				_ussd_on_operators_changed), ussd);
	gtk_box_pack_start(GTK_BOX(hbox), ussd->operators, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* codes */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Code:");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	model = gtk_list_store_new(UC_COUNT, G_TYPE_STRING, G_TYPE_STRING);
	ussd->codes = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ussd->codes), renderer,
			TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ussd->codes), renderer,
			"text", UC_DISPLAY, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), ussd->codes, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* send */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_button_new_with_label("Send request");
	image = gtk_image_new_from_icon_name("mail-send", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget), image);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_ussd_on_settings_send), ussd);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
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
				_ussd_on_settings_close), ussd);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ussd->window), vbox);
	if(ussd->config != NULL)
		config_foreach(ussd->config, _settings_window_operators, ussd);
	gtk_widget_show_all(vbox);
}

static void _settings_window_operators(String const * section, void * data)
{
	USSD * ussd = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	String const * name;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->operators));
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	name = config_get(ussd->config, section, "name");
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, UO_OPERATOR, section,
			UO_DISPLAY, (name != NULL) ? name : section, -1);
	if(ussd->_operator != NULL && strcmp(ussd->_operator, section) == 0)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ussd->operators),
				&iter);
}


/* useful */
/* ussd_load_operator */
static int _ussd_load_operator(USSD * ussd, char const * name)
{
	GtkTreeModel * model;
	GtkTreeIter iter;
	gboolean valid;
	gchar * _operator;

	if(name == NULL)
		return -1;
	if(ussd->_operator != NULL && strcmp(ussd->_operator, name) == 0)
		return 0;
	free(ussd->_operator);
	if((ussd->_operator = strdup(name)) == NULL)
		return -1;
	if(ussd->operators == NULL)
		return 0;
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->operators));
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, UO_OPERATOR, &_operator, -1);
		valid = (strcmp(name, _operator) == 0) ? TRUE : FALSE;
		g_free(_operator);
		if(valid)
		{
			gtk_combo_box_set_active_iter(
					GTK_COMBO_BOX(ussd->operators), &iter);
			break;
		}
	}
	return 0;
}


/* callbacks */
/* ussd_on_operators_changed */
static void _ussd_on_operators_changed_operator(char const * variable,
		char const * value, void * data);

static void _ussd_on_operators_changed(gpointer data)
{
	USSD * ussd = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	gchar * _operator;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->codes));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ussd->operators), &iter)
			!= TRUE)
		return;
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->operators));
	gtk_tree_model_get(model, &iter, UO_OPERATOR, &_operator, -1);
	if(ussd->config != NULL)
		config_foreach_section(ussd->config, _operator,
				_ussd_on_operators_changed_operator, ussd);
	g_free(_operator);
	gtk_combo_box_set_active(GTK_COMBO_BOX(ussd->codes), 0);
}

static void _ussd_on_operators_changed_operator(char const * variable,
		char const * value, void * data)
{
	USSD * ussd = data;
	GtkTreeModel * model;
	GtkTreeIter iter;

	if(strcmp(variable, "name") == 0)
		return;
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->codes));
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, UC_CODE, variable,
			UC_DISPLAY, value, -1);
}


/* ussd_on_settings_close */
static void _ussd_on_settings_close(gpointer data)
{
	USSD * ussd = data;

	gtk_widget_hide(ussd->window);
}


/* ussd_on_settings_send */
static void _ussd_on_settings_send(gpointer data)
{
	USSD * ussd = data;
	PhonePluginHelper * helper = ussd->helper;
	GtkTreeModel * model;
	GtkTreeIter iter;
	gchar * code;
	ModemRequest request;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->codes));
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ussd->codes), &iter)
			!= TRUE)
		return;
	gtk_tree_model_get(model, &iter, UC_CODE, &code, -1);
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_CALL;
	request.call.call_type = MODEM_CALL_TYPE_VOICE;
	request.call.number = code;
	helper->request(helper->phone, &request);
	g_free(code);
}
