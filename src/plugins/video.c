/* $Id$ */
/* Copyright (c) 2012-2024 Pierre Pronchery <khorben@defora.org> */
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
/* TODO:
 * - display a window even if it was impossible to open a capture device
 * - re-use code from the Camera project instead */



#include <sys/ioctl.h>
#include <sys/mman.h>
#ifdef __NetBSD__
# include <sys/videoio.h>
# include <paths.h>
#elif !defined(__APPLE__)
# include <linux/videodev2.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <errno.h>
#include <System.h>
#include <Desktop.h>
#include "Phone.h"


/* Video */
/* private */
/* types */
typedef struct _VideoBuffer
{
	void * start;
	size_t length;
} VideoBuffer;

typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	String * device;
	gboolean hflip;
	gboolean vflip;
	gboolean ratio;
	GdkInterpType interp;

	guint source;
	int fd;
#ifndef __APPLE__
	struct v4l2_capability cap;
	struct v4l2_format format;
#endif

	/* I/O channel */
	GIOChannel * channel;

	/* input data */
	/* XXX for mmap() */
	VideoBuffer * buffers;
	size_t buffers_cnt;
	char * raw_buffer;
	size_t raw_buffer_cnt;

	/* RGB data */
	unsigned char * rgb_buffer;
	size_t rgb_buffer_cnt;

	/* decoding */
	int yuv_amp;

	/* widgets */
	GtkWidget * window;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkGC * gc;
#endif
	GtkWidget * area;
	GtkAllocation area_allocation;
	GdkPixbuf * pixbuf;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkPixmap * pixmap;
#endif
} VideoPhonePlugin;


/* prototypes */
/* plug-in */
static VideoPhonePlugin * _video_init(PhonePluginHelper * helper);
static void _video_destroy(VideoPhonePlugin * video);
static void _video_settings(VideoPhonePlugin * video);

/* useful */
#ifndef __APPLE__
static int _video_ioctl(VideoPhonePlugin * video, unsigned long request,
		void * data);
#endif

static void _video_start(VideoPhonePlugin * video);
static void _video_stop(VideoPhonePlugin * video);

/* callbacks */
#ifndef __APPLE__
static gboolean _video_on_can_mmap(GIOChannel * channel, GIOCondition condition,
		gpointer data);
static gboolean _video_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data);
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean _video_on_drawing_area_draw(GtkWidget * widget, cairo_t * cr,
		gpointer data);
static void _video_on_drawing_area_size_allocate(GtkWidget * widget,
		GdkRectangle * allocation, gpointer data);
#else
static gboolean _video_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data);
static gboolean _video_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data);
#endif
static gboolean _video_on_open(gpointer data);
#ifndef __APPLE__
static gboolean _video_on_refresh(gpointer data);
#endif


/* constants */
#ifdef _PATH_VIDEO0
# define VIDEO_DEVICE	_PATH_VIDEO0
#else
# define VIDEO_DEVICE	"/dev/video0"
#endif


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Video",
	"camera-video",
	NULL,
	_video_init,
	_video_destroy,
	NULL,
	_video_settings
};


/* private */
/* functions */
/* video_init */
static gboolean _init_on_closex(gpointer data);

