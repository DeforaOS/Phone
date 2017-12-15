/* $Id$ */
/* Copyright (c) 2013-2017 Pierre Pronchery <khorben@defora.org> */
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



#include <string.h>
#include <System.h>
#include <Desktop.h>
#include "Phone.h"


/* Password */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	/* password */
	GtkWidget * window;
	GtkWidget * entry;
	GtkWidget * oldpassword;
	GtkWidget * newpassword;
	GtkWidget * newpassword2;
} PasswordPhonePlugin;


/* prototypes */
/* plug-in */
static PasswordPhonePlugin * _password_init(PhonePluginHelper * helper);
static void _password_destroy(PasswordPhonePlugin * password);
static int _password_event(PasswordPhonePlugin * password, PhoneEvent * event);
static void _password_settings(PasswordPhonePlugin * password);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Password",
	"stock_lock",
	NULL,
	_password_init,
	_password_destroy,
	_password_event,
	_password_settings
};


/* private */
/* functions */
/* password_init */
static PasswordPhonePlugin * _password_init(PhonePluginHelper * helper)
{
	PasswordPhonePlugin * password;

	if((password = object_new(sizeof(*password))) == NULL)
		return NULL;
	password->helper = helper;
	password->window = NULL;
	return password;
}


/* password_destroy */
static void _password_destroy(PasswordPhonePlugin * password)
{
	object_delete(password);
}


/* password_event */
static int _password_event(PasswordPhonePlugin * password, PhoneEvent * event)
{
	switch(event->type)
	{
		default:
			break;
	}
	return 0;
}


/* password_settings */
static void _on_settings_cancel(gpointer data);
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_ok(gpointer data);
static void _on_settings_ok_error(PasswordPhonePlugin * password,
		char const * error);

static void _password_settings(PasswordPhonePlugin * password)
{
	GtkSizeGroup * group;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;

	if(password->window != NULL)
	{
		gtk_window_present(GTK_WINDOW(password->window));
		return;
	}
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	password->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(password->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(password->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	/* XXX find something more appropriate */
	gtk_window_set_icon_name(GTK_WINDOW(password->window), "stock_lock");
#endif
	gtk_window_set_title(GTK_WINDOW(password->window), "Password");
	g_signal_connect_swapped(password->window, "delete-event", G_CALLBACK(
				_on_settings_closex), password);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* entry */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	widget = gtk_label_new("Name: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 24, 0)
	password->entry = gtk_combo_box_text_new_with_entry();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(password->entry),
			"SIM PIN");
#else
	password->entry = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(password->entry), "SIM PIN");
#endif
	gtk_box_pack_start(GTK_BOX(hbox), password->entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* old password */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	widget = gtk_label_new("Old password: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	password->oldpassword = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password->oldpassword), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), password->oldpassword, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* new password */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	widget = gtk_label_new("New password: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	password->newpassword = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password->newpassword), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), password->newpassword, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* new password */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	widget = gtk_label_new("Confirm: ");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	password->newpassword2 = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password->newpassword2), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), password->newpassword2, TRUE, TRUE,
			0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* buttons */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_settings_cancel), password);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_on_settings_ok),
			password);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(password->window), vbox);
	gtk_widget_show_all(password->window);
}

static void _on_settings_cancel(gpointer data)
{
	PasswordPhonePlugin * password = data;

	gtk_widget_hide(password->window);
	gtk_entry_set_text(GTK_ENTRY(password->oldpassword), "");
	gtk_entry_set_text(GTK_ENTRY(password->newpassword), "");
	gtk_entry_set_text(GTK_ENTRY(password->newpassword2), "");
}

static gboolean _on_settings_closex(gpointer data)
{
	PasswordPhonePlugin * password = data;

	gtk_widget_hide(password->window);
	return TRUE;
}

static void _on_settings_ok(gpointer data)
{
	PasswordPhonePlugin * password = data;
	PhonePluginHelper * helper = password->helper;
	GtkWidget * widget;
	char const * name;
	char const * oldpassword;
	char const * newpassword;
	char const * newpassword2;
	ModemRequest request;

	widget = gtk_bin_get_child(GTK_BIN(password->entry));
	name = gtk_entry_get_text(GTK_ENTRY(widget));
	oldpassword = gtk_entry_get_text(GTK_ENTRY(password->oldpassword));
	newpassword = gtk_entry_get_text(GTK_ENTRY(password->newpassword));
	newpassword2 = gtk_entry_get_text(GTK_ENTRY(password->newpassword2));
	if(strcmp(newpassword, newpassword2) != 0)
	{
		_on_settings_ok_error(password,
				"The new passwords do not match");
		return;
	}
	/* issue the request */
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_PASSWORD_SET;
	request.password_set.name = name;
	request.password_set.oldpassword = oldpassword;
	request.password_set.newpassword = newpassword;
	helper->request(helper->phone, &request);
	gtk_widget_hide(password->window);
}

static void _on_settings_ok_error(PasswordPhonePlugin * password,
		char const * error)
{
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(password->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", "Error");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", error);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
