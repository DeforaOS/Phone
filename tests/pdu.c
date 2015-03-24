/* $Id$ */
/* Copyright (c) 2014-2015 Pierre Pronchery <khorben@defora.org> */
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Phone/modem.h"
#include "../src/modems/hayes/command.c"
#include "../src/modems/hayes/quirks.c"
#include "../src/modems/hayes.c"


/* pdu */
/* prototypes */
static int _pdu(void);

static int _pdu_decode(char const * pdu, char const * number,
		char const * datetime, ModemMessageEncoding encoding,
		char const * message);


/* functions */
/* pdu */
static int _pdu(void)
{
	return _pdu_decode("07916407058099F9040B916407752743"
			"F60000990121017580001554747A0E4A"
			"CF416110945805B5CBF379F85C06",
			"+46705772346", "12/10/1999 11:57:08",
			MODEM_MESSAGE_ENCODING_UTF8,
			"This is a PDU message");
}


/* pdu_decode */
static int _pdu_decode(char const * pdu, char const * number,
		char const * datetime, ModemMessageEncoding encoding,
		char const * message)
{
	int ret = 0;
	time_t timestamp;
	char buf[32];
	ModemMessageEncoding e;
	char * p;
	struct tm t;
	size_t len;

	if((p = _cmgr_pdu_parse(pdu, &timestamp, buf, &e, &len)) == NULL)
	{
		fputs("pdu: Unable to decode PDU\n", stderr);
		return -1;
	}
	/* check the number */
	if(strcmp(buf, number) != 0)
	{
		fputs("pdu: Did not match the number\n", stderr);
		ret = 1;
	}
	/* check the timestamp */
	localtime_r(&timestamp, &t);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &t);
	if(strcmp(buf, datetime) != 0)
	{
		fputs("pdu: Did not match the date\n", stderr);
		ret = 2;
	}
	/* check the encoding */
	if(e != encoding)
	{
		fputs("pdu: Did not match the encoding\n", stderr);
		ret = 3;
	}
	/* check the message */
	if(strcmp(p, message) != 0)
	{
		fputs("pdu: Did not match the message\n", stderr);
		ret = 4;
	}
	free(p);
	return ret;
}


/* main */
int main(void)
{
	return (_pdu() == 0) ? 0 : 2;
}
