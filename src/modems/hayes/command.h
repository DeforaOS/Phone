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



#ifndef PHONE_MODEM_HAYES_COMMAND_H
# define PHONE_MODEM_HAYES_COMMAND_H

# include "channel.h"


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
	HCS_UNKNOWN = 0,
	HCS_QUEUED,
	HCS_PENDING,
	HCS_ACTIVE,
	HCS_TIMEOUT,
	HCS_ERROR,
	HCS_SUCCESS
} HayesCommandStatus;
#define HCS_LAST HCS_SUCCESS
#define HCS_COUNT (HCS_LAST + 1)

typedef struct _HayesCommand HayesCommand;

typedef HayesCommandStatus (*HayesCommandCallback)(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);


/* prototypes */
HayesCommand * hayes_command_new(char const * attention);
HayesCommand * hayes_command_new_copy(HayesCommand const * command);
void hayes_command_delete(HayesCommand * command);

/* accessors */
char const * hayes_command_get_answer(HayesCommand * command);
char const * hayes_command_get_attention(HayesCommand * command);
void * hayes_command_get_data(HayesCommand * command);
HayesCommandPriority hayes_command_get_priority(HayesCommand * command);
HayesCommandStatus hayes_command_get_status(HayesCommand * command);
unsigned int hayes_command_get_timeout(HayesCommand * command);
int hayes_command_is_complete(HayesCommand * command);
void hayes_command_set_callback(HayesCommand * command,
		HayesCommandCallback callback, HayesChannel * channel);
void hayes_command_set_data(HayesCommand * command, void * data);
void hayes_command_set_priority(HayesCommand * command,
		HayesCommandPriority priority);
HayesCommandStatus hayes_command_set_status(HayesCommand * command,
		HayesCommandStatus status);
void hayes_command_set_timeout(HayesCommand * command,
		unsigned int timeout);

/* useful */
int hayes_command_answer_append(HayesCommand * command,
		char const * answer);
HayesCommandStatus hayes_command_callback(HayesCommand * command);

#endif /* PHONE_MODEM_HAYES_COMMAND_H */
