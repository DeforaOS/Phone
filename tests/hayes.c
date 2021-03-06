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



#include "../src/modems/hayes/channel.c"
#include "../src/modems/hayes/command.c"
#include "../src/modems/hayes/common.c"
#include "../src/modems/hayes/pdu.c"
#include "../src/modems/hayes/quirks.c"
#include "../src/modems/hayes.c"
#include "../config.h"

#ifndef PROGNAME
# define PROGNAME "hayes"
#endif


/* private */
/* types */
struct _Modem
{
	Config * config;
};


/* prototypes */
static int _hayes(void);

static char const * _hayes_helper_config_get(Modem * modem,
		char const * variable);
static int _hayes_helper_config_set(Modem * modem, char const * variable,
		char const * value);
static int _hayes_helper_error(Modem * modem, char const * message, int ret);
static void _hayes_helper_event(Modem * modem, ModemEvent * event);


/* variables */
static GMainLoop * _loop;


/* functions */
/* hayes */
static gboolean _hayes_on_start(gpointer data);
static gboolean _hayes_on_stop(gpointer data);

static int _hayes(void)
{
	Modem modem;
	ModemPluginHelper helper;

	if((modem.config = config_new()) == NULL)
		return -error_print(PROGNAME);
	config_set(modem.config, NULL, "device", "/dev/null");
	config_set(modem.config, NULL, "hwflow", "0");
	memset(&helper, 0, sizeof(helper));
	helper.modem = &modem;
	helper.config_get = _hayes_helper_config_get;
	helper.config_set = _hayes_helper_config_set;
	helper.error = _hayes_helper_error;
	helper.event = _hayes_helper_event;
	_loop = g_main_loop_new(NULL, FALSE);
	g_idle_add(_hayes_on_start, &helper);
	g_main_loop_run(_loop);
	g_main_loop_unref(_loop);
	config_delete(modem.config);
	return 0;
}

static gboolean _hayes_on_start(gpointer data)
{
	ModemPluginHelper * helper = data;
	Hayes * hayes;

	if((hayes = plugin.init(helper)) == NULL)
		return FALSE;
	if(plugin.start(hayes, 0) == 0)
	{
		_hayes_parse(hayes, &hayes->channel);
		_on_code_cfun(&hayes->channel, "0");
		_on_code_cfun(&hayes->channel, "1");
		_on_code_cfun(&hayes->channel, "4");
		_on_code_cgmi(&hayes->channel, "DeforaOS");
		_on_code_cgmm(&hayes->channel, PACKAGE);
		_on_code_cgmr(&hayes->channel, VERSION);
		_on_code_cgsn(&hayes->channel, "IMEI");
		_on_code_cimi(&hayes->channel, "IMSI");
		_on_request_model(NULL, HCS_SUCCESS, &hayes->channel);
		_on_code_csq(&hayes->channel, "31,99");
		_on_code_csq(&hayes->channel, "20,99");
		_on_code_csq(&hayes->channel, "19,99");
		_on_code_csq(&hayes->channel, "18,99");
		_on_code_csq(&hayes->channel, "17,99");
		_on_code_csq(&hayes->channel, "16,99");
		_on_code_csq(&hayes->channel, "15,99");
		_on_code_csq(&hayes->channel, "14,99");
		_on_code_csq(&hayes->channel, "13,99");
		_on_code_csq(&hayes->channel, "12,99");
		_on_code_csq(&hayes->channel, "11,99");
		_on_code_csq(&hayes->channel, "10,99");
		_on_code_csq(&hayes->channel, "9,99");
		_on_code_csq(&hayes->channel, "8,99");
		_on_code_csq(&hayes->channel, "7,99");
		_on_code_csq(&hayes->channel, "6,99");
		_on_code_csq(&hayes->channel, "5,99");
		_on_code_csq(&hayes->channel, "4,99");
		_on_code_csq(&hayes->channel, "3,99");
		_on_code_csq(&hayes->channel, "2,99");
		_on_code_csq(&hayes->channel, "1,99");
		_on_code_csq(&hayes->channel, "0,99");
		g_timeout_add(1000, _hayes_on_stop, hayes);
	}
	else
		g_main_loop_quit(_loop);
	return FALSE;
}

static gboolean _hayes_on_stop(gpointer data)
{
	Hayes * hayes = data;

	plugin.stop(hayes);
	plugin.destroy(hayes);
	g_main_loop_quit(_loop);
	return FALSE;
}

/* helpers */
/* hayes_helper_config_get */
static char const * _hayes_helper_config_get(Modem * modem,
		char const * variable)
{
	return config_get(modem->config, NULL, variable);
}


/* hayes_helper_config_set */
static int _hayes_helper_config_set(Modem * modem, char const * variable,
		char const * value)
{
	return config_set(modem->config, NULL, variable, value);
}


/* hayes_helper_error */
static int _hayes_helper_error(Modem * modem, char const * message, int ret)
{
	(void) modem;

	fprintf(stderr, "%s: %s\n", PROGNAME, message);
	return ret;
}


/* hayes_helper_event */
static void _hayes_helper_event(Modem * modem, ModemEvent * event)
{
	(void) modem;

	switch(event->type)
	{
		case MODEM_EVENT_TYPE_MODEL:
			printf("%s=%s\n", "modem.event.model.vendor",
					event->model.vendor);
			printf("%s=%s\n", "modem.event.model.name",
					event->model.name);
			printf("%s=%s\n", "modem.event.model.serial",
					event->model.serial);
			printf("%s=%s\n", "modem.event.model.version",
					event->model.version);
			printf("%s=%s\n", "modem.event.model.identity",
					event->model.identity);
			break;
		case MODEM_EVENT_TYPE_REGISTRATION:
			printf("%s=%f\n", "modem.event.registration.signal",
					event->registration.signal);
			break;
		case MODEM_EVENT_TYPE_STATUS:
			printf("%s=%u\n", "modem.event.status.status",
					event->status.status);
		default:
			break;
	}
}


/* public */
/* functions */
/* main */
int main(void)
{
	return (_hayes() == 0) ? 0 : 2;
}
