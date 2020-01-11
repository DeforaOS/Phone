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
	(void) phone;
	(void) section;
	(void) variable;

	return NULL;
}

static int _oss_error(Phone * phone, char const * message, int ret)
{
	(void) phone;

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
