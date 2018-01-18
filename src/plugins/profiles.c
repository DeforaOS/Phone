/* $Id$ */
/* Copyright (c) 2011-2018 Pierre Pronchery <khorben@defora.org> */
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



#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"
#define _(string) gettext(string)
#define N_(string) string

/* macros */
#define max(a, b) ((a) > (b) ? (a) : (b))


/* Profiles */
/* private */
/* types */
typedef enum _ProfileColumn
{
	PROFILE_COLUMN_RADIO = 0,
	PROFILE_COLUMN_TYPE,
	PROFILE_COLUMN_DEFAULT,
	PROFILE_COLUMN_ONLINE,
	PROFILE_COLUMN_VOLUME,
	PROFILE_COLUMN_VIBRATE,
	PROFILE_COLUMN_SAMPLE,
	PROFILE_COLUMN_ICON,
	PROFILE_COLUMN_NAME,
	PROFILE_COLUMN_NAME_DISPLAY
} ProfileColumn;
#define PROFILE_COLUMN_LAST PROFILE_COLUMN_NAME_DISPLAY
#define PROFILE_COLUMN_COUNT (PROFILE_COLUMN_LAST + 1)

typedef enum _ProfileType
{
	PROFILE_TYPE_GENERAL = 0,
	PROFILE_TYPE_BEEP,
	PROFILE_TYPE_SILENT,
	PROFILE_TYPE_OFFLINE
} ProfileType;
#define PROFILE_TYPE_LAST PROFILE_TYPE_OFFLINE
#define PROFILE_TYPE_COUNT (PROFILE_TYPE_LAST + 1)

typedef enum _ProfileVolume
{
	PROFILE_VOLUME_SILENT	= 0,
	PROFILE_VOLUME_25	= 25,
	PROFILE_VOLUME_50	= 50,
	PROFILE_VOLUME_75	= 75,
	PROFILE_VOLUME_100	= 100,
	PROFILE_VOLUME_ASC	= -1
} ProfileVolume;

typedef struct _ProfileDefinition
{
	char const * icon;
	char const * name;
	gboolean online;
	ProfileVolume volume;
	gboolean vibrate;
	char const * sample;
} ProfileDefinition;

typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	guint source;

	/* profiles */
	ProfileDefinition * profiles;
	size_t profiles_cnt;
	size_t profiles_cur;

	/* vibrator */
	int vibrator;

	/* settings */
	GtkWidget * pr_window;
	GtkListStore * pr_store;
	GtkWidget * pr_view;
	GtkWidget * pr_online;
	GtkWidget * pr_volume;
	GtkWidget * pr_ring;
	GtkWidget * pr_vibrator;
} Profiles;

/* constants */
#define PROFILE_VIBRATOR_LOOP	500

/* variables */
static ProfileDefinition _profiles_definitions[PROFILE_TYPE_COUNT] =
{
	{ NULL, N_("General"),	TRUE,	PROFILE_VOLUME_ASC,	TRUE,	NULL	},
	{ NULL, N_("Beep"),	TRUE,	PROFILE_VOLUME_25,	TRUE,	"beep"	},
	{ "audio-volume-muted",
		N_("Silent"),	TRUE,	PROFILE_VOLUME_SILENT,	TRUE,	NULL	},
	{ NULL, N_("Offline"),	FALSE,	PROFILE_VOLUME_SILENT,	FALSE,	NULL	}
};

/* prototypes */
/* plug-in */
static Profiles * _profiles_init(PhonePluginHelper * helper);
static void _profiles_destroy(Profiles * profiles);
static int _profiles_event(Profiles * profiles, PhoneEvent * event);
static void _profiles_settings(Profiles * profiles);

/* accessors */
static int _profiles_set(Profiles * profiles, ProfileType type);

/* useful */
static int _profiles_load(Profiles * profiles);
static int _profiles_save(Profiles * profiles);

static void _profiles_play(Profiles * profiles, char const * sound,
		int vibrator);
