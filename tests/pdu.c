/* $Id$ */
/* Copyright (c) 2014-2018 Pierre Pronchery <khorben@defora.org> */
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
#include "../src/modems/hayes/channel.c"
#include "../src/modems/hayes/command.c"
#include "../src/modems/hayes/common.c"
#include "../src/modems/hayes/pdu.c"
#include "../src/modems/hayes/quirks.c"
#include "../src/modems/hayes.c"

#ifndef PROGNAME
# define PROGNAME "pdu"
#endif


/* pdu */
/* prototypes */
static int _pdu(void);

static int _pdu_decode(char const * pdu, char const * number,
		char const * datetime, ModemMessageEncoding encoding,
		char const * message);
static int _pdu_encode(void);


/* functions */
/* pdu */
static int _pdu(void)
{
	int ret = 0;

	ret |= (_pdu_decode("07916407058099F9040B916407752743"
				"F60000990121017580001554747A0E4A"
				"CF416110945805B5CBF379F85C06",
				"+46705772346", "12/10/1999 12:57:08",
				MODEM_MESSAGE_ENCODING_UTF8,
				"This is a PDU message") != 0) ? 1 : 0;
	ret |= (_pdu_encode() != 0) ? 2 : 0;
	return ret;
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
		fputs(PROGNAME ": Unable to decode PDU\n", stderr);
		return -1;
	}
	/* check the number */
	if(strcmp(buf, number) != 0)
	{
		fputs(PROGNAME ": Did not match the number\n", stderr);
		ret = 1;
	}
	/* check the timestamp */
	localtime_r(&timestamp, &t);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &t);
	if(strcmp(buf, datetime) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", PROGNAME, buf,
				"Did not match the date");
		ret = 2;
	}
	/* check the encoding */
	if(e != encoding)
	{
		fputs(PROGNAME ": Did not match the encoding\n", stderr);
		ret = 3;
	}
	/* check the message */
	if(strcmp(p, message) != 0)
	{
		fputs(PROGNAME ": Did not match the message\n", stderr);
		ret = 4;
	}
	free(p);
	return ret;
}


/* pdu_encode */
static int _pdu_encode(void)
{
	int ret;
	char * pdu;
	char const number[] = "1234";
	ModemMessageEncoding encoding = MODEM_MESSAGE_ENCODING_ASCII;
	char const string[] = "This is just a test.";

	if((pdu = hayespdu_encode(number, encoding, sizeof(string) - 1, string,
					0)) == NULL)
		return -1;
	ret = _pdu_decode(pdu, number, NULL, encoding, string);
	free(pdu);
	return ret;
}


/* main */
int main(void)
{
	return (_pdu() << 1);
}
