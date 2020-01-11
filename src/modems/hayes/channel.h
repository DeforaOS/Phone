/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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



#ifndef PHONE_MODEM_HAYES_CHANNEL_H
# define PHONE_MODEM_HAYES_CHANNEL_H

# include <sys/types.h>
# include <stdio.h>
# include <Phone/modem.h>
# include <glib.h>


/* HayesChannel */
/* public */
/* types */
typedef enum _HayesChannelMode
{
	HAYESCHANNEL_MODE_INIT = 0,
	HAYESCHANNEL_MODE_COMMAND,
	HAYESCHANNEL_MODE_DATA,
	HAYESCHANNEL_MODE_PDU
} HayesChannelMode;

typedef struct _HayesChannel
{
	ModemPlugin * hayes;

	unsigned int quirks;

	guint source;
	guint timeout;
	guint authenticate_count;
	guint authenticate_source;

	GIOChannel * channel;
	char * rd_buf;
	size_t rd_buf_cnt;
	guint rd_source;
	char * wr_buf;
	size_t wr_buf_cnt;
	guint wr_source;
	GIOChannel * rd_ppp_channel;
	guint rd_ppp_source;
	GIOChannel * wr_ppp_channel;
	guint wr_ppp_source;

	/* logging */
	FILE * fp;

	/* queue */
	HayesChannelMode mode;
	GSList * queue;
	GSList * queue_timeout;

	/* events */
	ModemEvent events[MODEM_EVENT_TYPE_COUNT];
	char * authentication_name;
	char * authentication_error;
	char * call_number;
	char * contact_name;
	char * contact_number;
	char * gprs_username;
	char * gprs_password;
	char * message_number;
	char * model_identity;
	char * model_name;
	char * model_serial;
	char * model_vendor;
	char * model_version;
	char * registration_media;
	char * registration_operator;
} HayesChannel;


/* functions */
void hayeschannel_init(HayesChannel * channel, ModemPlugin * modem);
void hayeschannel_destroy(HayesChannel * channel);

/* accessors */
int hayeschannel_has_quirks(HayesChannel * channel, unsigned int quirks);

int hayeschannel_is_started(HayesChannel * channel);

void hayeschannel_set_quirks(HayesChannel * channel, unsigned int quirks);

/* useful */
/* queue management */
int hayeschannel_queue_data(HayesChannel * channel, char const * buf,
		size_t size);
void hayeschannel_queue_flush(HayesChannel * channel);
int hayeschannel_queue_pop(HayesChannel * channel);

void hayeschannel_stop(HayesChannel * channel);

#endif /* PHONE_MODEM_HAYES_CHANNEL_H */
