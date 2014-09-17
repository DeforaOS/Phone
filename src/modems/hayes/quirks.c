/* $Id$ */
/* Copyright (c) 2014 Pierre Pronchery <khorben@defora.org> */
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



#include <sys/types.h>
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
