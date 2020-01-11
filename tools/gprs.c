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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <System.h>

#ifndef PROGNAME_GPRS
# define PROGNAME_GPRS "gprs"
#endif
#ifndef PROGNAME
# define PROGNAME 	PROGNAME_GPRS
#endif

#include "../src/plugins/gprs.c"
#include "common.c"


/* private */
/* prototypes */
static int _gprs(void);

static int _usage(void);


/* functions */
/* gprs */
static gboolean _gprs_on_idle(gpointer data);

static int _gprs(void)
{
	Phone phone;

	if(_phone_init(&phone, &plugin) != 0)
		return -1;
	g_idle_add(_gprs_on_idle, &phone);
	gtk_main();
	_gprs_destroy(phone.plugin);
	_phone_destroy(&phone);
	return 0;
}

static gboolean _gprs_on_idle(gpointer data)
{
	Phone * phone = data;

	if(phone->plugind->settings != NULL)
		phone->plugind->settings(phone->plugin);
	return FALSE;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_GPRS "\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int ret;
	int o;

	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	gtk_init(&argc, &argv);
	if((ret = (_gprs() == 0) ? 0 : 2) != 0)
		error_print(PROGNAME_GPRS);
	return ret;
}
