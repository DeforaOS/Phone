/* $Id$ */
/* Copyright (c) 2011-2013 Pierre Pronchery <khorben@defora.org> */
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
/* FIXME:
 * - implement native NetBSD support */



#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"
#include "../../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef SBINDIR
# define SBINDIR	PREFIX "/sbin"
#endif


/* OSS */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	GtkWidget * window;
	GtkWidget * sound;
	GtkWidget * mixer;
	int fd;
} OSS;

typedef struct _RIFFChunk
{
	char ckID[4];
	uint32_t ckSize;
	uint8_t ckData[0];
} RIFFChunk;

typedef struct _WaveFormat
{
	uint16_t wFormatTag;
	uint16_t wChannels;
	uint32_t dwSamplesPerSec;
	uint32_t dwAvgBytesPerSec;
	uint16_t wBlockAlign;
} WaveFormat;


/* prototypes */
static OSS * _oss_init(PhonePluginHelper * helper);
static void _oss_destroy(OSS * oss);
static int _oss_event(OSS * oss, PhoneEvent * event);
static int _oss_open(OSS * oss);
static void _oss_settings(OSS * oss);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"OSS audio",
	"audio-x-generic",
	NULL,
	_oss_init,
	_oss_destroy,
	_oss_event,
	_oss_settings
};


/* private */
/* functions */
/* oss_init */
static OSS * _oss_init(PhonePluginHelper * helper)
{
	OSS * oss;

	if((oss = object_new(sizeof(*oss))) == NULL)
		return NULL;
	oss->helper = helper;
	oss->window = NULL;
	oss->fd = -1;
	_oss_open(oss);
	return oss;
}


/* oss_destroy */
static void _oss_destroy(OSS * oss)
{
	if(oss->fd >= 0)
		close(oss->fd);
	if(oss->window != NULL)
		gtk_widget_destroy(oss->window);
	object_delete(oss);
}


/* oss_event */
static int _event_audio_play(OSS * oss, char const * sample);
static int _event_audio_play_chunk(OSS * oss, FILE * fp);
static int _event_audio_play_chunk_riff(OSS * oss, FILE * fp, RIFFChunk * rc);
static int _event_audio_play_chunk_wave(OSS * oss, FILE * fp, RIFFChunk * rc);
static int _event_modem_event(OSS * oss, ModemEvent * event);
static int _event_volume_get(OSS * oss, gdouble * level);
static int _event_volume_set(OSS * oss, gdouble level);

static int _oss_event(OSS * oss, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_AUDIO_PLAY:
			return _event_audio_play(oss,
					event->audio_play.sample);
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _event_modem_event(oss,
					event->modem_event.event);
		case PHONE_EVENT_TYPE_VOLUME_GET:
			return _event_volume_get(oss, &event->volume_get.level);
		case PHONE_EVENT_TYPE_VOLUME_SET:
			return _event_volume_set(oss, event->volume_set.level);
		default: /* not relevant */
			break;
	}
	return 0;
}

static int _event_audio_play(OSS * oss, char const * sample)
{
	const char path[] = DATADIR "/sounds/" PACKAGE;
	String * s;
	FILE * fp;

	/* XXX ignore errors */
	if((s = string_new_append(path, "/", sample, ".wav", NULL)) == NULL)
		return oss->helper->error(NULL, error_get(), 0);
	/* open the audio file */
	if((fp = fopen(s, "rb")) == NULL)
	{
		oss->helper->error(NULL, strerror(errno), 0);
		string_delete(s);
		return 0;
	}
	string_delete(s);
	/* go through every chunk */
	while(_event_audio_play_chunk(oss, fp) == 0);
	fclose(fp);
	return 0;
}

static int _event_audio_play_chunk(OSS * oss, FILE * fp)
{
	RIFFChunk rc;

	if(fread(&rc, sizeof(rc.ckID) + sizeof(rc.ckSize), 1, fp) != 1)
		return -1;
#if 0 /* FIXME for big endian */
	rc.ckSize = (rc.ckSize & 0xff000000) >> 24
		| (rc.ckSize & 0xff0000) >> 8
		| (rc.ckSize & 0xff00) << 8;
	| (rc.ckSize & 0xff) << 24;
#endif
#ifdef DEBUG
	fprintf(stderr, "DEBUG: chunk \"%c%c%c%c\"\n", rc.ckID[0],
			rc.ckID[1], rc.ckID[2], rc.ckID[3]);
#endif
	if(strncmp(rc.ckID, "RIFF", 4) == 0)
	{
		if(_event_audio_play_chunk_riff(oss, fp, &rc) != 0)
			return -1;
	}
	for(; rc.ckSize > 0; rc.ckSize--)
		if(fgetc(fp) == EOF)
			return -1;
	/* FIXME implement the padding byte */
	return 0;
}

