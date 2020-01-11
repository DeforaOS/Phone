/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <System.h>
#include <Desktop.h>
#include "Phone.h"


/* Template */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
} TemplatePhonePlugin;


/* prototypes */
/* plug-in */
static TemplatePhonePlugin * _template_init(PhonePluginHelper * helper);
static void _template_destroy(TemplatePhonePlugin * template);
static int _template_event(TemplatePhonePlugin * template, PhoneEvent * event);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Template",
	"applications-development",
	NULL,
	_template_init,
	_template_destroy,
	_template_event,
	NULL
};


/* private */
/* functions */
/* template_init */
static TemplatePhonePlugin * _template_init(PhonePluginHelper * helper)
{
	TemplatePhonePlugin * template;

	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	return template;
}


/* template_destroy */
static void _template_destroy(TemplatePhonePlugin * template)
{
	object_delete(template);
}


/* template_event */
static int _template_event(TemplatePhonePlugin * template, PhoneEvent * event)
{
	switch(event->type)
	{
		default:
			break;
	}
	return 0;
}
