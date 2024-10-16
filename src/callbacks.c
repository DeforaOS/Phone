/* $Id$ */
/* Copyright (c) 2010-2024 Pierre Pronchery <khorben@defora.org> */
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


/* calls */
/* on_phone_call_answer */
void on_phone_call_answer(gpointer data)
{
	Phone * phone = data;

	phone_call_answer(phone);
}


/* on_phone_call_close */
void on_phone_call_close(gpointer data)
{
	Phone * phone = data;

	phone_show_call(phone, FALSE);
}


/* on_phone_call_hangup */
void on_phone_call_hangup(gpointer data)
{
	Phone * phone = data;

	phone_call_hangup(phone);
}


/* on_phone_call_mute */
void on_phone_call_mute(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	gboolean mute;

	mute = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	phone_call_mute(phone, mute);
}


/* on_phone_call_reject */
void on_phone_call_reject(gpointer data)
{
	Phone * phone = data;

	phone_call_reject(phone);
}


/* on_phone_call_show_dialer */
void on_phone_call_show_dialer(gpointer data)
{
	Phone * phone = data;

	phone_show_dialer(phone, TRUE);
}


/* on_phone_call_speaker */
void on_phone_call_speaker(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	gboolean speaker;

	speaker = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	phone_call_speaker(phone, speaker);
}


/* on_phone_call_volume */
void on_phone_call_volume(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	gdouble volume;

	volume = gtk_range_get_value(GTK_RANGE(widget));
	phone_call_set_volume(phone, volume);
}


/* code */
/* on_phone_code_backspace */
void on_phone_code_backspace(gpointer data)
{
	Phone * phone = data;

	phone_code_backspace(phone);
}


/* on_phone_code_clicked */
void on_phone_code_clicked(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	char const * character;

	character = g_object_get_data(G_OBJECT(widget), "character");
	phone_event_type(phone, PHONE_EVENT_TYPE_KEY_TONE);
	phone_code_append(phone, *character);
}


/* code */
void on_phone_code_enter(gpointer data)
{
	Phone * phone = data;

	phone_event_type(phone, PHONE_EVENT_TYPE_KEY_TONE);
	phone_code_enter(phone);
}


/* on_phone_code_leave */
void on_phone_code_leave(gpointer data)
{
	Phone * phone = data;

	phone_event_type(phone, PHONE_EVENT_TYPE_KEY_TONE);
	phone_show_code(phone, FALSE);
}


/* contacts */
/* on_phone_contacts_call */
void on_phone_contacts_call(gpointer data)
{
	Phone * phone = data;

	phone_contacts_call_selected(phone);
}


/* on_phone_contacts_delete */
void on_phone_contacts_delete(gpointer data)
{
	Phone * phone = data;

	phone_contacts_delete_selected(phone);
}


/* on_phone_contacts_edit */
void on_phone_contacts_edit(gpointer data)
{
	Phone * phone = data;

	phone_contacts_edit_selected(phone);
}


/* on_phone_contacts_new */
void on_phone_contacts_new(gpointer data)
{
	Phone * phone = data;

	phone_contacts_new(phone);
}


/* on_phone_contacts_show */
void on_phone_contacts_show(gpointer data)
{
	Phone * phone = data;

	phone_show_contacts(phone, TRUE);
}


/* on_phone_contacts_write */
void on_phone_contacts_write(gpointer data)
{
	Phone * phone = data;

	phone_contacts_write_selected(phone);
}


/* dialer */
/* on_phone_dialer_backspace */
void on_phone_dialer_backspace(gpointer data)
{
	Phone * phone = data;

	phone_dialer_backspace(phone);
}


/* on_phone_dialer_call */
void on_phone_dialer_call(gpointer data)
{
	Phone * phone = data;

	phone_event_type(phone, PHONE_EVENT_TYPE_KEY_TONE);
	phone_dialer_call(phone, NULL);
}


/* on_phone_dialer_changed */
void on_phone_dialer_changed(GtkWidget * widget, gpointer data)
{
	Phone * phone = data;
	gchar * text;

	text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	if(strcmp(text, "*#0000#") == 0)
	{
		phone_show_about(phone, TRUE);
		phone_dialer_clear(phone);
	}
	else if(strcmp(text, "*#06#") == 0)
	{
		phone_event_trigger(phone, MODEM_EVENT_TYPE_MODEL);
		phone_dialer_clear(phone);
	}
	g_free(text);
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

	phone_event_type(phone, PHONE_EVENT_TYPE_KEY_TONE);
	phone_dialer_hangup(phone);
}


/* logs */
/* on_phone_log_activated */
void on_phone_log_activated(gpointer data)
{
	Phone * phone = data;

	/* XXX may not be the most appropriate action to take */
	phone_log_call_selected(phone);
}


/* on_phone_log_call */
void on_phone_log_call(gpointer data)
{
	Phone * phone = data;

	phone_log_call_selected(phone);
}


/* on_phone_log_clear */
void on_phone_log_clear(gpointer data)
{
	Phone * phone = data;

	phone_log_clear(phone);
}


/* on_phone_log_write */
void on_phone_log_write(gpointer data)
{
	Phone * phone = data;

	phone_log_write_selected(phone);
}


/* messages */
/* on_phone_messages_activated */
void on_phone_messages_activated(gpointer data)
{
	Phone * phone = data;

	phone_messages_read_selected(phone);
}


/* on_phone_messages_call */
void on_phone_messages_call(gpointer data)
{
	Phone * phone = data;

	phone_messages_call_selected(phone);
}


/* on_phone_messages_delete */
void on_phone_messages_delete(gpointer data)
{
	Phone * phone = data;

	phone_messages_delete_selected(phone);
}


/* on_phone_messages_reply */
void on_phone_messages_reply(gpointer data)
{
	Phone * phone = data;

	phone_messages_reply_selected(phone);
}


/* on_phone_messages_write */
void on_phone_messages_write(gpointer data)
{
	Phone * phone = data;

	phone_messages_write(phone, NULL, NULL);
}


/* read */
/* on_phone_read_call */
void on_phone_read_call(gpointer data)
{
	Phone * phone = data;

	phone_read_call(phone);
}


/* on_phone_read_delete */
void on_phone_read_delete(gpointer data)
{
	Phone * phone = data;

	phone_read_delete(phone);
}


/* on_phone_read_forward */
void on_phone_read_forward(gpointer data)
{
	Phone * phone = data;

	phone_read_forward(phone);
}


/* on_phone_read_reply */
void on_phone_read_reply(gpointer data)
{
	Phone * phone = data;

	phone_read_reply(phone);
}


/* write */
/* on_phone_write_attach */
void on_phone_write_attach(gpointer data)
{
	Phone * phone = data;

	phone_write_attach_dialog(phone);
}


/* on_phone_write_backspace */
void on_phone_write_backspace(gpointer data)
{
	Phone * phone = data;

	phone_write_backspace(phone);
}


/* on_phone_write_changed */
void on_phone_write_changed(gpointer data)
{
	Phone * phone = data;

	phone_write_count_buffer(phone);
}


/* on_phone_write_copy */
void on_phone_write_copy(gpointer data)
{
	Phone * phone = data;

	phone_write_copy(phone);
}


/* on_phone_write_cut */
void on_phone_write_cut(gpointer data)
{
	Phone * phone = data;

	phone_write_cut(phone);
}


/* on_phone_write_paste */
void on_phone_write_paste(gpointer data)
{
	Phone * phone = data;

	phone_write_paste(phone);
}


/* on_phone_write_send */
void on_phone_write_send(gpointer data)
{
	Phone * phone = data;

	phone_write_send(phone);
}
