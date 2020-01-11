/* $Id$ */
/* Copyright (c) 2015-2020 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "common.h"
#include "pdu.h"


/* HayesPDU */
/* private */
/* useful */
static char * _hayespdu_convert_number_to_address(char const * number);


/* public */
/* functions */
/* hayespdu_encode */
static char * _encode_text_to_data(char const * text, size_t length);
static char * _encode_text_to_sept(char const * text, size_t length);

char * hayespdu_encode(char const * number, ModemMessageEncoding encoding,
		size_t length, char const * content, unsigned int flags)
{
	char * ret = NULL;
	char * addr;
	char * data;
	char * p = NULL;
	size_t len;
	char const * smsc = "";
	char const prefix[] = "1100";
	char const pid[] = "00";
	char dcs[] = "0X";
	char const vp[] = "AA";

	if(!hayescommon_number_is_valid(number))
		return NULL;
	switch(encoding)
	{
		case MODEM_MESSAGE_ENCODING_UTF8:
			/* FIXME really support UTF-8 when necessary */
			p = g_convert(content, length, "ISO-8859-1", "UTF-8",
					NULL, NULL, NULL);
			if(p == NULL)
				return NULL;
			content = p;
			length = strlen(content);
			/* fallthrough */
		case MODEM_MESSAGE_ENCODING_ASCII:
			dcs[1] = '0';
			data = _encode_text_to_sept(content, length);
			break;
		case MODEM_MESSAGE_ENCODING_DATA:
			dcs[1] = '4';
			data = _encode_text_to_data(content, length);
			break;
		default:
			return NULL;
	}
	addr = _hayespdu_convert_number_to_address(number);
	len = 2 + sizeof(prefix) + 2 + strlen((addr != NULL) ? addr : "")
		+ sizeof(pid) + sizeof(dcs) + sizeof(vp) + 2
		+ strlen((data != NULL) ? data : "");
	if(addr != NULL && (ret = malloc(len)) != NULL)
	{
		if(flags & HAYESPDU_FLAG_WANT_SMSC)
			smsc = "00";
		if(snprintf(ret, len, "%s%s%02zX%s%s%s%s%02zX%s", smsc, prefix,
					strlen(number), addr, pid, dcs, vp,
					length, data) >= (int)len)
		{
			free(ret);
			ret = NULL;
		}
	}
	free(data);
	free(addr);
	free(p);
	return ret;
}

static char * _encode_text_to_data(char const * text, size_t length)
{
	char const tab[16] = "0123456789ABCDEF";
	char * buf;
	size_t i;

	if((buf = malloc((length * 2) + 1)) == NULL)
		return NULL;
	for(i = 0; i < length; i++)
	{
		buf[(i * 2) + 1] = tab[text[i] & 0x0f];
		buf[i * 2] = tab[((text[i] & 0xf0) >> 4) & 0x0f];
	}
	buf[i * 2] = '\0';
	return buf;
}

/* this function is heavily inspired from gsmd, (c) 2007 OpenMoko, Inc. */
static char * _encode_text_to_sept(char const * text, size_t length)
{
	char const tab[16] = "0123456789ABCDEF";
	unsigned char const * t = (unsigned char const *)text;
	char * buf;
	char * p;
	size_t i;
	unsigned char ch1;
	unsigned char ch2;
	int shift = 0;

	if((buf = malloc((length * 2) + 1)) == NULL)
		return NULL;
	p = buf;
	for(i = 0; i < length; i++)
	{
		ch1 = t[i] & 0x7f;
		ch1 = (ch1 >> shift);
		ch2 = t[i + 1] & 0x7f;
		ch2 = ch2 << (7 - shift);
		ch1 = ch1 | ch2;
		*(p++) = tab[(ch1 & 0xf0) >> 4];
		*(p++) = tab[ch1 & 0x0f];
		if(++shift == 7)
		{
			shift = 0;
			i++;
		}
	}
	*p = '\0';
	return buf;
}


/* private */
/* functions */
/* hayespdu_convert_number_to_address */
static char * _hayespdu_convert_number_to_address(char const * number)
{
	char * ret;
	size_t len;
	size_t i;

	len = 2 + strlen(number) + 2;
	if((ret = malloc(len)) == NULL)
		return NULL;
	snprintf(ret, len, "%02X", (number[0] == '+') ? 145 : 129);
	if(number[0] == '+')
		number++;
	for(i = 2; i < len; i+=2)
	{
		if(number[i - 2] == '\0')
			break;
		ret[i] = number[i - 1];
		ret[i + 1] = number[i - 2];
		if(number[i - 1] == '\0')
		{
			ret[i] = 'F';
			i+=2;
			break;
		}
	}
	ret[i] = '\0';
	return ret;
}
