/* $Id$ */
/* Copyright (c) 2013-2017 Pierre Pronchery <khorben@defora.org> */
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



#include <string.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <System.h>
#include <Phone/modem.h>


/* Template */
/* private */
/* types */
typedef struct _ModemPlugin
{
	ModemPluginHelper * helper;
} Template;


/* variables */
static ModemConfig _template_config[] =
{
	{ NULL,			NULL,		MCT_NONE	},
};


/* prototypes */
/* plug-in */
static ModemPlugin * _template_init(ModemPluginHelper * helper);
static void _template_destroy(ModemPlugin * modem);
static int _template_start(ModemPlugin * modem, unsigned int retry);
static int _template_stop(ModemPlugin * modem);
static int _template_request(ModemPlugin * modem, ModemRequest * request);


/* public */
/* variables */
ModemPluginDefinition plugin =
{
	"Template",
	"applications-development",
	_template_config,
	_template_init,
	_template_destroy,
	_template_start,
	_template_stop,
	_template_request,
	NULL
};


/* private */
/* functions */
/* template_init */
static ModemPlugin * _template_init(ModemPluginHelper * helper)
{
	Template * template;

	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	memset(template, 0, sizeof(*template));
	template->helper = helper;
	/* TODO implement */
	return template;
}


/* template_destroy */
static void _template_destroy(ModemPlugin * modem)
{
	Template * template = modem;

	_template_stop(modem);
	/* TODO implement */
	object_delete(template);
}


/* template_start */
static int _template_start(ModemPlugin * modem, unsigned int retry)
{
	Template * template = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* TODO implement */
	return 0;
}


/* template_stop */
static int _template_stop(ModemPlugin * modem)
{
	Template * template = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* TODO implement */
	return 0;
}


/* template_request */
static int _template_request(ModemPlugin * modem, ModemRequest * request)
{
	switch(request->type)
	{
		/* TODO implement */
#ifndef DEBUG
		default:
			break;
#endif
	}
	return 0;
}
