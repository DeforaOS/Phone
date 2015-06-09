/* $Id$ */
/* Copyright (c) 2014-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <System.h>
#include "command.h"


/* HayesCommand */
/* private */
/* types */
struct _HayesCommand
{
	HayesCommandPriority priority;
	HayesCommandStatus status;

	/* request */
	String * attention;
	unsigned int timeout;
	HayesCommandCallback callback;
	void * priv;

	/* answer */
	String * answer;

	/* XXX should be handled a better way */
	void * data;
};


/* public */
/* functions */
/* hayes_command_new */
HayesCommand * hayes_command_new(char const * attention)
{
	HayesCommand * command;

	if((command = object_new(sizeof(*command))) == NULL)
		return NULL;
	command->priority = HCP_NORMAL;
	command->status = HCS_UNKNOWN;
	command->attention = string_new(attention);
	command->timeout = 30000;
	command->callback = NULL;
	command->priv = NULL;
	command->answer = NULL;
	command->data = NULL;
	if(command->attention == NULL)
	{
		hayes_command_delete(command);
		return NULL;
	}
	return command;
}


/* hayes_command_new_copy */
HayesCommand * hayes_command_new_copy(HayesCommand const * command)
{
	HayesCommand * ret;

	if((ret = hayes_command_new(command->attention)) == NULL)
		return NULL;
	ret->priority = command->priority;
	ret->timeout = command->timeout;
	ret->callback = command->callback;
	ret->priv = command->priv;
	return ret;
}


/* hayes_command_delete */
void hayes_command_delete(HayesCommand * command)
{
	string_delete(command->attention);
	string_delete(command->answer);
	object_delete(command);
}


/* accessors */
/* hayes_command_get_answer */
char const * hayes_command_get_answer(HayesCommand * command)
{
	return command->answer;
}


/* hayes_command_get_attention */
char const * hayes_command_get_attention(HayesCommand * command)
{
	return command->attention;
}


/* hayes_command_get_data */
void * hayes_command_get_data(HayesCommand * command)
{
	return command->data;
}


/* hayes_command_get_priority */
HayesCommandPriority hayes_command_get_priority(HayesCommand * command)
{
	return command->priority;
}


/* hayes_command_get_status */
HayesCommandStatus hayes_command_get_status(HayesCommand * command)
{
	return command->status;
}


/* hayes_command_get_timeout */
unsigned int hayes_command_get_timeout(HayesCommand * command)
{
	return command->timeout;
}


/* hayes_command_is_complete */
int hayes_command_is_complete(HayesCommand * command)
{
	return command->status == HCS_SUCCESS
		|| command->status == HCS_ERROR
		|| command->status == HCS_TIMEOUT;
}


/* hayes_command_set_callback */
void hayes_command_set_callback(HayesCommand * command,
		HayesCommandCallback callback, void * priv)
{
	command->callback = callback;
	command->priv = priv;
}


/* hayes_command_set_id */
void hayes_command_set_data(HayesCommand * command, void * data)
{
	command->data = data;
}


/* hayes_command_set_priority */
void hayes_command_set_priority(HayesCommand * command,
		HayesCommandPriority priority)
{
	command->priority = priority;
}


/* hayes_command_set_status */
HayesCommandStatus hayes_command_set_status(HayesCommand * command,
		HayesCommandStatus status)
{
	command->status = status;
	return hayes_command_callback(command);
}


/* hayes_command_set_timeout */
void hayes_command_set_timeout(HayesCommand * command,
		unsigned int timeout)
{
	command->timeout = timeout;
}


/* useful */
/* hayes_command_answer_append */
int hayes_command_answer_append(HayesCommand * command,
		char const * answer)
{
	String * p;

	if(answer == NULL)
		return 0;
	if(command->answer == NULL)
		p = string_new(answer);
	else
		p = string_new_append(command->answer, "\n", answer, NULL);
	if(p == NULL)
		return -1;
	string_delete(command->answer);
	command->answer = p;
	return 0;
}


/* hayes_command_callback */
HayesCommandStatus hayes_command_callback(HayesCommand * command)
{
	if(command->callback != NULL)
		command->status = command->callback(command, command->status,
				command->priv);
	return command->status;
}