static VideoPhonePlugin * _video_init(PhonePluginHelper * helper)
{
	VideoPhonePlugin * video;
	char const * device;
	char const * p;

	if((video = object_new(sizeof(*video))) == NULL)
		return NULL;
	video->helper = helper;
	if((device = helper->config_get(helper->phone, "video", "device"))
			== NULL)
		device = VIDEO_DEVICE;
	p = helper->config_get(helper->phone, "video", "hflip");
	video->hflip = (p != NULL && p[0] != '\0' && strtol(p, NULL, 10) > 0)
		? TRUE : FALSE;
	p = helper->config_get(helper->phone, "video", "vflip");
	video->vflip = (p != NULL && p[0] != '\0' && strtol(p, NULL, 10) > 0)
		? TRUE : FALSE;
	p = helper->config_get(helper->phone, "video", "ratio");
	video->ratio = (p == NULL || p[0] == '\0' || strtol(p, NULL, 10) != 0)
		? TRUE : FALSE;
	video->interp = GDK_INTERP_BILINEAR;
	video->source = 0;
	video->fd = -1;
#ifndef __APPLE__
	memset(&video->cap, 0, sizeof(video->cap));
#endif
	video->channel = NULL;
	video->buffers = NULL;
	video->buffers_cnt = 0;
	video->raw_buffer = NULL;
	video->raw_buffer_cnt = 0;
	video->rgb_buffer = NULL;
	video->rgb_buffer_cnt = 0;
	video->yuv_amp = 255;
	video->window = NULL;
#if !GTK_CHECK_VERSION(3, 0, 0)
	video->gc = NULL;
#endif
	/* check for errors */
	if((video->device = string_new(device)) == NULL)
	{
		helper->error(helper->phone,
				"Could not initialize the video capture device",
				1);
		_video_destroy(video);
		return NULL;
	}
	/* create the window */
	video->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(video->window), "Phone - Video");
	gtk_widget_realize(video->window);
#if !GTK_CHECK_VERSION(3, 0, 0)
	video->gc = gdk_gc_new(video->window->window); /* XXX */
#endif
	g_signal_connect_swapped(video->window, "delete-event", G_CALLBACK(
				_init_on_closex), video);
	video->area = gtk_drawing_area_new();
	video->pixbuf = NULL;
#if !GTK_CHECK_VERSION(3, 0, 0)
	video->pixmap = NULL;
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect(video->area, "draw", G_CALLBACK(
				_video_on_drawing_area_draw), video);
	g_signal_connect(video->area, "size-allocate", G_CALLBACK(
				_video_on_drawing_area_size_allocate), video);
#else
	g_signal_connect(video->area, "configure-event", G_CALLBACK(
				_video_on_drawing_area_configure), video);
	g_signal_connect(video->area, "expose-event", G_CALLBACK(
				_video_on_drawing_area_expose), video);
#endif
	gtk_container_add(GTK_CONTAINER(video->window), video->area);
	gtk_widget_show_all(video->window);
	_video_start(video);
	return video;
}

static gboolean _init_on_closex(gpointer data)
{
	VideoPhonePlugin * video = data;

	gtk_widget_hide(video->window);
	_video_stop(video);
	return TRUE;
}


/* video_destroy */
static void _video_destroy(VideoPhonePlugin * video)
{
	size_t i;

	_video_stop(video);
	if(video->channel != NULL)
	{
		/* XXX we ignore errors at this point */
		g_io_channel_shutdown(video->channel, TRUE, NULL);
		g_io_channel_unref(video->channel);
	}
#if !GTK_CHECK_VERSION(3, 0, 0)
	if(video->pixmap != NULL)
		g_object_unref(video->pixmap);
	if(video->gc != NULL)
		g_object_unref(video->gc);
#endif
	if(video->window != NULL)
		gtk_widget_destroy(video->window);
	if(video->fd >= 0)
		close(video->fd);
	if((char *)video->rgb_buffer != video->raw_buffer)
		free(video->rgb_buffer);
	for(i = 0; i < video->buffers_cnt; i++)
		if(video->buffers[i].start != MAP_FAILED)
			munmap(video->buffers[i].start,
					video->buffers[i].length);
	free(video->buffers);
	free(video->raw_buffer);
	string_delete(video->device);
	object_delete(video);
}


/* video_settings */
static void _video_settings(VideoPhonePlugin * video)
{
	gtk_window_present(GTK_WINDOW(video->window));
	_video_start(video);
}


/* useful */
/* video_ioctl */
#ifndef __APPLE__
static int _video_ioctl(VideoPhonePlugin * video, unsigned long request,
		void * data)
{
	int ret;

	for(;;)
		if((ret = ioctl(video->fd, request, data)) != -1
				|| errno != EINTR)
			break;
	return ret;
}
#endif


/* video_start */
static void _video_start(VideoPhonePlugin * video)
{
	if(video->source != 0)
		return;
	video->source = g_idle_add(_video_on_open, video);
}


