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



#include <unistd.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>
#include <glib.h>
#include <System.h>
#include <Phone/modem.h>

#include <dev/usb/mbim.h>
#include <dev/usb/if_umbreg.h>

#ifndef SIOCGUMBINFO
# define SIOCGUMBINFO	_IOWR('i', 190, struct ifreq)	/* get MBIM info */
#endif
#ifndef SIOCSUMBPARAM
# define SIOCSUMBPARAM	_IOW('i', 191, struct ifreq)	/* set MBIM param */
#endif
#ifndef SIOCGUMBPARAM
# define SIOCGUMBPARAM	_IOWR('i', 192, struct ifreq)	/* get MBIM param */
#endif


/* MBIM */
/* private */
/* types */
typedef struct _ModemPlugin
{
	ModemPluginHelper * helper;

	int fd;

	unsigned int source;
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

static int _mbim_ioctl(ModemPlugin * modem, unsigned long request,
		struct ifreq * ifr);

/* callbacks */
static gboolean _mbim_on_timeout(gpointer data);


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
	mbim->fd = -1;
	mbim->source = 0;
	return mbim;
}


/* mbim_destroy */
static void _mbim_destroy(ModemPlugin * modem)
{
	MBIM * mbim = modem;

	_mbim_stop(modem);
	object_delete(mbim);
}


/* mbim_start */
static int _mbim_start(ModemPlugin * modem, unsigned int retry)
{
	MBIM * mbim = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(mbim->fd < 0 && (mbim->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	mbim->source = g_timeout_add(1000, _mbim_on_timeout, mbim);
	return 0;
}


/* mbim_stop */
static int _mbim_stop(ModemPlugin * modem)
{
	MBIM * mbim = modem;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(mbim->source > 0)
		g_source_remove(mbim->source);
	mbim->source = 0;
	if(mbim->fd >= 0)
		close(mbim->fd);
	mbim->fd = -1;
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


/* mbim_ioctl */
static int _mbim_ioctl(ModemPlugin * modem, unsigned long request,
		struct ifreq * ifr)
{
	MBIM * mbim = modem;

	if(ioctl(mbim->fd, request, ifr) != 0)
		return -1;
	return 0;
}


/* callbacks */
/* mbim_on_timeout */
static gboolean _mbim_on_timeout(gpointer data)
{
	ModemPlugin * mbim = data;
	ModemPluginHelper * helper = mbim->helper;
	struct ifreq ifr;
	struct umb_info umbi;
	char const * p;

	if((p = helper->config_get(helper->modem, "interface")) == NULL
			|| strlen(p) == 0)
		p = "umb0";
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, p, sizeof(ifr.ifr_name));
	memset(&umbi, 0, sizeof(umbi));
	ifr.ifr_data = &umbi;
	if(_mbim_ioctl(mbim, SIOCGUMBINFO, &ifr) != 0)
	{
		mbim->helper->error(mbim->helper->modem, strerror(errno),
				-errno);
		return TRUE;
	}
	return TRUE;
}
