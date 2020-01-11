/* $Id$ */
/* Copyright (c) 2011-2012 Pierre Pronchery <khorben@defora.org> */
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
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <Desktop.h>
#include "phone.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs(_("Usage: phonectl -C\n"
"       phonectl -D\n"
"       phonectl -L\n"
"       phonectl -M\n"
"       phonectl -S\n"
"       phonectl -W\n"
"       phonectl -r\n"
"       phonectl -s\n"
"  -C	Open the contacts window\n"
"  -D	Show the dialer\n"
"  -L	Open the phone log window\n"
"  -M	Open the messages window\n"
"  -S	Display or change settings\n"
"  -W	Write a new message\n"
"  -r	Resume telephony operation\n"
"  -s	Suspend telephony operation\n"), stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int type = PHONE_MESSAGE_SHOW;
	int action = -1;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "CDLMSWrs")) != -1)
		switch(o)
		{
			case 'C':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_CONTACTS;
				break;
			case 'D':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_DIALER;
				break;
			case 'L':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_LOGS;
				break;
			case 'M':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_MESSAGES;
				break;
			case 'S':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_SETTINGS;
				break;
			case 'W':
				if(action != -1)
					return _usage();
				action = PHONE_MESSAGE_SHOW_WRITE;
				break;
			case 'r':
				if(action != -1)
					return _usage();
				type = PHONE_MESSAGE_POWER_MANAGEMENT;
				action = PHONE_MESSAGE_POWER_MANAGEMENT_RESUME;
				break;
			case 's':
				if(action != -1)
					return _usage();
				type = PHONE_MESSAGE_POWER_MANAGEMENT;
				action = PHONE_MESSAGE_POWER_MANAGEMENT_SUSPEND;
				break;
			default:
				return _usage();
		}
	if(action < 0)
		return _usage();
	desktop_message_send(PHONE_CLIENT_MESSAGE, type, action, TRUE);
	return 0;
}