/* video_stop */
static void _video_stop(VideoPhonePlugin * video)
{
	if(video->source != 0)
		g_source_remove(video->source);
	video->source = 0;
}


/* callbacks */
#ifndef __APPLE__
/* video_on_can_mmap */
static gboolean _video_on_can_mmap(GIOChannel * channel, GIOCondition condition,
		gpointer data)
{
	VideoPhonePlugin * video = data;
	PhonePluginHelper * helper = video->helper;
	struct v4l2_buffer buf;

	if(channel != video->channel || condition != G_IO_IN)
		return FALSE;
	if(_video_ioctl(video, VIDIOC_DQBUF, &buf) == -1)
	{
		helper->error(helper->phone, "Could not save picture", 1);
		return FALSE;
	}
	video->raw_buffer = video->buffers[buf.index].start;
	video->raw_buffer_cnt = buf.bytesused;
# if 0 /* FIXME the raw buffer is not meant to be free()'d */
	video->source = g_idle_add(_video_on_refresh, video);
	return FALSE;
# else
	_video_on_refresh(video);
	video->raw_buffer = NULL;
	video->raw_buffer_cnt = 0;
	return TRUE;
# endif
}


/* video_on_can_read */
static gboolean _video_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data)
{
	VideoPhonePlugin * video = data;
	PhonePluginHelper * helper = video->helper;
	ssize_t s;

# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
# endif
	if(channel != video->channel || condition != G_IO_IN)
		return FALSE;
	if((s = read(video->fd, video->raw_buffer, video->raw_buffer_cnt))
			<= 0)
	{
		/* this error can be ignored */
		if(errno == EAGAIN)
			return TRUE;
		close(video->fd);
		video->fd = -1;
		/* FIXME also free video->buffers */
		helper->error(helper->phone, strerror(errno), 1);
		video->source = 0;
		return FALSE;
	}
# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %lu %ld\n", __func__,
			video->raw_buffer_cnt, s);
# endif
	video->source = g_idle_add(_video_on_refresh, video);
	return FALSE;
}
#endif


#if GTK_CHECK_VERSION(3, 0, 0)
/* video_on_drawing_area_draw */
static gboolean _video_on_drawing_area_draw(GtkWidget * widget, cairo_t * cr,
		gpointer data)
{
	VideoPhonePlugin * video = data;
	(void) widget;

	if(video->pixbuf != NULL)
	{
		gdk_cairo_set_source_pixbuf(cr, video->pixbuf, 0, 0);
		cairo_paint(cr);
	}
	return TRUE;
}


/* video_on_drawing_area_size_allocate */
static void _video_on_drawing_area_size_allocate(GtkWidget * widget,
		GdkRectangle * allocation, gpointer data)
{
	VideoPhonePlugin * video = data;
	(void) widget;

	video->area_allocation = *allocation;
}

#else


/* video_on_drawing_area_configure */
static gboolean _video_on_drawing_area_configure(GtkWidget * widget,
                GdkEventConfigure * event, gpointer data)
{
	/* XXX this code is inspired from GQcam */
	VideoPhonePlugin * video = data;
	GtkAllocation * allocation = &video->area_allocation;

	if(video->pixmap != NULL)
		g_object_unref(video->pixmap);
	/* FIXME requires Gtk+ 2.18 */
	gtk_widget_get_allocation(widget, allocation);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %dx%d\n", __func__, allocation->width,
			allocation->height);
#endif
	video->pixmap = gdk_pixmap_new(widget->window, allocation->width,
			allocation->height, -1);
	/* FIXME is it not better to scale the previous pixmap for now? */
	gdk_draw_rectangle(video->pixmap, video->gc, TRUE, 0, 0,
			allocation->width, allocation->height);
	return TRUE;
}


/* video_on_drawing_area_expose */
static gboolean _video_on_drawing_area_expose(GtkWidget * widget,
                GdkEventExpose * event, gpointer data)
{
        /* XXX this code is inspired from GQcam */
        VideoPhonePlugin * video = data;

        gdk_draw_pixmap(widget->window, video->gc, video->pixmap,
                        event->area.x, event->area.y,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);
        return FALSE;
}
#endif


