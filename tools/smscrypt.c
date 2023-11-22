/* $Id$ */
/* Copyright (c) 2010-2023 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <System.h>
#include "../src/plugins/smscrypt.c"


/* helper_config_foreach */
static void _config_foreach_section(Config const * config,
		String const * section, String const * variable,
		String const * value, void * priv);

static void _helper_config_foreach(Phone * phone, char const * section,
		PhoneConfigForeachCallback callback, void * priv)
{
	Config * config = (Config *)phone;
	struct PhoneConfigForeachData
	{
		PhoneConfigForeachCallback * callback;
		void * priv;
	} pcfd = { callback, priv };

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, \"%s\", %p, %p)\n", __func__,
			(void *)phone, section, (void *)callback, priv);
#endif
	config_foreach_section(config, section, _config_foreach_section, &pcfd);
}

static void _config_foreach_section(Config const * config,
		String const * section, String const * variable,
		String const * value, void * priv)
{
	struct PhoneConfigForeachData
	{
		PhoneConfigForeachCallback * callback;
		void * priv;
	} * pcfd = priv;

	pcfd->callback(variable, value, pcfd->priv);
}


/* helper_config_get */
static char const * _helper_config_get(Phone * phone, char const * section,
		char const * variable)
{
	Config * config = (Config *)phone;
#ifdef DEBUG
	char const * ret;

	fprintf(stderr, "DEBUG: %s(%p, \"%s\", \"%s\")\n", __func__,
			(void *)phone, section, variable);
	ret = config_get(config, section, variable);
	fprintf(stderr, "DEBUG: %s() => \"%s\"\n", __func__, ret);
	return ret;
#else
	return config_get(config, section, variable);
#endif
}


/* hexdump */
static void _hexdump(char const * buf, size_t len)
{
	unsigned char const * b = (unsigned char *)buf;
	size_t i;

	for(i = 0; i < len; i++)
	{
		printf(" %02x", b[i]);
		if((i % 16) == 7)
			fputc(' ', stdout);
		else if((i % 16) == 15)
			fputc('\n', stdout);
	}
	fputc('\n', stdout);
}


/* usage */
static int _usage(void)
{
	fputs("Usage: smscrypt [-p number] message\n", stderr);
	return 1;
}


/* main */
static gboolean _main_idle(gpointer data);

int main(int argc, char * argv[])
{
	int o;
	struct { char const * message; char const * number; } mn;
	
	mn.number = NULL;
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "p:")) != -1)
		switch(o)
		{
			case 'p':
				mn.number = optarg;
				break;
			default:
				return _usage();
		}
	if(optind + 1 != argc)
		return _usage();
	mn.message = argv[optind];
	g_idle_add(_main_idle, &mn);
	gtk_main();
	return 0;
}

static gboolean _main_idle(gpointer data)
{
#if 0
	struct { char const * message; char const * number; } * mn = data;
	PhonePluginHelper helper;
	Config * config;
	PhoneEncoding encoding = PHONE_ENCODING_UTF8;
	PhoneEvent event;
	char * p;
	size_t len;

	config = config_new();
	config_load(config, "/home/khorben/.phone"); /* FIXME hardcoded */
	helper.phone = (Phone*)config;
	helper.config_foreach = _helper_config_foreach;
	helper.config_get = _helper_config_get;
	plugin.helper = &helper;
	if(_smscrypt_init(&plugin) != 0)
	{
		error_print("smscrypt");
		return FALSE;
	}
	if((p = strdup(mn->message)) == NULL)
		return FALSE;
	printf("Message: \"%s\"\n", p);
	len = strlen(p);
	event.type = PHONE_EVENT_TYPE_SMS_SENDING;
	if(_smscrypt_event(&plugin, &event, mn->number, &encoding, &p, &len)
			!= 0)
#endif
		puts("Could not encrypt");
#if 0
	else
	{
		printf("Encrypted:\n");
		_hexdump(p, len);
		event.type = PHONE_EVENT_TYPE_SMS_RECEIVING;
		if(_smscrypt_event(&plugin, &event, mn->number, &encoding, &p,
					&len) != 0)
			puts("Could not decrypt");
		else
			printf("Decrypted: \"%s\"\n", p);
	}
	free(p);
	_smscrypt_destroy(&plugin);
	config_delete(config);
#endif
	gtk_main_quit();
	return FALSE;
}
