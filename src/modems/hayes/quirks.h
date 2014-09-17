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



#ifndef PHONE_MODEM_HAYES_QUIRKS_H
# define PHONE_MODEM_HAYES_QUIRKS_H


/* HayesQuirks */
/* public */
/* types */
typedef enum _HayesQuirk
{
	HAYES_QUIRK_BATTERY_70			= 0x01,
	HAYES_QUIRK_CPIN_NO_QUOTES		= 0x02,
	HAYES_QUIRK_CPIN_SLOW			= 0x04,
	HAYES_QUIRK_CONNECTED_LINE_DISABLED	= 0x08,
	HAYES_QUIRK_WANT_SMSC_IN_PDU		= 0x10,
	HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR	= 0x20
} HayesQuirk;

typedef const struct _HayesQuirks
{
	char const * vendor;
	char const * model;
	unsigned int quirks;
} HayesQuirks;


/* functions */
unsigned int hayes_quirks(char const * vendor, char const * model);

#endif /* PHONE_MODEM_HAYES_QUIRKS_H */
