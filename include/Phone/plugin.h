/* $Id$ */
/* Copyright (c) 2011-2020 Pierre Pronchery <khorben@defora.org> */
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



#ifndef DESKTOP_PHONE_PLUGIN_H
# define DESKTOP_PHONE_PLUGIN_H

# include "phone.h"


/* PhonePlugin */
/* types */
typedef struct _PhonePlugin PhonePlugin;

typedef void (PhoneConfigForeachCallback)(char const * variable,
		char const * value, void * priv);

typedef struct _PhonePluginHelper
{
	Phone * phone;
	void (*config_foreach)(Phone * phone, char const * section,
			PhoneConfigForeachCallback callback, void * priv);
	char const * (*config_get)(Phone * phone, char const * section,
			char const * variable);
	int (*config_set)(Phone * phone, char const * section,
			char const * variable, char const * value);
	int (*confirm)(Phone * phone, char const * message);
	int (*error)(Phone * phone, char const * message, int ret);
	void (*about_dialog)(Phone * phone);
	int (*event)(Phone * phone, PhoneEvent * event);
	void (*message)(Phone * phone, PhoneMessage message, ...);
	int (*request)(Phone * phone, ModemRequest * request);
	int (*trigger)(Phone * phone, ModemEventType event);
} PhonePluginHelper;

typedef const struct _PhonePluginDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	PhonePlugin * (*init)(PhonePluginHelper * helper);
	void (*destroy)(PhonePlugin * plugin);
	int (*event)(PhonePlugin * plugin, PhoneEvent * event);
	void (*settings)(PhonePlugin * plugin);
} PhonePluginDefinition;

#endif /* !DESKTOP_PHONE_PLUGIN_H */
