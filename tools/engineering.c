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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <System.h>
#include "../src/plugins/engineering.c"

#ifndef PROGNAME_ENGINEERING
# define PROGNAME_ENGINEERING "engineering"
#endif

#include "common.c"


/* private */
/* prototypes */
static int _engineering(void);

static int _usage(void);


/* functions */
/* engineering */
static int _engineering(void)
{
	Phone phone;

	if(_phone_init(&phone, &plugin) != 0)
		return -1;
	gtk_main();
	_engineering_destroy(phone.plugin);
	_phone_destroy(&phone);
	return 0;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_ENGINEERING "\n", stderr);
	return 1;
}


/* helpers */
/* helper_queue */
#if 0 /* FIXME re-implement */
static int _helper_queue(Phone * phone, char const * command)
{
	char const * answers1[] =
	{
		"%EM: 860,21,21,20,24,59693,13,1,0,0,0,0,0,0,0,293,0,0,2,255",
		NULL
	};
	char const * answers2[] = { "%EM: 0,0,0,0,0,255,255,0,0,0,0", NULL };
	char const * answers3[] =
	{
		"%EM: 5",
		"825,794,838,812,982,0",
		"18,21,3,20,20,0",
		"18,21,15,20,2,0",
		"19,22,16,21,21,0",
		"28,30,28,28,30,0",
		"51433,48353,48503,51453,19913,0",
		"293,293,293,293,293,0",
		"2534077,63,232981,0,22,0",
		"2524,1420,4092,0,4384,0",
		"0,0,0,0,0,0",
		"0,0,0,0,0,0",
		"2,2,2,2,2,0",
		"0,0,0,0,0,255",
		"0,0,6,0,9,0",
		"0,0,0,0,0,0",
		"3,3,15,3,3,0",
		NULL
	};
	char const * answers4[] = { "%EM: 5,120,262,003,0", NULL };
	char const ** answers;
	size_t i;

	if(strcmp(command, "AT%EM=2,1") == 0)
		answers = answers1;
	else if(strcmp(command, "AT%EM=2,2") == 0)
		answers = answers2;
	else if(strcmp(command, "AT%EM=2,3") == 0)
		answers = answers3;
	else if(strcmp(command, "AT%EM=2,4") == 0)
		answers = answers4;
	else
		return phone->callback(phone->plugin, "ERROR");
	for(i = 0; answers[i] != NULL; i++)
		if(phone->callback(phone->plugin, answers[i]) != 0)
			error_print("engineering");
	return 0;
}
#endif


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
	if((ret = (_engineering() == 0) ? 0 : 2) != 0)
		error_print(PROGNAME_ENGINEERING);
	return ret;
}
