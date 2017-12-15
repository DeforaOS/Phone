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
