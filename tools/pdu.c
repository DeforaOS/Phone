/* $Id$ */
/* Copyright (c) 2010-2020 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
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
static int _pdu_decode(char const * string);
static int _pdu_encode(char const * number, char const * string);

static void _hexdump(char const * buf, size_t len);


/* functions */
/* pdu_decode */
static int _pdu_decode(char const * string)
{
	time_t timestamp;
	char number[32];
	ModemMessageEncoding encoding;
	char * p;
	struct tm t;
	char buf[32];
	size_t len;

	if((p = _cmgr_pdu_parse(string, &timestamp, number, &encoding, &len))
			== NULL)
	{
		fputs(PROGNAME ": Unable to decode PDU\n", stderr);
		return -1;
	}
	printf("Number: %s\n", number);
	localtime_r(&timestamp, &t);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &t);
	printf("Timestamp: %s\n", buf);
	printf("Encoding: %u\n", encoding);
	_hexdump(p, len);
	if(encoding == MODEM_MESSAGE_ENCODING_UTF8)
		printf("Message: %s\n", p);
	free(p);
	return 0;
}

static int _pdu_encode(char const * number, char const * string)
{
#if 0
	char * cmd;
	char * pdu;

	if(_message_to_pdu(0, number, GSM_MODEM_ALPHABET_DEFAULT, string,
				strlen(string), &cmd, &pdu) != 0)
#endif
	{
		fputs(PROGNAME ": Unable to encode PDU\n", stderr);
		return -1;
	}
#if 0
	puts(cmd);
	puts(pdu);
	free(cmd);
	free(pdu);
	return 0;
#endif
}


/* hexdump */
static void _hexdump(char const * buf, size_t len)
{
	unsigned char const * b = (unsigned char *)buf;
	size_t i;

	for(i = 0; i < len; i++)
	{
		printf(" %02x", b[i]);
		if((i % 16) == 7)
			fputc(' ', stdout);
		else if((i % 16) == 15)
			fputc('\n', stdout);
	}
	fputc('\n', stdout);
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME " -d pdu\n"
"       " PROGNAME " -e -n number text\n", stderr);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	int o;
	int op = 0;
	char const * number = NULL;

	while((o = getopt(argc, argv, "den:")) != -1)
		switch(o)
		{
			case 'd':
			case 'e':
				op = o;
				break;
			case 'n':
				number = optarg;
				break;
			default:
				return _usage();
		}
	if(op == 0 || optind + 1 != argc || (op == 'e' && number == NULL))
		return _usage();
	return ((op == 'd') ? _pdu_decode(argv[optind])
			: _pdu_encode(number, argv[optind])) == 0 ? 0 : 2;
}
