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



#include "../src/modems/hayes/command.c"
#include "../src/modems/hayes/quirks.c"
#include "../src/modems/hayes.c"

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


/* functions */
/* hayes */
static int _hayes(void)
{
	Modem modem;
	ModemPluginHelper helper;
	Hayes * hayes;

	if((modem.config = config_new()) == NULL)
		return -error_print(PROGNAME);
	memset(&helper, 0, sizeof(helper));
	helper.modem = &modem;
	helper.config_get = _hayes_helper_config_get;
	helper.config_set = _hayes_helper_config_set;
	if((hayes = plugin.init(&helper)) == NULL)
		return -1;
	if(plugin.start(hayes, 0) == 0)
		plugin.stop(hayes);
	plugin.destroy(hayes);
	config_delete(modem.config);
	return 0;
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


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	return (_hayes() == 0) ? 0 : 2;
}
