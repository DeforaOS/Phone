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



#include <stddef.h>
#include "quirks.h"


/* HayesQuirks */
/* public */
/* constants */
HayesQuirks hayes_quirks[] =
{
	{ "\"Neo1973 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "\"Neo1973 GTA01/GTA02 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "\"Neo1973 GTA02 Embedded GSM Modem\"",
		HAYES_QUIRK_WANT_SMSC_IN_PDU
			| HAYES_QUIRK_CONNECTED_LINE_DISABLED
			| HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR		},
	{ "Nokia N900",
		HAYES_QUIRK_BATTERY_70					},
	{ NULL,	0							}
};
