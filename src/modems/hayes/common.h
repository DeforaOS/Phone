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



#ifndef PHONE_MODEM_HAYES_COMMON_H
# define PHONE_MODEM_HAYES_COMMON_H

# include <glib.h>


/* HayesCommon */
/* public */
/* functions */
int hayescommon_number_is_valid(char const * number);

void hayescommon_source_reset(guint * source);

#endif /* PHONE_MODEM_HAYES_COMMON_H */
