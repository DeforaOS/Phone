/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
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
	/* FIXME implement */
	return template;
}


/* template_destroy */
static void _template_destroy(ModemPlugin * modem)
{
	Template * template = modem;

	_template_stop(modem);
	/* FIXME implement */
	object_delete(template);
}


/* template_start */
static int _template_start(ModemPlugin * modem, unsigned int retry)
{
	Template * template = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* FIXME implement */
	return 0;
}


/* template_stop */
static int _template_stop(ModemPlugin * modem)
{
	Template * template = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* FIXME implement */
	return 0;
}


/* template_request */
static int _template_request(ModemPlugin * modem, ModemRequest * request)
{
	switch(request->type)
	{
		/* FIXME implement */
#ifndef DEBUG
		default:
			break;
#endif
	}
	return 0;
}
