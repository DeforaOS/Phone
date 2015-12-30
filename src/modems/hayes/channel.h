/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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
void hayeschannel_set_quirks(HayesChannel * channel, unsigned int quirks);

/* useful */
/* queue management */
int hayeschannel_queue_data(HayesChannel * channel, char const * buf,
		size_t size);
void hayeschannel_queue_flush(HayesChannel * channel);
int hayeschannel_queue_pop(HayesChannel * channel);

#endif /* PHONE_MODEM_HAYES_CHANNEL_H */
