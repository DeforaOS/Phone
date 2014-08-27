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



#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>


/* private */
/* types */
struct _Phone
{
	Config * config;
	PhonePluginHelper helper;
	PhonePluginDefinition * plugind;
	PhonePlugin * plugin;
	int fd;
};


/* prototypes */
static int _phone_init(Phone * phone, PhonePluginDefinition * plugind);
static void _phone_destroy(Phone * phone);

/* helpers */
static char const * _helper_config_get(Phone * phone, char const * section,
		char const * variable);
static int _helper_config_set(Phone * phone, char const * section,
		char const * variable, char const * value);
static int _helper_error(Phone * phone, char const * message, int ret);
static int _helper_request(Phone * phone, ModemRequest * request);
static int _helper_trigger(Phone * phone, ModemEventType event);


/* functions */
/* phone_init */
static int _phone_init(Phone * phone, PhonePluginDefinition * plugind)
{
	if((phone->config = config_new()) == NULL)
		return -1;
	memset(&phone->helper, 0, sizeof(phone->helper));
	phone->helper.phone = phone;
	phone->helper.config_get = _helper_config_get;
	phone->helper.config_set = _helper_config_set;
	phone->helper.error = _helper_error;
	phone->helper.request = _helper_request;
	phone->helper.trigger = _helper_trigger;
	phone->plugind = plugind;
	phone->plugin = NULL;
	phone->fd = -1;
	return 0;
}


/* phone_destroy */
static void _phone_destroy(Phone * phone)
{
	if(phone->fd >= 0)
		close(phone->fd);
}


/* helpers */
/* helper_config_get */
static char const * _helper_config_get(Phone * phone, char const * section,
		char const * variable)
{
	return config_get(phone->config, section, variable);
}


/* helper_config_set */
static int _helper_config_set(Phone * phone, char const * section,
		char const * variable, char const * value)
{
	return config_set(phone->config, section, variable, value);
}


/* helper_error */
static int _helper_error(Phone * phone, char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", PROGNAME, message);
	return ret;
}


/* helper_request */
static int _request_call(Phone * phone, ModemRequest * request);
static int _request_authenticate(Phone * phone, ModemRequest * request);

static int _helper_request(Phone * phone, ModemRequest * request)
{
	switch(request->type)
	{
		case MODEM_REQUEST_AUTHENTICATE:
			return _request_authenticate(phone, request);
		case MODEM_REQUEST_CALL:
			return _request_call(phone, request);
		default:
			/* FIXME implement more */
			return -error_set_code(1, "Not implemented");
	}
}

static int _request_authenticate(Phone * phone, ModemRequest * request)
{
	if(request->authenticate.name == NULL)
		return -error_set_code(1, "Unknown authentication");
	if(strcmp(request->authenticate.name, "APN") == 0
			|| strcmp(request->authenticate.name, "GPRS") == 0)
		/* FIXME really implement */
		return 0;
	return -error_set_code(1, "Unknown authentication");
}

static int _request_call(Phone * phone, ModemRequest * request)
{
	if(request->call.call_type != MODEM_CALL_TYPE_DATA)
		return -error_set_code(1, "Unknown call type");
	/* FIXME really implement */
	return 0;
}


/* helper_trigger */
static int _trigger_connection(Phone * phone, ModemEventType type);

static int _helper_trigger(Phone * phone, ModemEventType event)
{
	switch(event)
	{
		case MODEM_EVENT_TYPE_CONNECTION:
			return _trigger_connection(phone, event);
		default:
			/* FIXME implement more */
			return 0;
	}
}

static int _trigger_connection(Phone * phone, ModemEventType type)
{
	PhoneEvent pevent;
	ModemEvent mevent;
#if defined(SIOCGIFDATA) || defined(SIOCGIFFLAGS)
	/* FIXME no longer hardcode the interface */
	const char interface[] = "ppp0";
# ifdef SIOCGIFDATA
	struct ifdatareq ifdr;
# endif
# ifdef SIOCGIFFLAGS
	struct ifreq ifr;
# endif
#endif

	memset(&pevent, 0, sizeof(pevent));
	memset(&mevent, 0, sizeof(mevent));
	pevent.type = PHONE_EVENT_TYPE_MODEM_EVENT;
	pevent.modem_event.event = &mevent;
	mevent.type = type;
	mevent.connection.connected = FALSE;
	mevent.connection.in = 0;
	mevent.connection.out = 0;
#if defined(SIOCGIFDATA) || defined(SIOCGIFFLAGS)
	if(phone->fd < 0)
		if((phone->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			error_set_print(PROGNAME, 1, "%s", strerror(errno));
	if(phone->fd >= 0)
	{
# ifdef SIOCGIFDATA
		memset(&ifdr, 0, sizeof(ifdr));
		strncpy(ifdr.ifdr_name, interface, sizeof(ifdr.ifdr_name));
		if(ioctl(phone->fd, SIOCGIFDATA, &ifdr) == -1)
			error_set_print(PROGNAME, 1, "%s: %s", interface,
					strerror(errno));
		else
		{
			mevent.connection.connected = TRUE;
			mevent.connection.in = ifdr.ifdr_data.ifi_ibytes;
			mevent.connection.out = ifdr.ifdr_data.ifi_obytes;
		}
# endif
# ifdef SIOCGIFFLAGS
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
		if(ioctl(phone->fd, SIOCGIFFLAGS, &ifr) == -1)
			error_set_print(PROGNAME, 1, "%s: %s", interface,
					strerror(errno));
		else
		{
#  ifdef IFF_UP
			mevent.connection.connected = (ifr.ifr_flags & IFF_UP)
				? TRUE : FALSE;
#  endif
		}
# endif
#endif
	}
	return phone->plugind->event(phone->plugin, &pevent);
}
