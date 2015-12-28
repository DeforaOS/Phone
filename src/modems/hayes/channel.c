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



#include "channel.h"


/* HayesChannel */
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
