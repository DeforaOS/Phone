/* $Id$ */
/* Copyright (c) 2014-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <sys/types.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include "quirks.h"


/* HayesQuirks */
/* private */
/* constants */
static HayesQuirks _hayes_quirks[] =
{
	{ "Ericsson", "F3507g",
		HAYES_QUIRK_CPIN_SLOW
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "Nokia", "Nokia N9",
		HAYES_QUIRK_WANT_SMSC_IN_PDU				},
	{ "Sierra Wireless Inc.", "Sierra Wireless EM7345 4G LTE",
		HAYES_QUIRK_WANT_SMSC_IN_PDU				},
	{ "Openmoko", "\"Neo1973 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "Openmoko", "\"Neo1973 GTA01/GTA02 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "Openmoko", "\"Neo1973 GTA02 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "Nokia", "Nokia N900",
		HAYES_QUIRK_BATTERY_70					}
};


/* public */
/* functions */
/* hayes_quirks */
unsigned int hayes_quirks(char const * vendor, char const * model)
{
	size_t i;

	if(vendor == NULL || model == NULL)
		return 0;
	for(i = 0; i < sizeof(_hayes_quirks) / sizeof(*_hayes_quirks); i++)
		if(strcmp(_hayes_quirks[i].vendor, vendor) == 0
				&& strcmp(_hayes_quirks[i].model, model) == 0)
		{
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() quirks=%u\n", __func__,
					_hayes_quirks[i].quirks);
#endif
			return _hayes_quirks[i].quirks;
		}
	return 0;
}
