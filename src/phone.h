/* $Id$ */
/* Copyright (c) 2010-2016 Pierre Pronchery <khorben@defora.org> */
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



#ifndef PHONE_SRC_PHONE_H
# define PHONE_SRC_PHONE_H

# include <gtk/gtk.h>
# include "../include/Phone/phone.h"


/* Phone */
/* types */
typedef enum _PhoneCallType
{
	PHONE_CALL_TYPE_OUTGOING = 0,
	PHONE_CALL_TYPE_INCOMING,
	PHONE_CALL_TYPE_MISSED
} PhoneCallType;


/* functions */
Phone * phone_new(char const * plugin, int retry);
void phone_delete(Phone * phone);


/* useful */
int phone_error(Phone * phone, char const * message, int ret);
void phone_info(Phone * phone, char const * title, char const * message);
int phone_warning(Phone * phone, char const * message, int ret);

/* calls */
void phone_call_answer(Phone * phone);
void phone_call_hangup(Phone * phone);
void phone_call_mute(Phone * phone, gboolean mute);
void phone_call_reject(Phone * phone);
void phone_call_set_volume(Phone * phone, gdouble volume);
void phone_call_speaker(Phone * phone, gboolean speaker);

/* code */
int phone_code_append(Phone * phone, char character);
void phone_code_backspace(Phone * phone);
void phone_code_clear(Phone * phone);
void phone_code_enter(Phone * phone);

/* contacts */
int phone_contacts_call_selected(Phone * phone);
void phone_contacts_delete_selected(Phone * phone);
void phone_contacts_edit_selected(Phone * phone);
void phone_contacts_new(Phone * phone);
void phone_contacts_set(Phone * phone, unsigned int index,
		ModemContactStatus status, char const * name,
		char const * number);
void phone_contacts_write_selected(Phone * phone);

/* dialer */
int phone_dialer_append(Phone * phone, char character);
void phone_dialer_backspace(Phone * phone);
int phone_dialer_call(Phone * phone, char const * number);
void phone_dialer_clear(Phone * phone);
void phone_dialer_hangup(Phone * phone);

/* events */
int phone_event(Phone * phone, PhoneEvent * event);
int phone_event_type(Phone * phone, PhoneEventType type, ...);

int phone_event_trigger(Phone * phone, ModemEventType type);

/* interface */
void phone_show_about(Phone * phone, gboolean show);
void phone_show_call(Phone * phone, gboolean show, ...);	/* PhoneCall */
void phone_show_code(Phone * phone, gboolean show, ...);	/* PhoneCode */
void phone_show_contacts(Phone * phone, gboolean show);
void phone_show_dialer(Phone * phone, gboolean show);
void phone_show_log(Phone * phone, gboolean show);
void phone_show_messages(Phone * phone, gboolean show, ...);
void phone_show_plugins(Phone * phone, gboolean show);
void phone_show_read(Phone * phone, gboolean show, ...);
void phone_show_settings(Phone * phone, gboolean show);
void phone_show_status(Phone * phone, gboolean show, ...);
void phone_show_system(Phone * phone, gboolean show);
void phone_show_write(Phone * phone, gboolean show, ...);

/* log */
void phone_log_append(Phone * phone, PhoneCallType type, char const * number);
int phone_log_call_selected(Phone * phone);
void phone_log_clear(Phone * phone);
void phone_log_write_selected(Phone * phone);

/* messages */
int phone_messages_call_selected(Phone * phone);
void phone_messages_delete_selected(Phone * phone);
void phone_messages_read_selected(Phone * phone);
void phone_messages_reply_selected(Phone * phone);
void phone_messages_set(Phone * phone, unsigned int index, char const * number,
		time_t date, ModemMessageFolder folder,
		ModemMessageStatus status, size_t length, char const * content);
void phone_messages_write(Phone * phone, char const * number,
		char const * text);

/* plugins */
int phone_load(Phone * phone, char const * plugin);
int phone_unload(Phone * phone, char const * plugin);
void phone_unload_all(Phone * phone);

/* read */
int phone_read_call(Phone * phone);
void phone_read_delete(Phone * phone);
void phone_read_forward(Phone * phone);
void phone_read_reply(Phone * phone);

/* settings */
void phone_settings_open_selected(Phone * phone);

/* write */
void phone_write_attach_dialog(Phone * phone);
void phone_write_backspace(Phone * phone);
void phone_write_copy(Phone * phone);
void phone_write_count_buffer(Phone * phone);
void phone_write_cut(Phone * phone);
void phone_write_paste(Phone * phone);
void phone_write_send(Phone * phone);

#endif /* !PHONE_SRC_PHONE_H */