static void _profiles_switch(Profiles * profiles, ProfileType type);

/* callbacks */
static gboolean _profiles_on_vibrate(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"Profiles",
	"system-config-users",
	NULL,
	_profiles_init,
	_profiles_destroy,
	_profiles_event,
	_profiles_settings
};


/* private */
/* functions */
/* profiles_init */
static Profiles * _profiles_init(PhonePluginHelper * helper)
{
	Profiles * profiles;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((profiles = object_new(sizeof(*profiles))) == NULL)
		return NULL;
	profiles->helper = helper;
	profiles->source = 0;
	profiles->profiles = _profiles_definitions;
	profiles->profiles_cnt = sizeof(_profiles_definitions)
		/ sizeof(*_profiles_definitions);
	profiles->profiles_cur = PROFILE_TYPE_GENERAL;
	profiles->vibrator = 0;
	profiles->pr_window = NULL;
	profiles->pr_store = gtk_list_store_new(PROFILE_COLUMN_COUNT,
			G_TYPE_BOOLEAN,		/* radio */
			G_TYPE_UINT,		/* type */
			G_TYPE_BOOLEAN,		/* default */
			G_TYPE_BOOLEAN,		/* online */
			G_TYPE_INT,		/* volume */
			G_TYPE_BOOLEAN,		/* vibrate */
			G_TYPE_STRING,		/* sample */
			GDK_TYPE_PIXBUF,	/* icon */
			G_TYPE_STRING,		/* name */
			G_TYPE_STRING);		/* name display */
	_profiles_load(profiles);
	return profiles;
}


/* profiles_destroy */
static void _profiles_destroy(Profiles * profiles)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(profiles->source != 0)
		g_source_remove(profiles->source);
	if(profiles->pr_window != NULL)
		gtk_widget_destroy(profiles->pr_window);
	object_delete(profiles);
}


/* profiles_event */
static int _event_key_tone(Profiles * profiles);
static int _event_modem_event(Profiles * profiles, ModemEvent * event);
static int _event_offline(Profiles * profiles);
static int _event_starting(Profiles * profiles);
static int _event_stopping(Profiles * profiles);

static int _profiles_event(Profiles * profiles, PhoneEvent * event)
{
	ProfileDefinition * profile = &profiles->profiles[
		profiles->profiles_cur];

	switch(event->type)
	{
		case PHONE_EVENT_TYPE_KEY_TONE:
			return _event_key_tone(profiles);
		case PHONE_EVENT_TYPE_OFFLINE:
			return _event_offline(profiles);
		case PHONE_EVENT_TYPE_STARTING:
			return _event_starting(profiles);
		case PHONE_EVENT_TYPE_STOPPING:
			return _event_stopping(profiles);
		case PHONE_EVENT_TYPE_MESSAGE_RECEIVED:
			_profiles_play(profiles, (profile->sample != NULL)
					? profile->sample : "message", 2);
			break;
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _event_modem_event(profiles,
					event->modem_event.event);
		default: /* not relevant */
			break;
	}
	return 0;
}

static int _event_key_tone(Profiles * profiles)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_profiles_play(profiles, "keytone", 1);
	return 0;
}

static int _event_modem_event(Profiles * profiles, ModemEvent * event)
{
	ProfileDefinition * profile = &profiles->profiles[
		profiles->profiles_cur];
	ModemCallDirection direction;
	ModemCallStatus status;

	switch(event->type)
	{
		case MODEM_EVENT_TYPE_CALL:
			if(event->call.call_type != MODEM_CALL_TYPE_VOICE)
				break;
			direction = event->call.direction;
			status = event->call.status;
			if(direction == MODEM_CALL_DIRECTION_INCOMING
					&& status == MODEM_CALL_STATUS_RINGING)
				_profiles_play(profiles,
						(profile->sample != NULL)
						? profile->sample : "ringtone",
						10);
			else if(direction == MODEM_CALL_DIRECTION_OUTGOING
					&& status == MODEM_CALL_STATUS_RINGING)
				_profiles_play(profiles, "ringback", 0);
			else if(status == MODEM_CALL_STATUS_BUSY)
				_profiles_play(profiles, "busy", 0);
			else if(status == MODEM_CALL_STATUS_NONE
					|| status == MODEM_CALL_STATUS_ACTIVE)
				_profiles_play(profiles, NULL, 0);
			break;
		default:
			break;
	}
	return 0;
}

