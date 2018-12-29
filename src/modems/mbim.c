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
#include <ctype.h>
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
	struct ifreq ifr;

	char provider[UMB_PROVIDERNAME_MAXLEN + 1];

	unsigned int source;
} MBIM;

struct umb_valenum {
	int		 val;
	int		 conv;
};


/* constants */
static const struct umb_valdescr _mbim_dataclass[] =
	MBIM_DATACLASS_DESCRIPTIONS;
static const struct umb_valenum _mbim_regstate[] =
{
	{ MBIM_REGSTATE_UNKNOWN,	MODEM_REGISTRATION_STATUS_UNKNOWN },
	{ MBIM_REGSTATE_SEARCHING,	MODEM_REGISTRATION_STATUS_SEARCHING },
	{ MBIM_REGSTATE_HOME,		MODEM_REGISTRATION_STATUS_REGISTERED },
	{ MBIM_REGSTATE_ROAMING,	MODEM_REGISTRATION_STATUS_REGISTERED },
	{ MBIM_REGSTATE_PARTNER,	MODEM_REGISTRATION_STATUS_REGISTERED },
	{ MBIM_REGSTATE_DENIED,		MODEM_REGISTRATION_STATUS_DENIED },
	{ 0,				-1 }
};


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
static int _mbim_trigger(ModemPlugin * modem, ModemEventType event);

/* useful */
static int _mbim_ioctl(ModemPlugin * modem, unsigned long request, void * data);

/* callbacks */
static gboolean _mbim_on_timeout(gpointer data);

static int umb_val2enum(const struct umb_valenum *vdp, int val);

static void _utf16_to_char(uint16_t *in, int inlen, char *out, size_t outlen);


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
	_mbim_trigger
};


/* private */
/* functions */
/* mbim_init */
static ModemPlugin * _mbim_init(ModemPluginHelper * helper)
{
	MBIM * mbim;
	char const * p;

	if((mbim = object_new(sizeof(*mbim))) == NULL)
		return NULL;
	memset(mbim, 0, sizeof(*mbim));
	mbim->helper = helper;
	mbim->fd = -1;
	if((p = helper->config_get(helper->modem, "interface")) == NULL
			|| strlen(p) == 0)
		p = "umb0";
	strlcpy(mbim->ifr.ifr_name, p, sizeof(mbim->ifr.ifr_name));
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


/* mbim_trigger */
static int _trigger_registration(MBIM * mbim);

static int _mbim_trigger(ModemPlugin * modem, ModemEventType event)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, event);
#endif
	switch(event)
	{
		case MODEM_EVENT_TYPE_REGISTRATION:
			return _trigger_registration(modem);
		case MODEM_EVENT_TYPE_ERROR:
			break;
		/* TODO implement the rest */
#ifndef DEBUG
		default:
			break;
#endif
	}
	return -1;
}

static int _trigger_registration(MBIM * mbim)
{
	ModemEvent event;
	struct umb_info umbi;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(_mbim_ioctl(mbim, SIOCGUMBINFO, &umbi) != 0)
		return mbim->helper->error(mbim->helper->modem, strerror(errno),
				-errno);
	memset(&event, 0, sizeof(event));
	event.type = MODEM_EVENT_TYPE_REGISTRATION;
	event.registration.mode = umb_val2enum(_mbim_regstate, umbi.regstate);
	event.registration.status = MODEM_REGISTRATION_STATUS_UNKNOWN;
	event.registration.media = umb_val2descr(_mbim_dataclass,
			umbi.cellclass);
	_utf16_to_char(umbi.provider, UMB_PROVIDERNAME_MAXLEN,
			mbim->provider, sizeof(mbim->provider));
	event.registration._operator = mbim->provider;
	event.registration.signal = 0.0 / 0.0;
	event.registration.roaming = (umbi.regstate == MBIM_REGSTATE_ROAMING)
		? 1 : 0;
	mbim->helper->event(mbim->helper->modem, &event);
	return 0;
}


/* useful */
/* mbim_ioctl */
static int _mbim_ioctl(ModemPlugin * modem, unsigned long request,
		void * data)
{
	MBIM * mbim = modem;

	mbim->ifr.ifr_data = data;
	if(ioctl(mbim->fd, request, &mbim->ifr) != 0)
		return -1;
	return 0;
}


/* callbacks */
/* mbim_on_timeout */
static gboolean _mbim_on_timeout(gpointer data)
{
	ModemPlugin * mbim = data;
	ModemPluginHelper * helper = mbim->helper;
	struct umb_info umbi;

	memset(&umbi, 0, sizeof(umbi));
	if(_mbim_ioctl(mbim, SIOCGUMBINFO, &umbi) != 0)
	{
		helper->error(helper->modem, strerror(errno), -errno);
		return TRUE;
	}
	return TRUE;
}


/* umb_val2enum */
static int umb_val2enum(const struct umb_valenum *vdp, int val)
{
	while (vdp->conv != -1) {
		if (vdp->val == val)
			return vdp->conv;
		vdp++;
	}
	return -1;
}


/* utf16_to_char */
static void _utf16_to_char(uint16_t *in, int inlen, char *out, size_t outlen)
{
	uint16_t c;

	while (outlen > 0) {
		c = inlen > 0 ? htole16(*in) : 0;
		if (c == 0 || --outlen == 0) {
			/* always NUL terminate result */
			*out = '\0';
			break;
		}
		*out++ = isascii(c) ? (char)c : '?';
		in++;
		inlen--;
	}
}
