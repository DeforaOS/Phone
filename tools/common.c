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
/* helpers */
static char const * _helper_config_get(Phone * phone, char const * section,
		char const * variable);
static int _helper_config_set(Phone * phone, char const * section,
		char const * variable, char const * value);
static int _helper_trigger(Phone * phone, ModemEventType event);


/* functions */
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


/* helper_trigger */
static int _helper_trigger(Phone * phone, ModemEventType event)
{
	return 0;
}