static int _event_offline(Profiles * profiles)
{
	_profiles_set(profiles, PROFILE_TYPE_OFFLINE);
	return 0;
}

static int _event_starting(Profiles * profiles)
{
	PhonePluginHelper * helper = profiles->helper;
	ProfileDefinition * profile = &profiles->profiles[
		profiles->profiles_cur];

	if(profile->online)
		return 0;
	if(helper->confirm(helper->phone, "You are currently offline.\n"
				"Do you want to go online?") != 0)
		return 1;
	_profiles_switch(profiles, PROFILE_TYPE_GENERAL);
	return 0;
}

static int _event_stopping(Profiles * profiles)
{
	ProfileDefinition * profile = &profiles->profiles[
		profiles->profiles_cur];

	/* prevent stopping the modem except if we're going offline */
	return profile->online ? 1 : 0;
}


/* profiles_settings */
static void _on_settings_activated(GtkTreeView * view, GtkTreePath * path,
		GtkTreeViewColumn * column, gpointer data);
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_cancel(gpointer data);
static void _on_settings_changed(GtkTreeSelection * treesel, gpointer data);
static void _on_settings_ok(gpointer data);
static void _on_settings_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data);

static void _profiles_settings(Profiles * profiles)
{
	GtkWidget * vbox;
	GtkWidget * frame;
	GtkWidget * bbox;
	GtkWidget * widget;
	GtkTreeSelection * treesel;
	GtkTreeViewColumn * column;
	GtkCellRenderer * renderer;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__,
			profiles->profiles[profiles->profiles_cur].name);
#endif
	if(profiles->pr_window != NULL)
	{
		gtk_window_present(GTK_WINDOW(profiles->pr_window));
		return;
	}
	profiles->pr_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(profiles->pr_window), 4);
	gtk_window_set_default_size(GTK_WINDOW(profiles->pr_window), 200, 300);
	gtk_window_set_title(GTK_WINDOW(profiles->pr_window), "Profiles");
	g_signal_connect_swapped(profiles->pr_window, "delete-event",
			G_CALLBACK(_on_settings_closex), profiles);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	profiles->pr_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				profiles->pr_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(profiles->pr_view),
			FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(profiles->pr_view), TRUE);
	g_signal_connect(profiles->pr_view, "row-activated",
			G_CALLBACK(_on_settings_activated), profiles);
	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(profiles->pr_view));
	g_signal_connect(treesel, "changed", G_CALLBACK(_on_settings_changed),
			profiles);
	/* view: radio */
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(
				_on_settings_toggled), profiles);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"active", PROFILE_COLUMN_DEFAULT,
			"radio", PROFILE_COLUMN_RADIO, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(profiles->pr_view), column);
	/* view: icon */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", PROFILE_COLUMN_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(profiles->pr_view), column);
	/* view: name */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
			"text", PROFILE_COLUMN_NAME_DISPLAY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(profiles->pr_view), column);
	gtk_container_add(GTK_CONTAINER(widget), profiles->pr_view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* frame */
	frame = gtk_frame_new("Overview");
#if GTK_CHECK_VERSION(3, 0, 0)
	widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	widget = gtk_vbox_new(FALSE, 4);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(widget), 4);
	profiles->pr_online = gtk_check_button_new_with_label(_("Online"));
	gtk_widget_set_sensitive(profiles->pr_online, FALSE);
	gtk_box_pack_start(GTK_BOX(widget), profiles->pr_online, FALSE, TRUE,
			0);
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	bbox = gtk_hbox_new(FALSE, 4);
#endif
	profiles->pr_volume = gtk_label_new(_("Volume: "));
	gtk_widget_set_sensitive(profiles->pr_volume, FALSE);
	gtk_box_pack_start(GTK_BOX(bbox), profiles->pr_volume, FALSE, TRUE, 0);
	profiles->pr_volume = gtk_progress_bar_new();
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(profiles->pr_volume, "show-text", TRUE, NULL);
#endif
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(profiles->pr_volume), "");
	gtk_widget_set_sensitive(profiles->pr_volume, FALSE);
	gtk_box_pack_start(GTK_BOX(bbox), profiles->pr_volume, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(widget), bbox, FALSE, TRUE, 0);
	profiles->pr_ring = gtk_check_button_new_with_label(_("Ring"));
	gtk_widget_set_sensitive(profiles->pr_ring, FALSE);
	gtk_box_pack_start(GTK_BOX(widget), profiles->pr_ring, FALSE, TRUE,
			0);
	profiles->pr_vibrator = gtk_check_button_new_with_label(_("Vibrate"));
	gtk_widget_set_sensitive(profiles->pr_vibrator, FALSE);
	gtk_box_pack_start(GTK_BOX(widget), profiles->pr_vibrator, FALSE, TRUE,
			0);
	gtk_container_add(GTK_CONTAINER(frame), widget);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	/* dialog */
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	bbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_settings_cancel), profiles);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_settings_ok), profiles);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(profiles->pr_window), vbox);
	gtk_widget_show_all(vbox);
	_on_settings_cancel(profiles);
	gtk_window_present(GTK_WINDOW(profiles->pr_window));
}

