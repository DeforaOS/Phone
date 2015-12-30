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
