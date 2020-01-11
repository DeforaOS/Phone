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



#ifndef PHONE_MODEM_HAYES_QUIRKS_H
# define PHONE_MODEM_HAYES_QUIRKS_H


/* HayesQuirks */
/* public */
/* types */
typedef enum _HayesQuirk
{
	HAYES_QUIRK_BATTERY_70			= 0x01,
	HAYES_QUIRK_CPIN_NO_QUOTES		= 0x02,
	HAYES_QUIRK_CPIN_SLOW			= 0x04,
	HAYES_QUIRK_CONNECTED_LINE_DISABLED	= 0x08,
	HAYES_QUIRK_WANT_SMSC_IN_PDU		= 0x10,
	HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR	= 0x20
} HayesQuirk;

typedef const struct _HayesQuirks
{
	char const * vendor;
	char const * model;
	unsigned int quirks;
} HayesQuirks;


/* functions */
unsigned int hayes_quirks(char const * vendor, char const * model);

#endif /* PHONE_MODEM_HAYES_QUIRKS_H */