static void _on_settings_activated(GtkTreeView * view, GtkTreePath * path,
		GtkTreeViewColumn * column, gpointer data)
{
	Profiles * profiles = data;
	GtkTreeModel * model = GTK_TREE_MODEL(profiles->pr_store);
	GtkTreeIter iter;
	ProfileType type;
	(void) view;
	(void) column;

	if(gtk_tree_model_get_iter(model, &iter, path) != TRUE)
		return;
	gtk_tree_model_get(model, &iter, PROFILE_COLUMN_TYPE, &type, -1);
	_profiles_set(profiles, type);
}

static gboolean _on_settings_closex(gpointer data)
{
	Profiles * profiles = data;

	_on_settings_cancel(profiles);
	return TRUE;
}

static void _on_settings_cancel(gpointer data)
{
	Profiles * profiles = data;
	GtkTreeModel * model = GTK_TREE_MODEL(profiles->pr_store);
	GtkTreeIter iter;
	gboolean valid;
	size_t i;

	gtk_widget_hide(profiles->pr_window);
	valid = gtk_tree_model_get_iter_first(model, &iter);
	for(i = 0; valid; valid = gtk_tree_model_iter_next(model, &iter), i++)
		gtk_list_store_set(profiles->pr_store, &iter,
				PROFILE_COLUMN_DEFAULT,
				(i == profiles->profiles_cur) ? TRUE : FALSE,
				-1);
}

static void _on_settings_changed(GtkTreeSelection * treesel, gpointer data)
{
	Profiles * profiles = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	gboolean online;
	ProfileVolume volume;
	gboolean ring;
	gboolean vibrate;
	char buf[16];
	double fraction;

	if(gtk_tree_selection_get_selected(treesel, &model, &iter) != TRUE)
		return;
	gtk_tree_model_get(model, &iter, PROFILE_COLUMN_ONLINE, &online,
			PROFILE_COLUMN_VOLUME, &volume,
			PROFILE_COLUMN_VIBRATE, &vibrate, -1);
	fraction = volume;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(profiles->pr_online),
			online);
	if(volume > 0)
	{
		snprintf(buf, sizeof(buf), "%u %%", volume);
		ring = TRUE;
	}
	else if(volume == 0)
	{
		snprintf(buf, sizeof(buf), "%s", _("Silent"));
		ring = FALSE;
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s", _("Ascending"));
		fraction = 0.0;
		ring = TRUE;
	}
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(profiles->pr_volume),
			fraction / 100.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(profiles->pr_volume), buf);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(profiles->pr_ring),
			ring);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(profiles->pr_vibrator),
			vibrate);
}

