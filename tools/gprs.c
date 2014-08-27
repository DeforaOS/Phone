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
/* TODO:
 * - fix the counters upon start
 * - store the default settings for the operator in a configuration file
 * - actually call (and track) pppd */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <System.h>
#include "../src/plugins/gprs.c"
#include "common.c"

#ifndef PROGNAME
# define PROGNAME "gprs"
#endif


/* private */
/* prototypes */
static int _gprs(Config * config);


/* functions */
/* gprs */
static gboolean _gprs_on_idle(gpointer data);

static int _gprs(Config * config)
{
	PhonePluginHelper helper;
	Phone phone;

	phone.config = config;
	phone.plugind = &plugin;
	_helper_init(&helper, &phone);
	if((phone.plugin = _gprs_init(&helper)) == NULL)
		return -1;
	g_idle_add(_gprs_on_idle, &phone);
	gtk_main();
	_gprs_destroy(phone.plugin);
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
	fputs("Usage: " PROGNAME "\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int ret;
	Config * config;
	int o;

	if((config = config_new()) == NULL)
		return 2;
	config_set(config, "gprs", "systray", "1");
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	gtk_init(&argc, &argv);
	ret = (_gprs(config) == 0) ? 0 : 2;
	config_delete(config);
	return ret;
}
