/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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



#include <stddef.h>
#include "common.h"


/* HayesCommon */
/* public */
/* functions */
/* hayescommon_number_is_valid */
static int _is_figure(int c);

int hayescommon_number_is_valid(char const * number)
{
	if(number == NULL || number[0] == '\0')
		return 0;
	while(*number != '\0')
		if(!_is_figure(*(number++)))
			return 0;
	return 1;
}

static int _is_figure(int c)
{
	if(c >= '0' && c <= '9')
		return 1;
	if(c == '*' || c == '+' || c == '#')
		return 1;
	if(c >= 'A' && c <= 'D')
		return 1;
	return 0;
}


/* hayescommon_source_reset */
void hayescommon_source_reset(guint * source)
{
	if(*source == 0)
		return;
	g_source_remove(*source);
	*source = 0;
}