static void _on_settings_ok(gpointer data)
{
	Profiles * profiles = data;
	GtkTreeModel * model = GTK_TREE_MODEL(profiles->pr_store);
	GtkTreeIter iter;
	ProfileType type;
	gboolean valid;

	gtk_widget_hide(profiles->pr_window);
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter,
				PROFILE_COLUMN_TYPE, &type,
				PROFILE_COLUMN_DEFAULT, &valid, -1);
		if(valid)
		{
			_profiles_switch(profiles, type);
			break;
		}
	}
}

static void _on_settings_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Profiles * profiles = data;
	GtkTreeModel * model = GTK_TREE_MODEL(profiles->pr_store);
	GtkTreeIter iter;
	ProfileType type;
	(void) renderer;

	gtk_tree_model_get_iter_from_string(model, &iter, path);
	gtk_tree_model_get(model, &iter, PROFILE_COLUMN_TYPE, &type, -1);
	_profiles_set(profiles, type);
}


/* accessors */
/* profiles_set */
static int _profiles_set(Profiles * profiles, ProfileType type)
{
	PhonePluginHelper * helper = profiles->helper;
	GtkTreeModel * model = GTK_TREE_MODEL(profiles->pr_store);
	GtkTreeIter iter;
	gboolean valid;
	ProfileType pt;

	if(type >= profiles->profiles_cnt)
		return -helper->error(NULL, _("Invalid profile"), 1);
	profiles->profiles_cur = type;
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, PROFILE_COLUMN_TYPE, &pt, -1);
		gtk_list_store_set(profiles->pr_store, &iter,
				PROFILE_COLUMN_DEFAULT,
				(pt == type) ? TRUE : FALSE, -1);
	}
	return 0;
}


/* useful */
/* profiles_load */
static int _profiles_load(Profiles * profiles)
{
	PhonePluginHelper * helper = profiles->helper;
	GtkIconTheme * theme;
	ProfileDefinition * profile;
	GtkTreeIter iter;
	char const * p;
	size_t i = 0;

	theme = gtk_icon_theme_get_default();
	/* profiles */
	if((p = helper->config_get(helper->phone, "profiles", "default"))
			== NULL)
		p = profiles->profiles[0].name;
	gtk_list_store_clear(profiles->pr_store);
	for(i = 0; i < profiles->profiles_cnt; i++)
	{
		profile = &profiles->profiles[i];
		gtk_list_store_append(profiles->pr_store, &iter);
		gtk_list_store_set(profiles->pr_store, &iter,
				PROFILE_COLUMN_RADIO, TRUE,
				PROFILE_COLUMN_TYPE, i,
				PROFILE_COLUMN_DEFAULT,
				(strcmp(profile->name, p) == 0) ? TRUE : FALSE,
				PROFILE_COLUMN_ONLINE, profile->online,
				PROFILE_COLUMN_VOLUME, profile->volume,
				PROFILE_COLUMN_VIBRATE, profile->vibrate,
				PROFILE_COLUMN_SAMPLE, profile->sample,
				PROFILE_COLUMN_ICON,
				gtk_icon_theme_load_icon(theme,
				(profile->icon != NULL)
				? profile->icon : "gnome-settings",
				16, 0, NULL),
				PROFILE_COLUMN_NAME, profile->name,
				PROFILE_COLUMN_NAME_DISPLAY, _(profile->name),
				-1);
	}
	if(i == profiles->profiles_cnt)
		i = PROFILE_TYPE_GENERAL;
	return _profiles_set(profiles, i);
}


/* profiles_play */
static void _profiles_play(Profiles * profiles, char const * sample,
		int vibrator)
{
	PhonePluginHelper * helper = profiles->helper;
	ProfileDefinition * profile = &profiles->profiles[
		profiles->profiles_cur];
	PhoneEvent event;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\") %s\n", __func__, sample,
			profile->name);
