/* $Id$ */
/* Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org> */
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
 * - display a window even if it was impossible to open a capture device
 * - re-use code from the Camera project instead */



#include <sys/ioctl.h>
#ifdef __NetBSD__
# include <sys/videoio.h>
#else
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
	struct v4l2_capability cap;
	struct v4l2_format format;

	/* IO channel */
	GIOChannel * channel;

	/* input data */
	char * raw_buffer;
	size_t raw_buffer_cnt;

	/* RGB data */
	unsigned char * rgb_buffer;
	size_t rgb_buffer_cnt;

	/* decoding */
	int yuv_amp;

	/* widgets */
	GtkWidget * window;
	GdkGC * gc;
	GtkWidget * area;
	GtkAllocation area_allocation;
	GdkPixmap * pixmap;
} VideoPhonePlugin;


/* prototypes */
/* plug-in */
static VideoPhonePlugin * _video_init(PhonePluginHelper * helper);
static void _video_destroy(VideoPhonePlugin * video);

/* useful */
static int _video_ioctl(VideoPhonePlugin * video, unsigned long request,
		void * data);

static void _video_start(VideoPhonePlugin * video);
static void _video_stop(VideoPhonePlugin * video);

/* callbacks */
static gboolean _video_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data);
static gboolean _video_on_closex(gpointer data);
static gboolean _video_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data);
static gboolean _video_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data);
static gboolean _video_on_open(gpointer data);
static gboolean _video_on_refresh(gpointer data);


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
	NULL
};


/* private */
/* functions */
/* video_init */
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
		device = "/dev/video0";
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
	memset(&video->cap, 0, sizeof(video->cap));
	video->channel = NULL;
	video->raw_buffer = NULL;
	video->raw_buffer_cnt = 0;
	video->rgb_buffer = NULL;
	video->rgb_buffer_cnt = 0;
	video->yuv_amp = 255;
	video->window = NULL;
	video->gc = NULL;
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
	video->gc = gdk_gc_new(video->window->window); /* XXX */
	g_signal_connect_swapped(video->window, "delete-event", G_CALLBACK(
				_video_on_closex), video);
	video->area = gtk_drawing_area_new();
	video->pixmap = NULL;
	g_signal_connect(video->area, "configure-event", G_CALLBACK(
				_video_on_drawing_area_configure), video);
	g_signal_connect(video->area, "expose-event", G_CALLBACK(
				_video_on_drawing_area_expose), video);
	gtk_widget_set_size_request(video->area, video->format.fmt.pix.width,
			video->format.fmt.pix.height);
	gtk_container_add(GTK_CONTAINER(video->window), video->area);
	gtk_widget_show_all(video->window);
	_video_start(video);
	return video;
}


/* video_destroy */
static void _video_destroy(VideoPhonePlugin * video)
{
	_video_stop(video);
	if(video->channel != NULL)
	{
		/* XXX we ignore errors at this point */
		g_io_channel_shutdown(video->channel, TRUE, NULL);
#if 0
		/* FIXME seems to cause a crash in the original code */
		g_object_unref(video->channel);
#endif
	}
	if(video->pixmap != NULL)
		g_object_unref(video->pixmap);
	if(video->gc != NULL)
		g_object_unref(video->gc);
	if(video->window != NULL)
		gtk_widget_destroy(video->window);
	if(video->fd >= 0)
		close(video->fd);
	if((char *)video->rgb_buffer != video->raw_buffer)
		free(video->rgb_buffer);
	free(video->raw_buffer);
	string_delete(video->device);
	object_delete(video);
}


/* useful */
/* video_ioctl */
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
/* video_on_can_read */
static gboolean _video_on_can_read(GIOChannel * channel, GIOCondition condition,
		gpointer data)
{
	VideoPhonePlugin * video = data;
	PhonePluginHelper * helper = video->helper;
	ssize_t s;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
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
		helper->error(helper->phone, strerror(errno), 1);
		video->source = 0;
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %lu %ld\n", __func__,
			video->raw_buffer_cnt, s);
