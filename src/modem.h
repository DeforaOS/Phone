/* $Id$ */
/* Copyright (c) 2010-2012 Pierre Pronchery <khorben@defora.org> */
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



#ifndef PHONE_MODEM_H
# define PHONE_MODEM_H

# include <System.h>
# include "../include/Phone/modem.h"


/* Modem */
/* public */
/* types */
typedef void (*ModemEventCallback)(void * priv, ModemEvent * event);


/* functions */
Modem * modem_new(Config * config, char const * plugin, unsigned int retry);
void modem_delete(Modem * modem);

/* accessors */
ModemConfig * modem_get_config(Modem * modem);
char const * modem_get_name(Modem * modem);
void modem_set_callback(Modem * modem, ModemEventCallback callback,
		void * priv);

/* useful */
int modem_request(Modem * modem, ModemRequest * request);
int modem_request_type(Modem * modem, ModemRequestType type, ...);
int modem_start(Modem * modem);
int modem_stop(Modem * modem);
int modem_trigger(Modem * modem, ModemEventType event);

#endif /* !PHONE_MODEM_H */