#endif
	if(sample == NULL)
	{
		/* cancel the current sample */
		memset(&event, 0, sizeof(event));
		event.type = PHONE_EVENT_TYPE_AUDIO_STOP;
		helper->event(helper->phone, &event);
	}
	else if(profile->volume != PROFILE_VOLUME_SILENT)
	{
		memset(&event, 0, sizeof(event));
		event.type = PHONE_EVENT_TYPE_AUDIO_PLAY;
		event.audio_play.sample = sample;
		helper->event(helper->phone, &event);
	}
	profiles->vibrator = max(profiles->vibrator, vibrator);
	if(vibrator == 0)
	{
		/* stop the vibrator */
		memset(&event, 0, sizeof(event));
		event.type = PHONE_EVENT_TYPE_VIBRATOR_OFF;
		helper->event(helper->phone, &event);
		/* remove the callback */
		if(profiles->source != 0)
			g_source_remove(profiles->source);
		profiles->source = 0;
		profiles->vibrator = 0;
	}
	else if(profile->vibrate && profiles->vibrator != 0)
	{
		memset(&event, 0, sizeof(event));
		event.type = PHONE_EVENT_TYPE_VIBRATOR_ON;
		helper->event(helper->phone, &event);
		if(profiles->source != 0)
			g_source_remove(profiles->source);
		profiles->source = g_timeout_add(PROFILE_VIBRATOR_LOOP,
				_profiles_on_vibrate, profiles);
	}
}


/* profiles_save */
static int _profiles_save(Profiles * profiles)
{
	int ret = 0;
	PhonePluginHelper * helper = profiles->helper;

	/* default profile */
	ret |= helper->config_set(helper->phone, "profiles", "default",
			profiles->profiles[profiles->profiles_cur].name);
	/* online */
	ret |= helper->config_set(helper->phone, NULL, "online",
			profiles->profiles[profiles->profiles_cur].online
			? NULL : "0");
	return ret;
}


/* profiles_switch */
static void _profiles_switch(Profiles * profiles, ProfileType type)
{
	PhonePluginHelper * helper = profiles->helper;
	ProfileType current = profiles->profiles_cur;
	PhoneEvent pevent;

	if(type == current)
		return;
	if(_profiles_set(profiles, type) != 0)
		return;
	_profiles_save(profiles);
	memset(&pevent, 0, sizeof(pevent));
	if(profiles->profiles[current].online
			&& !profiles->profiles[type].online)
	{
		/* go offline */
		pevent.type = PHONE_EVENT_TYPE_STOPPING;
		helper->event(helper->phone, &pevent);
	}
	else if(!profiles->profiles[current].online
			&& profiles->profiles[type].online)
	{
		/* go online */
		pevent.type = PHONE_EVENT_TYPE_STARTING;
		helper->event(helper->phone, &pevent);
	}
}


/* callbacks */
/* profiles_on_vibrate */
static gboolean _profiles_on_vibrate(gpointer data)
{
	Profiles * profiles = data;
	PhonePluginHelper * helper = profiles->helper;
	PhoneEvent event;

	memset(&event, 0, sizeof(event));
	if(profiles->vibrator < 0)
	{
		/* stop the vibrator */
		event.type = PHONE_EVENT_TYPE_VIBRATOR_OFF;
		helper->event(helper->phone, &event);
		/* vibrate again only if necessary */
		profiles->vibrator = (-profiles->vibrator) - 1;
	}
	else if(profiles->vibrator > 0)
	{
		/* start the vibrator */
		event.type = PHONE_EVENT_TYPE_VIBRATOR_ON;
		helper->event(helper->phone, &event);
		/* pause the vibrator next time */
		profiles->vibrator = -profiles->vibrator;
	}
	else
	{
		profiles->source = 0;
		return FALSE;
	}
	return TRUE;
}