#endif
	video->source = g_idle_add(_video_on_refresh, video);
	return FALSE;
}


/* video_on_closex */
static gboolean _video_on_closex(gpointer data)
{
	VideoPhonePlugin * video = data;

	gtk_widget_hide(video->window);
	_video_stop(video);
	return TRUE;
}


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


/* video_on_open */
static int _open_setup(VideoPhonePlugin * video);

static gboolean _video_on_open(gpointer data)
{
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
		helper->error(helper->phone, error_get(), 1);
		video->source = g_timeout_add(10000, _video_on_open, video);
		return FALSE;
	}
	if(_open_setup(video) != 0)
	{
		helper->error(helper->phone, error_get(), 1);
		close(video->fd);
		video->fd = -1;
		video->source = g_timeout_add(10000, _video_on_open, video);
		return FALSE;
	}
	video->source = 0;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %dx%d\n", __func__,
			video->format.fmt.pix.width,
			video->format.fmt.pix.height);
#endif
	/* FIXME allow the window to be smaller */
	gtk_widget_set_size_request(video->area, video->format.fmt.pix.width,
			video->format.fmt.pix.height);
	return FALSE;
}

static int _open_setup(VideoPhonePlugin * video)
{
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	size_t cnt;
	char * p;
	GError * error = NULL;

	/* check for errors */
	if(_video_ioctl(video, VIDIOC_QUERYCAP, &video->cap) == -1)
		return -error_set_code(1, "%s: %s (%s)", video->device,
				"Could not obtain the capabilities",
				strerror(errno));
	if((video->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0
			/* FIXME also implement mmap() and streaming */
			|| (video->cap.capabilities & V4L2_CAP_READWRITE) == 0)
		return -error_set_code(1, "%s: %s", video->device,
				"Unsupported capabilities");
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
}


/* video_on_refresh */
static void _refresh_convert(VideoPhonePlugin * video);
static void _refresh_convert_yuv(int amp, uint8_t y, uint8_t u, uint8_t v,
                uint8_t * r, uint8_t * g, uint8_t * b);
static void _refresh_hflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);
static void _refresh_scale(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);
static void _refresh_vflip(VideoPhonePlugin * video, GdkPixbuf ** pixbuf);

static gboolean _video_on_refresh(gpointer data)
{
	VideoPhonePlugin * video = data;
	GtkAllocation * allocation = &video->area_allocation;
	int width = video->format.fmt.pix.width;
	int height = video->format.fmt.pix.height;
	GdkPixbuf * pixbuf;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() 0x%x\n", __func__,
			video->format.fmt.pix.pixelformat);
#endif
	_refresh_convert(video);
	if(video->hflip == FALSE
			&& video->vflip == FALSE
			&& width == allocation->width
			&& height == allocation->height)
		/* render directly */
		gdk_draw_rgb_image(video->pixmap, video->gc, 0, 0,
				width, height, GDK_RGB_DITHER_NORMAL,
				video->rgb_buffer, width * 3);
	else
	{
		/* render after scaling */
		pixbuf = gdk_pixbuf_new_from_data(video->rgb_buffer,
				GDK_COLORSPACE_RGB, FALSE, 8, width, height,
				width * 3, NULL, NULL);
		_refresh_hflip(video, &pixbuf);
		_refresh_vflip(video, &pixbuf);
		_refresh_scale(video, &pixbuf);
		gdk_pixbuf_render_to_drawable(pixbuf, video->pixmap,
				video->gc, 0, 0, 0, 0, -1, -1,
				GDK_RGB_DITHER_NORMAL, 0, 0);
		g_object_unref(pixbuf);
	}
	/* force a refresh */
	gtk_widget_queue_draw_area(video->area, 0, 0,
			video->area_allocation.width,
			video->area_allocation.height);
	video->source = g_io_add_watch(video->channel, G_IO_IN,
			_video_on_can_read, video);
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