/* video_on_open */
static int _open_setup(VideoPhonePlugin * video);
#ifdef NOTYET
static int _open_setup_mmap(VideoPhonePlugin * video);
#endif
#ifndef __APPLE__
static int _open_setup_read(VideoPhonePlugin * video);
#endif

static gboolean _video_on_open(gpointer data)
{
	const int timeout = 10000;
	VideoPhonePlugin * video = data;
	PhonePluginHelper * helper = video->helper;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, video->device);
#endif
	if((video->fd = open(video->device, O_RDWR)) < 0)
	{
		error_set_code(1, "%s: %s (%s)", video->device,
				"Could not open the video capture device",
				strerror(errno));
		helper->error(helper->phone, error_get(NULL), 1);
		video->source = g_timeout_add(timeout, _video_on_open, video);
		return FALSE;
	}
	if(_open_setup(video) != 0)
	{
		helper->error(helper->phone, error_get(NULL), 1);
		close(video->fd);
		video->fd = -1;
		video->source = g_timeout_add(timeout, _video_on_open, video);
		return FALSE;
	}
	video->source = 0;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %dx%d\n", __func__,
			video->format.fmt.pix.width,
			video->format.fmt.pix.height);
#endif
#ifndef __APPLE__
	/* FIXME allow the window to be smaller */
	gtk_widget_set_size_request(video->area, video->format.fmt.pix.width,
			video->format.fmt.pix.height);
#endif
	return FALSE;
}

static int _open_setup(VideoPhonePlugin * video)
{
#ifdef __APPLE__
	return -error_set_code(1, "%s: %s", video->device,
			"Video is not supported on this platform");
#else
	int ret;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	GError * error = NULL;

	/* check for capabilities */
	if(_video_ioctl(video, VIDIOC_QUERYCAP, &video->cap) == -1)
		return -error_set_code(1, "%s: %s (%s)", video->device,
				"Could not obtain the capabilities",
				strerror(errno));
	if((video->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
		return -error_set_code(1, "%s: %s", video->device,
				"Not a video capture device");
	/* reset cropping */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(_video_ioctl(video, VIDIOC_CROPCAP, &cropcap) == 0)
	{
		/* reset to default */
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;
		if(_video_ioctl(video, VIDIOC_S_CROP, &crop) == -1
				&& errno == EINVAL)
			/* XXX ignore this error for now */
			error_set_code(1, "%s: %s", video->device,
					"Cropping not supported");
	}
	/* obtain the current format */
	if(_video_ioctl(video, VIDIOC_G_FMT, &video->format) == -1)
		return -error_set_code(1, "%s: %s", video->device,
				"Could not obtain the video capture format");
	/* check the current format */
	if(video->format.type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -error_set_code(1, "%s: %s", video->device,
				"Unsupported video capture type");
	if((video->cap.capabilities & V4L2_CAP_STREAMING) != 0)
#ifdef NOTYET
		ret = _open_setup_mmap(video);
#else
		ret = _open_setup_read(video);
#endif
	else if((video->cap.capabilities & V4L2_CAP_READWRITE) != 0)
		ret = _open_setup_read(video);
	else
		ret = -error_set_code(1, "%s: %s", video->device,
				"Unsupported capabilities");
	if(ret != 0)
		return ret;
	/* setup a IO channel */
	video->channel = g_io_channel_unix_new(video->fd);
	if(g_io_channel_set_encoding(video->channel, NULL, &error)
			!= G_IO_STATUS_NORMAL)
	{
		error_set_code(1, "%s", error->message);
		g_error_free(error);
		return -1;
	}
	g_io_channel_set_buffered(video->channel, FALSE);
	video->source = g_io_add_watch(video->channel, G_IO_IN,
			_video_on_can_read, video);
	return 0;
#endif
}

#ifdef NOTYET
static int _open_setup_mmap(VideoPhonePlugin * video)
{
	struct v4l2_requestbuffers req;
	size_t i;
	struct v4l2_buffer buf;

	/* memory mapping support */
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(_video_ioctl(video, VIDIOC_REQBUFS, &req) == -1)
		return -error_set_code(1, "%s: %s", video->device,
				"Could not request buffers");
	if(req.count < 2)
		return -error_set_code(1, "%s: %s", video->device,
				"Could not obtain enough buffers");
	if((video->buffers = malloc(sizeof(*video->buffers) * req.count))
			== NULL)
		return -error_set_code(1, "%s: %s", video->device,
				"Could not allocate buffers");
	video->buffers_cnt = req.count;
	/* initialize the buffers */
	memset(video->buffers, 0, sizeof(*video->buffers) * video->buffers_cnt);
	for(i = 0; i < video->buffers_cnt; i++)
		video->buffers[i].start = MAP_FAILED;
	/* map the buffers */
	for(i = 0; i < video->buffers_cnt; i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if(_video_ioctl(video, VIDIOC_QUERYBUF, &buf) == -1)
			return -error_set_code(1, "%s: %s", video->device,
					"Could not setup buffers");
		video->buffers[i].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, video->fd,
				buf.m.offset);
		if(video->buffers[i].start == MAP_FAILED)
			return -error_set_code(1, "%s: %s", video->device,
					"Could not map buffers");
		video->buffers[i].length = buf.length;
	}
	return 0;
}
#endif

#ifndef __APPLE__
static int _open_setup_read(VideoPhonePlugin * video)
{
	size_t cnt;
	char * p;

	/* FIXME also try to obtain a RGB24 format if possible */
	/* allocate the raw buffer */
	cnt = video->format.fmt.pix.sizeimage;
	if((p = realloc(video->raw_buffer, cnt)) == NULL)
		return -error_set_code(1, "%s: %s", video->device,
				strerror(errno));
	video->raw_buffer = p;
	video->raw_buffer_cnt = cnt;
	/* allocate the rgb buffer */
	cnt = video->format.fmt.pix.width * video->format.fmt.pix.height * 3;
	if((p = realloc(video->rgb_buffer, cnt)) == NULL)
		return -error_set_code(1, "%s: %s", video->device,
				strerror(errno));
	video->rgb_buffer = (unsigned char *)p;
	video->rgb_buffer_cnt = cnt;
	return 0;
}
#endif


/* video_on_refresh */
#ifndef __APPLE__
static void _refresh_convert(VideoPhonePlugin * video);
static void _refresh_convert_yuv(int amp, uint8_t y, uint8_t u, uint8_t v,
                uint8_t * r, uint8_t * g, uint8_t * b);
static void _refresh_hflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);
static void _refresh_scale(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);
static void _refresh_vflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);