static int _event_audio_play_chunk_riff(OSS * oss, FILE * fp, RIFFChunk * rc)
{
	char riffid[4];
	const char wave[4] = "WAVE";

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(rc->ckSize < sizeof(riffid)
			|| fread(&riffid, sizeof(riffid), 1, fp) != 1)
		return -1;
	rc->ckSize -= sizeof(riffid);
	if(strncmp(riffid, wave, sizeof(wave)) == 0)
		return _event_audio_play_chunk_wave(oss, fp, rc);
	/* skip the rest of the chunk */
	for(; rc->ckSize > 0; rc->ckSize--)
		if(fgetc(fp) == EOF)
			return -1;
	return 0;
}

static int _event_audio_play_chunk_wave(OSS * oss, FILE * fp, RIFFChunk * rc)
{
	RIFFChunk rc2;
#ifdef __NetBSD__
	const char devdsp[] = "/dev/sound";
#else
	const char devdsp[] = "/dev/dsp";
#endif
	char const * dev;
	const char data[4] = "data";
	const char fmt[4] = "fmt ";
	WaveFormat wf;
	int fd = -1;
	int format;
	int channels;
	uint8_t u8;

	while(rc->ckSize > 0)
	{
		/* read the current WAVE chunk */
		if(rc->ckSize < sizeof(rc2)
				|| fread(&rc2, sizeof(rc2), 1, fp) != 1)
			return -1;
#if 0 /* FIXME for big endian */
		/* FIXME implement */
#endif
		rc->ckSize -= sizeof(rc2);
		/* interpret the WAVE chunk */
#ifdef DEBUG
		fprintf(stderr, "DEBUG: wave chunk \"%c%c%c%c\"\n", rc2.ckID[0],
				rc2.ckID[1], rc2.ckID[2], rc2.ckID[3]);
#endif
		if(strncmp(rc2.ckID, fmt, sizeof(fmt)) == 0)
		{
			if(fd >= 0)
			{
				close(fd);
				return -1;
			}
			if(rc->ckSize < sizeof(wf)
					|| rc2.ckSize < sizeof(wf)
					|| fread(&wf, sizeof(wf), 1, fp) != 1)
				return -1;
#if 0 /* FIXME for big endian */
			/* FIXME implement */
#endif
			rc->ckSize -= sizeof(wf);
			rc2.ckSize -= sizeof(wf);
#ifdef DEBUG
			fprintf(stderr, "DEBUG: 0x%04x format, %u channels\n",
					wf.wFormatTag, wf.wChannels);
#endif
			dev = oss->helper->config_get(oss->helper->phone, "oss",
					"device");
			switch(wf.wFormatTag)
			{
				case 0x01:
					dev = (dev != NULL) ? dev : devdsp;
					format = AFMT_U8;
					break;
				default:
					return -1;
			}
			channels = wf.wChannels;
			if((fd = open(dev, O_WRONLY)) < 0)
				return -oss->helper->error(NULL, dev, 1);
			if(ioctl(fd, SNDCTL_DSP_SETFMT, &format) < 0
					|| ioctl(fd, SNDCTL_DSP_CHANNELS,
						&channels) < 0)
			{
				close(fd);
				return -oss->helper->error(NULL, dev, 1);
			}
		}
		else if(strncmp(rc2.ckID, data, sizeof(data)) == 0)
		{
#if 0 /* FIXME for big endian */
			/* FIXME implement */
#endif
			if(fd < 0)
				return -1;
			/* FIXME use a larger buffer instead */
			for(; fread(&u8, sizeof(u8), 1, fp) == 1;
					rc->ckSize -= sizeof(u8),
					rc2.ckSize -= sizeof(u8))
				if(write(fd, &u8, sizeof(u8)) != sizeof(u8))
					break;
		}
		/* skip the rest of the chunk */
		for(; rc2.ckSize > 0; rc2.ckSize--, rc->ckSize--)
			if(fgetc(fp) == EOF)
			{
				if(fd >= 0)
					close(fd);
				return -1;
			}
	}
	if(fd >= 0)
		close(fd);
	return 0;
}

