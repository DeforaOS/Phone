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
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
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
	char * username;
	char * password;
	int fd;
	guint source;
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
	char const * homedir;
	String * path;

	if((phone->config = config_new()) == NULL)
		return -1;
	if((homedir = g_getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((path = string_new_append(homedir, "/.phone", NULL)) != 0)
	{
		if(config_load(phone->config, path) != 0)
			error_print(PROGNAME);
		string_delete(path);
	}
	memset(&phone->helper, 0, sizeof(phone->helper));
	phone->helper.phone = phone;
	phone->helper.config_get = _helper_config_get;
	phone->helper.config_set = _helper_config_set;
	phone->helper.error = _helper_error;
	phone->helper.request = _helper_request;
	phone->helper.trigger = _helper_trigger;
	phone->plugind = plugind;
	phone->plugin = NULL;
	phone->username = NULL;
	phone->password = NULL;
	phone->fd = -1;
	phone->source = 0;
	if((phone->plugin = plugind->init(&phone->helper)) == NULL)
	{
		_phone_destroy(phone);
		return -1;
	}
	return 0;
}


/* phone_destroy */
static void _phone_destroy(Phone * phone)
{
	free(phone->username);
	free(phone->password);
	if(phone->fd >= 0)
		close(phone->fd);
	if(phone->source != 0)
		g_source_remove(phone->source);
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
static void _request_call_child(gpointer data);
static void _request_call_child_watch(GPid pid, gint status, gpointer data);
static int _request_call_hangup(Phone * phone, ModemRequest * request);
static int _request_authenticate(Phone * phone, ModemRequest * request);

static int _helper_request(Phone * phone, ModemRequest * request)
{
	switch(request->type)
	{
		case MODEM_REQUEST_AUTHENTICATE:
			return _request_authenticate(phone, request);
		case MODEM_REQUEST_CALL:
			return _request_call(phone, request);
		case MODEM_REQUEST_CALL_HANGUP:
			return _request_call_hangup(phone, request);
		default:
			/* FIXME implement more */
			return -error_set_code(1, "Not implemented");
	}
}

static int _request_authenticate(Phone * phone, ModemRequest * request)
{
	char const * p;

	if(request->authenticate.name == NULL)
		return -error_set_code(1, "Unknown authentication");
	if(strcmp(request->authenticate.name, "APN") == 0)
		/* FIXME really implement */
		return 0;
	else if(strcmp(request->authenticate.name, "GPRS") == 0)
	{
		free(phone->username);
		free(phone->password);
		p = (request->authenticate.username != NULL)
			? request->authenticate.username : "";
		phone->username = strdup(p);
		p = (request->authenticate.password != NULL)
			? request->authenticate.password : "";
		phone->password = strdup(p);
		if(phone->username == NULL || phone->password == NULL)
		{
			free(phone->username);
			phone->username = NULL;
			free(phone->password);
			phone->password = NULL;
			return -error_set_code(1, "%s", strerror(errno));
		}
		return 0;
	}
	return -error_set_code(1, "Unknown authentication");
}

static int _request_call(Phone * phone, ModemRequest * request)
{
	char * argv[] = { "/usr/sbin/pppd", "pppd", "call", "gprs",
		"user", NULL, "password", NULL, NULL };
	char const * p;
	gboolean res;
	const GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO
		| G_SPAWN_DO_NOT_REAP_CHILD;
	GPid pid;
	GError * error = NULL;

	if(request->call.call_type != MODEM_CALL_TYPE_DATA)
		return -error_set_code(1, "Unknown call type");
	/* pppd */
	if((p = config_get(phone->config, "gprs", "pppd")) != NULL)
	{
		if((argv[0] = strdup(p)) == NULL)
			return -error_set_code(1, "%s", strerror(errno));
		argv[1] = basename(argv[0]);
	}
	argv[5] = phone->username;
	argv[7] = phone->password;
	res = g_spawn_async(NULL, argv, NULL, flags, _request_call_child, NULL,
			&pid, &error);
	if(p != NULL)
		free(argv[0]);
	if(res == FALSE)
	{
		error_set_code(1, "%s", error->message);
		g_error_free(error);
		return -1;
	}
	if(phone->source != 0)
		g_source_remove(phone->source);
	phone->source = g_child_watch_add(pid, _request_call_child_watch,
			phone);
	return 0;
}

static void _request_call_child(gpointer data)
{
	/* XXX lets the PID file readable with higher privileges */
	umask(022);
}

static void _request_call_child_watch(GPid pid, gint status, gpointer data)
{
	Phone * phone = data;

	phone->source = 0;
	_helper_trigger(phone, MODEM_EVENT_TYPE_CONNECTION);
}

static int _request_call_hangup(Phone * phone, ModemRequest * request)
{
	int ret = 0;
	char const * interface;
	String * path;
	FILE * fp;
	char buf[16];
	pid_t pid;

	if((interface = config_get(phone->config, "gprs", "interface")) == NULL)
		return -error_set_code(1, "Unknown interface");
	if((path = string_new_append("/var/run/", interface, ".pid", NULL))
			== NULL)
		return -1;
	if((fp = fopen(path, "r")) == NULL)
		ret = -error_set_code(1, "%s: %s", path, strerror(errno));
	else if(fread(buf, sizeof(*buf), sizeof(buf), fp) == 0)
		ret = -error_set_code(1, "%s: %s", path, strerror(errno));
	else
	{
		buf[sizeof(buf) - 1] = '\0';
		if(sscanf(buf, "%u", &pid) != 1)
			ret = -error_set_code(1, "%s", strerror(errno));
		else if(kill(pid, SIGHUP) != 0)
			ret = -error_set_code(1, "%u: %s", pid,
					strerror(errno));
	}
	if(fp != NULL)
		fclose(fp);
	string_delete(path);
	return ret;
}


/* helper_trigger */
static int _trigger_connection(Phone * phone, ModemEventType type);
#if defined(SIOCGIFDATA) || defined(SIOCGIFFLAGS)
static int _trigger_connection_interface(Phone * phone, PhoneEvent * event,
		char const * interface);
#endif

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
	char const * p;
#endif

	if(phone->source != 0)
		/* wait for the result of the connection */
		return 0;
	memset(&pevent, 0, sizeof(pevent));
	memset(&mevent, 0, sizeof(mevent));
	pevent.type = PHONE_EVENT_TYPE_MODEM_EVENT;
	pevent.modem_event.event = &mevent;
	mevent.type = type;
	mevent.connection.connected = FALSE;
	mevent.connection.in = 0;
	mevent.connection.out = 0;
#if defined(SIOCGIFDATA) || defined(SIOCGIFFLAGS)
	if((p = config_get(phone->config, "gprs", "interface")) != NULL)
		/* XXX ignore errors */
		_trigger_connection_interface(phone, &pevent, p);
#endif
	return phone->plugind->event(phone->plugin, &pevent);
}

#if defined(SIOCGIFDATA) || defined(SIOCGIFFLAGS)
static int _trigger_connection_interface(Phone * phone, PhoneEvent * event,
		char const * interface)
{
	ModemEvent * mevent = event->modem_event.event;
# ifdef SIOCGIFDATA
	struct ifdatareq ifdr;
# endif
# ifdef SIOCGIFFLAGS
	struct ifreq ifr;
# endif

	if(phone->fd < 0 && (phone->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -error_set_print(PROGNAME, 1, "%s", strerror(errno));
# ifdef SIOCGIFDATA
	memset(&ifdr, 0, sizeof(ifdr));
	strncpy(ifdr.ifdr_name, interface, sizeof(ifdr.ifdr_name));
	if(ioctl(phone->fd, SIOCGIFDATA, &ifdr) == -1)
		error_set_print(PROGNAME, 1, "%s: %s", interface,
				strerror(errno));
	else
	{
		mevent->connection.connected = TRUE;
		mevent->connection.in = ifdr.ifdr_data.ifi_ibytes;
		mevent->connection.out = ifdr.ifdr_data.ifi_obytes;
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
		mevent->connection.connected = (ifr.ifr_flags & IFF_UP)
			? TRUE : FALSE;
#  endif
	}
# endif
	return 0;
}
#endif
