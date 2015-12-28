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



#include <stdlib.h>
#include "command.h"
#include "channel.h"


/* HayesChannel */
/* private */
/* prototypes */
static void _hayeschannel_reset_source(guint * source);


/* public */
/* functions */
/* hayeschannel_init */
void hayeschannel_init(HayesChannel * channel, ModemPlugin * modem)
{
	size_t i;

	channel->hayes = modem;
	channel->mode = HAYES_MODE_INIT;
	for(i = 0; i < sizeof(channel->events) / sizeof(*channel->events); i++)
		channel->events[i].type = i;
	channel->events[MODEM_EVENT_TYPE_REGISTRATION].registration.signal
		= 0.0 / 0.0;
}


/* hayeschannel_destroy */
void hayeschannel_destroy(HayesChannel * channel)
{
	(void) channel;
}


/* useful */
/* queue management */
/* hayeschannel_queue_flush */
void hayeschannel_queue_flush(HayesChannel * channel)
{
	g_slist_foreach(channel->queue_timeout, (GFunc)hayes_command_delete,
			NULL);
	g_slist_free(channel->queue_timeout);
	channel->queue_timeout = NULL;
	g_slist_foreach(channel->queue, (GFunc)hayes_command_delete, NULL);
	g_slist_free(channel->queue);
	channel->queue = NULL;
	free(channel->rd_buf);
	channel->rd_buf = NULL;
	channel->rd_buf_cnt = 0;
	_hayeschannel_reset_source(&channel->rd_source);
	free(channel->wr_buf);
	channel->wr_buf = NULL;
	channel->wr_buf_cnt = 0;
	_hayeschannel_reset_source(&channel->wr_source);
	_hayeschannel_reset_source(&channel->rd_ppp_source);
	_hayeschannel_reset_source(&channel->wr_ppp_source);
	channel->authenticate_count = 0;
	_hayeschannel_reset_source(&channel->authenticate_source);
	_hayeschannel_reset_source(&channel->timeout);
}


/* private */
/* functions */
/* hayeschannel_reset_source */
static void _hayeschannel_reset_source(guint * source)
{
	/* XXX duplicated from _hayes_reset_source() */
	if(*source == 0)
		return;
	g_source_remove(*source);
	*source = 0;
}