static int _event_modem_event(OSS * oss, ModemEvent * event)
{
	ModemCallDirection direction;

	switch(event->type)
	{
		case MODEM_EVENT_TYPE_CALL:
			if(event->call.status != MODEM_CALL_STATUS_RINGING)
				break;
			direction = event->call.direction;
			if(direction == MODEM_CALL_DIRECTION_INCOMING)
				/* FIXME ringtone */
				break;
			else if(direction == MODEM_CALL_DIRECTION_OUTGOING)
				/* FIXME tone */
				break;
			break;
		default:
			break;
	}
	return 0;
}

static int _event_volume_get(OSS * oss, gdouble * level)
{
	int ret = 0;
	int v;
	char buf[256];

	if(oss->fd < 0)
		return 1;
	if(ioctl(oss->fd, MIXER_READ(SOUND_MIXER_VOLUME), &v) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", "MIXER_READ", strerror(
					errno));
		ret |= oss->helper->error(NULL, buf, 0);
	}
	*level = (((v & 0xff00) >> 8) + (v & 0xff)) / 2;
	*level /= 100;
	return ret;
}

static int _event_volume_set(OSS * oss, gdouble level)
{
	int ret = 0;
	int v = level * 100;
	char buf[256];

	if(oss->fd < 0)
		return 1;
	v |= v << 8;
	if(ioctl(oss->fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &v) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", "MIXER_WRITE", strerror(
					errno));
		ret |= oss->helper->error(NULL, buf, 0);
	}
	return ret;
}


/* oss_open */
static int _oss_open(OSS * oss)
{
	char const * p;
	char buf[256];

	if(oss->fd >= 0)
		close(oss->fd);
	if((p = oss->helper->config_get(oss->helper->phone, "oss", "mixer"))
			== NULL)
		p = "/dev/mixer";
	if((oss->fd = open(p, O_RDWR)) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", p, strerror(errno));
		return oss->helper->error(NULL, buf, 1);
	}
	return 0;
}


/* oss_settings */
static void _on_settings_cancel(gpointer data);
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_ok(gpointer data);

static void _oss_settings(OSS * oss)
{
	GtkWidget * vbox;
	GtkWidget * bbox;
	GtkWidget * widget;

	if(oss->window != NULL)
	{
		gtk_window_present(GTK_WINDOW(oss->window));
		return;
	}
	oss->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(oss->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(oss->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(oss->window), "audio-x-generic");
#endif
	gtk_window_set_title(GTK_WINDOW(oss->window), "Sound preferences");
	g_signal_connect_swapped(oss->window, "delete-event", G_CALLBACK(
				_on_settings_closex), oss);
	vbox = gtk_vbox_new(FALSE, 4);
	/* devices */
	widget = gtk_label_new("Sound device:");
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	widget = gtk_file_chooser_button_new("Set the sound device",
			GTK_FILE_CHOOSER_ACTION_OPEN);
	oss->sound = widget;
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	widget = gtk_label_new("Mixer device:");
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	widget = gtk_file_chooser_button_new("Set the mixer device",
			GTK_FILE_CHOOSER_ACTION_OPEN);
	oss->mixer = widget;
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* button box */
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_settings_cancel), oss);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_on_settings_ok),
			oss);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(oss->window), vbox);
	_on_settings_cancel(oss);
	gtk_widget_show_all(oss->window);
}

static void _on_settings_cancel(gpointer data)
{
	OSS * oss = data;
#ifdef __NetBSD__
	const char devdsp[] = "/dev/sound";
#else
	const char devdsp = "/dev/dsp";
#endif
	char const * p;

	gtk_widget_hide(oss->window);
	if((p = oss->helper->config_get(oss->helper->phone, "oss", "device"))
			== NULL)
		p = devdsp;
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(oss->sound), p);
	if((p = oss->helper->config_get(oss->helper->phone, "oss", "mixer"))
			== NULL)
		p = "/dev/mixer";
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(oss->mixer), p);
}

static gboolean _on_settings_closex(gpointer data)
{
	OSS * oss = data;

	_on_settings_cancel(oss);
	return TRUE;
}

static void _on_settings_ok(gpointer data)
{
	OSS * oss = data;
	char const * p;

	gtk_widget_hide(oss->window);
	if((p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(oss->sound)))
			!= NULL)
		oss->helper->config_set(oss->helper->phone, "oss", "device", p);
	if((p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(oss->mixer)))
			!= NULL)
		oss->helper->config_set(oss->helper->phone, "oss", "mixer", p);
	_oss_open(oss);
}
