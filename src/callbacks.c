/* $Id$ */
/* Copyright (c) 2010 Pierre Pronchery <khorben@defora.org> */
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



#include "phone.h"
#include "callbacks.h"


/* callbacks */
/* on_phone_closex */
gboolean on_phone_closex(gpointer data)
{
	GtkWidget * widget = data;

	gtk_widget_hide(widget);
	return TRUE;
}


/* on_phone_code_clear */
void on_phone_code_clear(gpointer data)
{
	Phone * phone = data;

	phone_code_clear(phone);
}


/* on_phone_code_clicked */
void on_phone_code_clicked(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	char const * character;

	character = g_object_get_data(G_OBJECT(widget), "character");
	phone_code_append(phone, *character);
}


/* code */
void on_phone_code_enter(gpointer data)
{
	Phone * phone = data;

	phone_show_code(phone, FALSE);
	phone_code_enter(phone);
}


/* on_phone_code_leave */
void on_phone_code_leave(gpointer data)
{
	Phone * phone = data;

	phone_show_code(phone, FALSE);
}


/* contacts */
/* on_phone_contacts_call */
void on_phone_contacts_call(gpointer data)
{
	Phone * phone = data;

	phone_contact_call_selected(phone);
}


/* on_phone_contacts_show */
void on_phone_contacts_show(gpointer data)
{
	Phone * phone = data;

	phone_show_contacts(phone, TRUE);
}


/* dialer */
/* on_phone_dialer_call */
void on_phone_dialer_call(gpointer data)
{
	Phone * phone = data;

	phone_call(phone, NULL);
}


/* on_phone_dialer_clicked */
void on_phone_dialer_clicked(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	char const * character;

	character = g_object_get_data(G_OBJECT(widget), "character");
	phone_dialer_append(phone, *character);
}


/* on_phone_dialer_hangup */
void on_phone_dialer_hangup(gpointer data)
{
	Phone * phone = data;

	phone_hangup(phone);
}
