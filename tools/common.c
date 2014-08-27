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



/* private */
/* types */
struct _Phone
{
	Config * config;
	PhonePluginDefinition * plugind;
	PhonePlugin * plugin;
};


/* prototypes */
static void _helper_init(PhonePluginHelper * helper, Phone * phone);

/* helpers */
static char const * _helper_config_get(Phone * phone, char const * section,
		char const * variable);
static int _helper_config_set(Phone * phone, char const * section,
		char const * variable, char const * value);
static int _helper_error(Phone * phone, char const * message, int ret);
static int _helper_request(Phone * phone, ModemRequest * request);
static int _helper_trigger(Phone * phone, ModemEventType event);


/* functions */
/* helper_init */
static void _helper_init(PhonePluginHelper * helper, Phone * phone)
{
	memset(helper, 0, sizeof(*helper));
	helper->phone = phone;
	helper->config_get = _helper_config_get;
	helper->config_set = _helper_config_set;
	helper->error = _helper_error;
	helper->request = _helper_request;
	helper->trigger = _helper_trigger;
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
static int _helper_trigger(Phone * phone, ModemEventType event)
{
	return 0;
}