static gboolean _video_on_refresh(gpointer data)
{
	VideoPhonePlugin * video = data;
	video->source = 0;
# if !GTK_CHECK_VERSION(3, 0, 0)
	GtkAllocation * allocation = &video->area_allocation;
# endif
	int width = video->format.fmt.pix.width;
	int height = video->format.fmt.pix.height;

# ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() 0x%x\n", __func__,
			video->format.fmt.pix.pixelformat);
# endif
	_refresh_convert(video);
# if !GTK_CHECK_VERSION(3, 0, 0)
	if(video->hflip == FALSE
			&& video->vflip == FALSE
			&& width == allocation->width
			&& height == allocation->height)
		/* render directly */
		gdk_draw_rgb_image(video->pixmap, video->gc, 0, 0,
				width, height, GDK_RGB_DITHER_NORMAL,
				video->rgb_buffer, width * 3);
	else
# endif
	{
		if(video->pixbuf != NULL)
			g_object_unref(video->pixbuf);
		/* render after scaling */
		video->pixbuf = gdk_pixbuf_new_from_data(video->rgb_buffer,
				GDK_COLORSPACE_RGB, FALSE, 8, width, height,
				width * 3, NULL, NULL);
		_refresh_hflip(video, &video->pixbuf);
		_refresh_vflip(video, &video->pixbuf);
		_refresh_scale(video, &video->pixbuf);
# if !GTK_CHECK_VERSION(3, 0, 0)
		gdk_pixbuf_render_to_drawable(video->pixbuf, video->pixmap,
				video->gc, 0, 0, 0, 0, -1, -1,
				GDK_RGB_DITHER_NORMAL, 0, 0);
# endif
	}
	/* force a refresh */
	gtk_widget_queue_draw(video->area);
	video->source = g_io_add_watch(video->channel, G_IO_IN,
			(video->buffers != NULL) ? _video_on_can_mmap
			: _video_on_can_read, video);
	return FALSE;
}

