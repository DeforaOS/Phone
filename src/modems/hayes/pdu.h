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



#ifndef PHONE_MODEM_HAYES_PDU_H
# define PHONE_MODEM_HAYES_PDU_H

# include <Phone/modem.h>


/* HayesPDU */
/* public */
/* types */
typedef enum _HayesPDUFlag
{
	HAYESPDU_FLAG_WANT_SMSC = 1
} HayesPDUFlag;


/* functions */
char * hayespdu_encode(char const * number, ModemMessageEncoding encoding,
		size_t length, char const * content, unsigned int flags);

#endif /* PHONE_MODEM_HAYES_PDU_H */
