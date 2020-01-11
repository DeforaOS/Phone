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



#ifndef DESKTOP_PHONE_PHONE_H
# define DESKTOP_PHONE_PHONE_H

# include "modem.h"


/* Phone */
/* types */
typedef struct _Phone Phone;

typedef enum _PhoneCall
{
	PHONE_CALL_ESTABLISHED = 0,
	PHONE_CALL_INCOMING,
	PHONE_CALL_OUTGOING,
	PHONE_CALL_TERMINATED,
	PHONE_CALL_BUSY
} PhoneCall;

typedef enum _PhoneCode
{
	PHONE_CODE_SIM_PIN = 0
} PhoneCode;

typedef enum _PhoneEncoding
{
	/* XXX must match GSMEncoding from src/gsm.h */
	PHONE_ENCODING_UTF8 = 0,
	PHONE_ENCODING_DATA
} PhoneEncoding;

typedef enum _PhoneNotificationType
{
	PHONE_NOTIFICATION_TYPE_INFO = MODEM_NOTIFICATION_TYPE_INFO,
	PHONE_NOTIFICATION_TYPE_ERROR = MODEM_NOTIFICATION_TYPE_ERROR,
	PHONE_NOTIFICATION_TYPE_WARNING = MODEM_NOTIFICATION_TYPE_WARNING
} PhoneNotificationType;

typedef enum _PhoneEventType
{
	PHONE_EVENT_TYPE_AUDIO_PLAY,		/* char const * sample */
	PHONE_EVENT_TYPE_AUDIO_STOP,
	PHONE_EVENT_TYPE_KEY_TONE,
	PHONE_EVENT_TYPE_MESSAGE_RECEIVED,
	PHONE_EVENT_TYPE_MESSAGE_RECEIVING,	/* char const *, GSMEncoding *,
						   char **, size_t * */
	PHONE_EVENT_TYPE_MESSAGE_SENDING,	/* char const *, GSMEncoding *,
						   char **, size_t * */
	PHONE_EVENT_TYPE_MESSAGE_SENT,
	PHONE_EVENT_TYPE_MODEM_EVENT,		/* ModemEvent * event */
	PHONE_EVENT_TYPE_NOTIFICATION,		/* PhoneNotificationType type,
						   char const * title,
						   char const * message */
	PHONE_EVENT_TYPE_NOTIFICATION_OFF,
	PHONE_EVENT_TYPE_NOTIFICATION_ON,	/* char const * message? */
	PHONE_EVENT_TYPE_OFFLINE,
	PHONE_EVENT_TYPE_ONLINE,
	PHONE_EVENT_TYPE_QUIT,
	PHONE_EVENT_TYPE_RESUME,
	PHONE_EVENT_TYPE_SPEAKER_OFF,
	PHONE_EVENT_TYPE_SPEAKER_ON,
	PHONE_EVENT_TYPE_STARTED,
	PHONE_EVENT_TYPE_STARTING,
	PHONE_EVENT_TYPE_STOPPED,
	PHONE_EVENT_TYPE_STOPPING,
	PHONE_EVENT_TYPE_SUSPEND,
	PHONE_EVENT_TYPE_UNAVAILABLE,
	PHONE_EVENT_TYPE_VIBRATOR_OFF,
	PHONE_EVENT_TYPE_VIBRATOR_ON,
	PHONE_EVENT_TYPE_VOLUME_GET,
	PHONE_EVENT_TYPE_VOLUME_SET		/* double volume */
} PhoneEventType;

typedef union _PhoneEvent
{
	PhoneEventType type;

	/* PHONE_EVENT_TYPE_AUDIO_PLAY */
	struct
	{
		PhoneEventType type;
		char const * sample;
	} audio_play;

	/* PHONE_EVENT_TYPE_MODEM_EVENT */
	struct
	{
		PhoneEventType type;
		ModemEvent * event;
	} modem_event;

	/* PHONE_EVENT_TYPE_NOTIFICATION */
	struct
	{
		PhoneEventType type;
		PhoneNotificationType ntype;
		char const * title;
		char const * message;
	} notification;

	/* PHONE_EVENT_TYPE_VOLUME_GET, PHONE_EVENT_TYPE_VOLUME_SET */
	struct
	{
		PhoneEventType type;
		double level;
	} volume_get, volume_set;
} PhoneEvent;

typedef enum _PhoneMessage
{
	PHONE_MESSAGE_SHOW = 0,
	PHONE_MESSAGE_POWER_MANAGEMENT
} PhoneMessage;

typedef enum _PhoneMessagePowerManagement
{
	PHONE_MESSAGE_POWER_MANAGEMENT_RESUME = 0,
	PHONE_MESSAGE_POWER_MANAGEMENT_SUSPEND
} PhoneMessagePowerManagement;

typedef enum _PhoneMessageShow
{
	PHONE_MESSAGE_SHOW_ABOUT = 0,
	PHONE_MESSAGE_SHOW_CONTACTS,
	PHONE_MESSAGE_SHOW_DIALER,
	PHONE_MESSAGE_SHOW_LOGS,
	PHONE_MESSAGE_SHOW_MESSAGES,
	PHONE_MESSAGE_SHOW_SETTINGS,
	PHONE_MESSAGE_SHOW_WRITE
} PhoneMessageShow;


/* constants */
# define PHONE_CLIENT_MESSAGE	"DEFORAOS_DESKTOP_PHONE_CLIENT"
# define PHONE_EMBED_MESSAGE	"DEFORAOS_DESKTOP_PHONE_EMBED"

#endif /* !DESKTOP_PHONE_PHONE_H */
