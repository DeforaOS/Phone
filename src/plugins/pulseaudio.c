/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
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
/* TODO:
 * - optionally loop the audio sample until told to stop
 * - implement setting the volume (for headset/loudspeaker/audio...) */



#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>
#include <System.h>
#include "Phone.h"
#include "../../config.h"
#define max(a, b) ((a) > (b) ? (a) : (b))


/* Pulseaudio */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	guint source;

	pa_threaded_mainloop * pam;
	pa_context * pac;
	pa_operation * pao;
} Pulseaudio;

/* prototypes */
/* plug-in */
static Pulseaudio * _pa_init(PhonePluginHelper * helper);
static void _pa_destroy(Pulseaudio * pa);
static int _pa_event(Pulseaudio * pa, PhoneEvent * event);

/* useful */
static void _pa_play(Pulseaudio * pa, char const * sound);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Pulseaudio",
	"system-config-users",
	NULL,
	_pa_init,
	_pa_destroy,
	_pa_event,
	NULL
};


/* private */
/* functions */
/* pa_init */
static Pulseaudio * _pa_init(PhonePluginHelper * helper)
{
	Pulseaudio * pa;
	pa_mainloop_api * mapi = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((pa = object_new(sizeof(*pa))) == NULL)
		return NULL;
	pa->helper = helper;
	pa->source = 0;
	pa->pam = pa_threaded_mainloop_new();
	pa->pac = NULL;
	pa->pao = NULL;
	if(pa->pam == NULL)
	{
		_pa_destroy(pa);
		error_set_code(1, "%s", "Could not initialize PulseAudio");
		return NULL;
	}
	mapi = pa_threaded_mainloop_get_api(pa->pam);
	/* XXX update the context name */
	if((pa->pac = pa_context_new(mapi, PACKAGE)) == NULL)
	{
		_pa_destroy(pa);
		error_set_code(1, "%s", "Could not initialize PulseAudio");
		return NULL;
	}
	pa_context_connect(pa->pac, NULL, 0, NULL);
	pa_threaded_mainloop_start(pa->pam);
	return pa;
}


/* pa_destroy */
static void _pa_destroy(Pulseaudio * pa)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(pa->source != 0)
		g_source_remove(pa->source);
	if(pa->pao != NULL)
		pa_operation_cancel(pa->pao);
	if(pa->pac != NULL)
		pa_context_unref(pa->pac);
	pa_threaded_mainloop_free(pa->pam);
	object_delete(pa);
}


/* pa_event */
static int _event_modem_event(Pulseaudio * pa, ModemEvent * event);

static int _pa_event(Pulseaudio * pa, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_AUDIO_PLAY:
			_pa_play(pa, event->audio_play.sample);
			break;
		case PHONE_EVENT_TYPE_AUDIO_STOP:
			_pa_play(pa, NULL);
			break;
		default: /* not relevant */
			break;
	}
	return 0;
}


/* pa_play */
static void _pa_play(Pulseaudio * pa, char const * sample)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, sample);
#endif
	if(sample == NULL)
	{
		/* cancel the current sample */
		if(pa->pao != NULL)
			pa_operation_cancel(pa->pao);
		pa->pao = NULL;
	}
	else if(pa->pao == NULL)
		/* FIXME apply the proper volume */
		pa->pao = pa_context_play_sample(pa->pac, sample, NULL,
				PA_VOLUME_NORM, NULL, NULL);
}
