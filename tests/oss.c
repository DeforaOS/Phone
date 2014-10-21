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



#include <unistd.h>
#include <stdio.h>
#include "../src/plugins/oss.c"


/* private */
/* prototypes */
static int _oss(char const * filename);

static int _usage(void);


/* functions */
/* plugins */
static char const * _oss_config_get(Phone * phone, char const * section,
		char const * variable);
static int _oss_error(Phone * phone, char const * message, int ret);

static int _oss(char const * filename)
{
	OSS * oss;
	PhonePluginHelper helper;
	PhoneEvent event;

	memset(&helper, 0, sizeof(helper));
	helper.config_get = _oss_config_get;
	helper.error = _oss_error;
	if((oss = _oss_init(&helper)) == NULL)
		return 2;
	event.type = PHONE_EVENT_TYPE_AUDIO_PLAY;
	event.audio_play.sample = filename;
	_oss_event(oss, &event);
	_oss_destroy(oss);
	return 0;
}

static char const * _oss_config_get(Phone * phone, char const * section,
		char const * variable)
{
	return NULL;
}

static int _oss_error(Phone * phone, char const * message, int ret)
{
	fprintf(stderr, "oss: %s\n", message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: oss sample...\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int ret = 0;
	int o;

	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind == argc)
		return _usage();
	while(optind < argc)
		if(_oss(argv[optind++]) != 0)
			ret = 2;
	return ret;
}
