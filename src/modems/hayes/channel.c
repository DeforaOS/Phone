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



#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include "command.h"
#include "common.h"
#include "channel.h"


/* HayesChannel */
/* public */
/* functions */
/* hayeschannel_init */
void hayeschannel_init(HayesChannel * channel, ModemPlugin * modem)
{
	size_t i;

	channel->hayes = modem;
	channel->mode = HAYESCHANNEL_MODE_INIT;
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


/* accessors */
/* hayeschannel_has_quirks */
int hayeschannel_has_quirks(HayesChannel * channel, unsigned int quirks)
{
	return ((channel->quirks & quirks) == quirks) ? 1 : 0;
}


/* hayeschannel_is_started */
int hayeschannel_is_started(HayesChannel * channel)
{
	return (channel->channel != NULL) ? 1 : 0;
}


/* hayeschannel_set_quirks */
void hayeschannel_set_quirks(HayesChannel * channel, unsigned int quirks)
{
	channel->quirks = quirks;
}


/* useful */
/* queue management */
/* hayeschannel_queue_data */
int hayeschannel_queue_data(HayesChannel * channel, char const * buf,
		size_t size)
{
	char * p;

	if((p = realloc(channel->wr_buf, channel->wr_buf_cnt + size)) == NULL)
		return -1;
	channel->wr_buf = p;
	memcpy(&channel->wr_buf[channel->wr_buf_cnt], buf, size);
	channel->wr_buf_cnt += size;
	return 0;
}


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
	hayescommon_source_reset(&channel->rd_source);
	free(channel->wr_buf);
	channel->wr_buf = NULL;
	channel->wr_buf_cnt = 0;
	hayescommon_source_reset(&channel->wr_source);
	hayescommon_source_reset(&channel->rd_ppp_source);
	hayescommon_source_reset(&channel->wr_ppp_source);
	channel->authenticate_count = 0;
	hayescommon_source_reset(&channel->authenticate_source);
	hayescommon_source_reset(&channel->timeout);
}


/* hayeschannel_queue_pop */
int hayeschannel_queue_pop(HayesChannel * channel)
{
	HayesCommand * command;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	hayescommon_source_reset(&channel->timeout);
	if(channel->queue == NULL) /* nothing to send */
		return 0;
	command = channel->queue->data; /* XXX assumes it's valid */
	hayes_command_delete(command);
	channel->queue = g_slist_remove(channel->queue, command);
	return 0;
}


/* hayeschannel_stop */
static void _stop_giochannel(GIOChannel * channel);
static void _stop_string(char ** string);

void hayeschannel_stop(HayesChannel * channel)
{
	size_t i;

	/* close everything opened */
	if(channel->fp != NULL)
		fclose(channel->fp);
	channel->fp = NULL;
	hayeschannel_queue_flush(channel);
	_stop_giochannel(channel->channel);
	channel->channel = NULL;
	_stop_giochannel(channel->rd_ppp_channel);
	channel->rd_ppp_channel = NULL;
	_stop_giochannel(channel->wr_ppp_channel);
	channel->wr_ppp_channel = NULL;
	/* remove internal data */
	_stop_string(&channel->authentication_name);
	_stop_string(&channel->authentication_error);
	_stop_string(&channel->call_number);
	_stop_string(&channel->contact_name);
	_stop_string(&channel->contact_number);
	_stop_string(&channel->gprs_username);
	_stop_string(&channel->gprs_password);
	_stop_string(&channel->message_number);
	_stop_string(&channel->model_identity);
	_stop_string(&channel->model_name);
	_stop_string(&channel->model_serial);
	_stop_string(&channel->model_vendor);
	_stop_string(&channel->model_version);
	_stop_string(&channel->registration_media);
	_stop_string(&channel->registration_operator);
	/* reset events */
	memset(&channel->events, 0, sizeof(channel->events));
	for(i = 0; i < sizeof(channel->events) / sizeof(*channel->events); i++)
		channel->events[i].type = i;
	/* reset mode */
	channel->mode = HAYESCHANNEL_MODE_INIT;
}

static void _stop_giochannel(GIOChannel * channel)
{
	GError * error = NULL;

	if(channel == NULL)
		return;
	/* XXX should the file descriptor also be closed? */
	if(g_io_channel_shutdown(channel, TRUE, &error) == G_IO_STATUS_ERROR)
		/* XXX report error */
		g_error_free(error);
	g_io_channel_unref(channel);
}

static void _stop_string(char ** string)
{
	free(*string);
	*string = NULL;
}
