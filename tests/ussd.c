/* $Id$ */
/* Copyright (c) 2014-2020 Pierre Pronchery <khorben@defora.org> */
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



#include "../src/modems/hayes/channel.c"
#include "../src/modems/hayes/command.c"
#include "../src/modems/hayes/common.c"
#include "../src/modems/hayes/pdu.c"
#include "../src/modems/hayes/quirks.c"
#include "../src/modems/hayes.c"


/* private */
/* prototypes */
static int _ussd(void);


/* functions */
/* ussd */
static int _ussd(void)
{
	int ret = 0;
	const char * codes[] = { "*100#", "*109*72348937857623#" };
	const char * notcodes[] = { "*#06#0" };
	size_t i;

	for(i = 0; i < sizeof(codes) / sizeof(*codes); i++)
		if(!_is_ussd_code(codes[i]))
		{
			printf("%s: %s: %s\n", "ussd", codes[i],
					"Is a valid USSD code");
			ret = 2;
		}
	for(i = 0; i < sizeof(notcodes) / sizeof(*notcodes); i++)
		if(_is_ussd_code(notcodes[i]))
		{
			printf("%s: %s: %s\n", "ussd", notcodes[i],
					"Is not a valid USSD code");
			ret = 2;
		}
	return ret;
}


/* public */
/* functions */
/* main */
int main(void)
{
	return (_ussd() == 0) ? 0 : 2;
}