static void _refresh_convert(VideoPhonePlugin * video)
{
	size_t i;
	size_t j;

	switch(video->format.fmt.pix.pixelformat)
	{
		case V4L2_PIX_FMT_YUYV:
			for(i = 0, j = 0; i + 3 < video->raw_buffer_cnt;
					i += 4, j += 6)
			{
				/* pixel 0 */
				_refresh_convert_yuv(video->yuv_amp,
						video->raw_buffer[i],
						video->raw_buffer[i + 1],
						video->raw_buffer[i + 3],
						&video->rgb_buffer[j + 2],
						&video->rgb_buffer[j + 1],
						&video->rgb_buffer[j]);
				/* pixel 1 */
				_refresh_convert_yuv(video->yuv_amp,
						video->raw_buffer[i + 2],
						video->raw_buffer[i + 1],
						video->raw_buffer[i + 3],
						&video->rgb_buffer[j + 5],
						&video->rgb_buffer[j + 4],
						&video->rgb_buffer[j + 3]);
			}
			break;
		default:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() Unsupported format\n",
					__func__);
#endif
			break;
	}
}

static void _refresh_convert_yuv(int amp, uint8_t y, uint8_t u, uint8_t v,
                uint8_t * r, uint8_t * g, uint8_t * b)
{
	double dr;
	double dg;
	double db;

	dr = amp * (0.004565 * y + 0.007935 * u - 1.088);
	dg = amp * (0.004565 * y - 0.001542 * u - 0.003183 * v + 0.531);
	db = amp * (0.004565 * y + 0.000001 * u + 0.006250 * v - 0.872);
	*r = (dr < 0) ? 0 : ((dr > 255) ? 255 : dr);
	*g = (dg < 0) ? 0 : ((dg > 255) ? 255 : dg);
	*b = (db < 0) ? 0 : ((db > 255) ? 255 : db);
}

static void _refresh_hflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf)
{
	GdkPixbuf * pixbuf2;

	if(video->hflip == FALSE)
		return;
	/* XXX could probably be more efficient */
	pixbuf2 = gdk_pixbuf_flip(*pixbuf, TRUE);
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}

static void _refresh_scale(VideoPhonePlugin * video, GdkPixbuf ** pixbuf)
{
	GtkAllocation * allocation = &video->area_allocation;
	GdkPixbuf * pixbuf2;
	gdouble scale;
	gint width;
	gint height;
	gint x;
	gint y;

	if(video->ratio == FALSE)
		pixbuf2 = gdk_pixbuf_scale_simple(*pixbuf, allocation->width,
				allocation->height, video->interp);
	else
	{
		if((pixbuf2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
						allocation->width,
						allocation->height)) == NULL)
			return;
		/* XXX could be more efficient */
		gdk_pixbuf_fill(pixbuf2, 0);
		scale = (gdouble)allocation->width
			/ video->format.fmt.pix.width;
		scale = MIN(scale, (gdouble)allocation->height
				/ video->format.fmt.pix.height);
		width = (gdouble)video->format.fmt.pix.width * scale;
		width = MIN(width, allocation->width);
		height = (gdouble)video->format.fmt.pix.height * scale;
		height = MIN(height, allocation->height);
		x = (allocation->width - width) / 2;
		y = (allocation->height - height) / 2;
		gdk_pixbuf_scale(*pixbuf, pixbuf2, x, y, width, height,
				0.0, 0.0, scale, scale, video->interp);
	}
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}

static void _refresh_vflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf)
{
	GdkPixbuf * pixbuf2;

	if(video->vflip == FALSE)
		return;
	/* XXX could probably be more efficient */
	pixbuf2 = gdk_pixbuf_flip(*pixbuf, FALSE);
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}
#endif
