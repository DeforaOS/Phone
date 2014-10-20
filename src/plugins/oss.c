/* $Id$ */
/* Copyright (c) 2011-2014 Pierre Pronchery <khorben@defora.org> */
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
#define min(a, b) ((a) < (b) ? (a) : (b))

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

#pragma pack(1)
typedef struct _RIFFChunk
{
	char ckID[4];
	uint32_t ckSize;
	uint8_t ckData[0];
} RIFFChunk;
#pragma pack()

#pragma pack(1)
typedef struct _WaveFormat
{
	uint16_t wFormatTag;
	uint16_t wChannels;
	uint32_t dwSamplesPerSec;
	uint32_t dwAvgBytesPerSec;
	uint16_t wBlockAlign;
} WaveFormat;
#pragma pack()
#define WAVE_FORMAT_PCM		0x0001
#define IBM_FORMAT_MULAW	0x0101
#define IBM_FORMAT_ALAW		0x0102
#define IBM_FORMAT_ADPCM	0x0103


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
static int _event_audio_play_file(OSS * oss, char const * filename);
static int _event_audio_play_open(OSS * oss, char const * device, FILE * fp,
		WaveFormat * wf, RIFFChunk * rc);
static int _event_audio_play_write(OSS * oss, RIFFChunk * rc, RIFFChunk * rc2,
		FILE * fp, int fd);
static int _event_volume_get(OSS * oss, gdouble * level);
static int _event_volume_set(OSS * oss, gdouble level);

static int _oss_event(OSS * oss, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_AUDIO_PLAY:
			/* XXX ignore errors */
			_event_audio_play(oss, event->audio_play.sample);
			return 0;
		case PHONE_EVENT_TYPE_VOLUME_GET:
			/* XXX ignore errors */
			_event_volume_get(oss, &event->volume_get.level);
			return 0;
		case PHONE_EVENT_TYPE_VOLUME_SET:
			/* XXX ignore errors */
			_event_volume_set(oss, event->volume_set.level);
			return 0;
		default: /* not relevant */
			break;
	}
	return 0;
}

static int _event_audio_play(OSS * oss, char const * sample)
{
	const char path[] = DATADIR "/sounds/" PACKAGE;
	const char ext[] = ".wav";
	String * s;
	char buf[128];

	if((s = string_new_append(path, "/", sample, ext, NULL)) == NULL)
		return -oss->helper->error(NULL, error_get(), 1);
	/* play the audio file */
	if(_event_audio_play_file(oss, s) != 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", s, strerror(errno));
		oss->helper->error(NULL, buf, 1);
		string_delete(s);
		return -1;
	}
	string_delete(s);
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
	if(fseek(fp, rc.ckSize, SEEK_CUR) != 0)
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
	if(rc->ckSize < sizeof(riffid))
		return -1;
	if(fread(&riffid, sizeof(riffid), 1, fp) != 1)
		return -oss->helper->error(NULL, strerror(errno), 1);
	rc->ckSize -= sizeof(riffid);
	if(strncmp(riffid, wave, sizeof(wave)) == 0)
		return _event_audio_play_chunk_wave(oss, fp, rc);
	/* skip the rest of the chunk */
	if(fseek(fp, rc->ckSize, SEEK_CUR) != 0)
		return -1;
	rc->ckSize = 0;
	return 0;
}

static int _event_audio_play_chunk_wave(OSS * oss, FILE * fp, RIFFChunk * rc)
{
	RIFFChunk rc2;
	char const * dev;
	const char data[4] = "data";
	const char fmt[4] = "fmt ";
	WaveFormat wf;
	int fd = -1;

	while(rc->ckSize > 0)
	{
		/* read the current WAVE chunk */
		if(rc->ckSize < sizeof(rc2))
			return -1;
		if(fread(&rc2, sizeof(rc2), 1, fp) != 1)
			return -oss->helper->error(NULL, strerror(errno), 1);
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
			fprintf(stderr, "DEBUG: format 0x%04x, %u channels\n",
					wf.wFormatTag, wf.wChannels);
			fprintf(stderr, "DEBUG: %u %u\n", rc->ckSize, rc2.ckSize);
#endif
			dev = oss->helper->config_get(oss->helper->phone, "oss",
					"device");
			if((fd = _event_audio_play_open(oss, dev, fp, &wf,
							&rc2)) < 0)
				return -1;
		}
		else if(strncmp(rc2.ckID, data, sizeof(data)) == 0)
		{
#if 0 /* FIXME for big endian */
			/* FIXME implement */
#endif
			if(fd < 0)
				return -1;
			if(_event_audio_play_write(oss, rc, &rc2, fp, fd) != 0)
				break;
		}
		/* skip the rest of the chunk */
		if(fseek(fp, rc2.ckSize, SEEK_CUR) != 0)
		{
			if(fd >= 0)
				close(fd);
			return -1;
		}
		rc->ckSize -= rc2.ckSize;
		rc2.ckSize = 0;
	}
	if(fd >= 0)
		close(fd);
	return 0;
}

