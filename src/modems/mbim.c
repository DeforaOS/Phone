/* $Id$ */
/* Copyright (c) 2018 Pierre Pronchery <khorben@defora.org> */
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


/* MBIM */
/* private */
/* types */
typedef struct _ModemPlugin
{
	ModemPluginHelper * helper;
} MBIM;


/* variables */
static ModemConfig _mbim_config[] =
{
	{ "interface",	"Interface",		MCT_STRING	},
	{ NULL,		NULL,			MCT_NONE	},
};


/* prototypes */
/* plug-in */
static ModemPlugin * _mbim_init(ModemPluginHelper * helper);
static void _mbim_destroy(ModemPlugin * modem);
static int _mbim_start(ModemPlugin * modem, unsigned int retry);
static int _mbim_stop(ModemPlugin * modem);
static int _mbim_request(ModemPlugin * modem, ModemRequest * request);


/* public */
/* variables */
ModemPluginDefinition plugin =
{
	"MBIM",
	"applications-development",
	_mbim_config,
	_mbim_init,
	_mbim_destroy,
	_mbim_start,
	_mbim_stop,
	_mbim_request,
	NULL
};


/* private */
/* functions */
/* mbim_init */
static ModemPlugin * _mbim_init(ModemPluginHelper * helper)
{
	MBIM * mbim;

	if((mbim = object_new(sizeof(*mbim))) == NULL)
		return NULL;
	memset(mbim, 0, sizeof(*mbim));
	mbim->helper = helper;
	/* TODO implement */
	return mbim;
}


/* mbim_destroy */
static void _mbim_destroy(ModemPlugin * modem)
{
	MBIM * mbim = modem;

	_mbim_stop(modem);
	/* TODO implement */
	object_delete(mbim);
}


/* mbim_start */
static int _mbim_start(ModemPlugin * modem, unsigned int retry)
{
	MBIM * mbim = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* TODO implement */
	return 0;
}


/* mbim_stop */
static int _mbim_stop(ModemPlugin * modem)
{
	MBIM * mbim = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* TODO implement */
	return 0;
}


/* mbim_request */
static int _mbim_request(ModemPlugin * modem, ModemRequest * request)
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
