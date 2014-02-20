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



#ifndef PHONE_MODEM_HAYES_COMMAND_H
# define PHONE_MODEM_HAYES_COMMAND_H


/* HayesCommand */
/* public */
/* types */
typedef enum _HayesCommandPriority
{
	HCP_LOWER = 0,
	HCP_NORMAL,
	HCP_HIGHER,
	HCP_IMMEDIATE
} HayesCommandPriority;

typedef enum _HayesCommandStatus
{
	HCS_PENDING = 0,
	HCS_QUEUED,
	HCS_ACTIVE,
	HCS_TIMEOUT,
	HCS_ERROR,
	HCS_SUCCESS
} HayesCommandStatus;
#define HCS_LAST HCS_SUCCESS
#define HCS_COUNT (HCS_LAST + 1)

typedef struct _HayesCommand HayesCommand;

typedef HayesCommandStatus (*HayesCommandCallback)(HayesCommand * command,
		HayesCommandStatus status, void * priv);


/* prototypes */
HayesCommand * hayes_command_new(char const * attention);
HayesCommand * hayes_command_new_copy(HayesCommand const * command);
void hayes_command_delete(HayesCommand * command);

/* accessors */
char const * hayes_command_get_answer(HayesCommand * command);
char const * hayes_command_get_attention(HayesCommand * command);
void * hayes_command_get_data(HayesCommand * command);
#if 0 /* XXX no longer being used */
char * hayes_command_get_line(HayesCommand * command,
		char const * prefix);
#endif
HayesCommandPriority hayes_command_get_priority(HayesCommand * command);
HayesCommandStatus hayes_command_get_status(HayesCommand * command);
unsigned int hayes_command_get_timeout(HayesCommand * command);
void hayes_command_set_callback(HayesCommand * command,
		HayesCommandCallback callback, void * priv);
void hayes_command_set_data(HayesCommand * command, void * data);
void hayes_command_set_priority(HayesCommand * command,
		HayesCommandPriority priority);
void hayes_command_set_status(HayesCommand * command,
		HayesCommandStatus status);
void hayes_command_set_timeout(HayesCommand * command,
		unsigned int timeout);
int hayes_command_answer_append(HayesCommand * command,
		char const * answer);
HayesCommandStatus hayes_command_callback(HayesCommand * command);

#endif /* PHONE_MODEM_HAYES_COMMAND_H */