static int _event_audio_play_file(OSS * oss, char const * filename)
{
	FILE * fp;

	/* open the audio file */
	if((fp = fopen(filename, "rb")) == NULL)
		return -1;
	/* go through every chunk */
	while(_event_audio_play_chunk(oss, fp) == 0);
	if(fclose(fp) != 0)
		return -1;
	return 0;
}

static int _event_audio_play_open(OSS * oss, char const * device, FILE * fp,
		WaveFormat * wf, RIFFChunk * rc)
{
#ifdef __NetBSD__
	const char devdsp[] = "/dev/sound";
#else
	const char devdsp[] = "/dev/dsp";
#endif
	int fd;
	int format;
	int channels;
	int samplerate;
	char buf[128];
	uint16_t bps;

	device = (device != NULL) ? device : devdsp;
	switch(wf->wFormatTag)
	{
		case WAVE_FORMAT_PCM:
			if(rc == NULL || rc->ckSize < sizeof(bps)
					|| fread(&bps, sizeof(bps), 1, fp) != 1)
				return -oss->helper->error(NULL,
						"Invalid WAVE file", 1);
			rc->ckSize -= sizeof(bps);
			/* FIXME for big endian */
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %u\n", __func__, bps);
#endif
			/* FIXME may be wrong */
			format = (bps == 8) ? AFMT_U8
				: ((bps == 16) ? AFMT_S16_LE : AFMT_U8);
			break;
		default:
			return -oss->helper->error(NULL,
					"Unsupported WAVE format", 1);
	}
	channels = wf->wChannels;
	samplerate = wf->dwSamplesPerSec;
	if((fd = open(device, O_WRONLY)) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", device, strerror(errno));
		return -oss->helper->error(NULL, buf, 1);
	}
	if(ioctl(fd, SNDCTL_DSP_SETFMT, &format) < 0
			|| ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) < 0
			|| ioctl(fd, SNDCTL_DSP_SPEED, &samplerate) < 0)
	{
		close(fd);
		snprintf(buf, sizeof(buf), "%s: %s", device, strerror(errno));
		return -oss->helper->error(NULL, buf, 1);
	}
	return fd;
}

static int _event_audio_play_write(OSS * oss, RIFFChunk * rc, RIFFChunk * rc2,
		FILE * fp, int fd)
{
	uint8_t u8[4096];
	size_t s;
	ssize_t ss;

	for(; (s = min(sizeof(u8), rc2->ckSize)) > 0;
			rc->ckSize -= s, rc2->ckSize -= s)
	{
		if((s = fread(&u8, sizeof(*u8), s, fp)) == 0)
			return -1;
		if((ss = write(fd, &u8, s)) < 0)
			return -oss->helper->error(NULL, strerror(errno), 1);
		else if((size_t)ss != s) /* XXX */
			return -1;
	}
	return 0;
}

static int _event_volume_get(OSS * oss, gdouble * level)
{
	int v;
	char buf[128];

	if(oss->fd < 0)
		return 1;
	if(ioctl(oss->fd, MIXER_READ(SOUND_MIXER_VOLUME), &v) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", "MIXER_READ", strerror(
					errno));
		return -oss->helper->error(NULL, buf, 1);
	}
	*level = (((v & 0xff00) >> 8) + (v & 0xff)) / 2;
	*level /= 100;
	return 0;
}

static int _event_volume_set(OSS * oss, gdouble level)
{
	int v = level * 100;
	char buf[128];

	if(oss->fd < 0)
		return 1;
	v |= v << 8;
	if(ioctl(oss->fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &v) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", "MIXER_WRITE", strerror(
					errno));
		return -oss->helper->error(NULL, buf, 1);
	}
	return 0;
}


/* oss_open */
static int _oss_open(OSS * oss)
{
	char const * p;
	char buf[128];

	if(oss->fd >= 0 && close(oss->fd) != 0)
		oss->helper->error(NULL, strerror(errno), 1);
	if((p = oss->helper->config_get(oss->helper->phone, "oss", "mixer"))
			== NULL)
		p = "/dev/mixer";
	if((oss->fd = open(p, O_RDWR)) < 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", p, strerror(errno));
		return -oss->helper->error(NULL, buf, 1);
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
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
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
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	bbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 4);
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
