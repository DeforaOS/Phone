/* $Id$ */
/* Copyright (c) 2010-2020 Pierre Pronchery <khorben@defora.org> */
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



#ifndef PHONE_CALLBACKS_H
# define PHONE_CALLBACKS_H

# include <gtk/gtk.h>


/* callbacks */
gboolean on_phone_closex(gpointer data);

/* calls */
void on_phone_call_answer(gpointer data);
void on_phone_call_close(gpointer data);
void on_phone_call_hangup(gpointer data);
void on_phone_call_mute(GtkWidget * widget, gpointer data);
void on_phone_call_reject(gpointer data);
void on_phone_call_show_dialer(gpointer data);
void on_phone_call_speaker(GtkWidget * widget, gpointer data);
void on_phone_call_volume(GtkWidget * widget, gpointer data);

/* code */
void on_phone_code_backspace(gpointer data);
void on_phone_code_clicked(GtkWidget * widget, gpointer data);
void on_phone_code_enter(gpointer data);
void on_phone_code_leave(gpointer data);

/* contacts */
void on_phone_contacts_call(gpointer data);
void on_phone_contacts_delete(gpointer data);
void on_phone_contacts_edit(gpointer data);
void on_phone_contacts_new(gpointer data);
void on_phone_contacts_show(gpointer data);
void on_phone_contacts_write(gpointer data);

/* dialer */
void on_phone_dialer_backspace(gpointer data);
void on_phone_dialer_call(gpointer data);
void on_phone_dialer_changed(GtkWidget * widget, gpointer data);
void on_phone_dialer_clicked(GtkWidget * widget, gpointer data);
void on_phone_dialer_hangup(gpointer data);

/* logs */
void on_phone_log_activated(gpointer data);
void on_phone_log_call(gpointer data);
void on_phone_log_clear(gpointer data);
void on_phone_log_write(gpointer data);

/* messages */
void on_phone_messages_activated(gpointer data);
void on_phone_messages_call(gpointer data);
void on_phone_messages_delete(gpointer data);
void on_phone_messages_reply(gpointer data);
void on_phone_messages_write(gpointer data);

/* read */
void on_phone_read_call(gpointer data);
void on_phone_read_delete(gpointer data);
void on_phone_read_forward(gpointer data);
void on_phone_read_reply(gpointer data);

/* write */
void on_phone_write_attach(gpointer data);
void on_phone_write_backspace(gpointer data);
void on_phone_write_changed(gpointer data);
void on_phone_write_copy(gpointer data);
void on_phone_write_cut(gpointer data);
void on_phone_write_paste(gpointer data);
void on_phone_write_send(gpointer data);

#endif /* !PHONE_CALLBACKS_H */
