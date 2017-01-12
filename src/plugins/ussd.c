/* $Id$ */
/* Copyright (c) 2011-2016 Pierre Pronchery <khorben@defora.org> */
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
 * - use a configuration file instead */



#include <string.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"


/* USSD */
/* private */
/* types */
typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;
	size_t _operator;
	/* widgets */
	GtkWidget * window;
	GtkWidget * operators;
	GtkWidget * codes;
} USSD;

typedef struct _USSDCode
{
	char const * number;
	char const * name;
} USSDCode;


/* constants */
/* Germany */
static USSDCode _ussd_codes_de_ccc_32c3[] =
{
	{ "*#100#",	"Number request"				},
	{ NULL,		NULL						}
};

/* E-Plus, see http://www.prepaid-wiki.de/index.php5/E-Plus */
static USSDCode _ussd_codes_de_eplus[] =
{
	{ "*100#",	"Balance enquiry"				},
	{ NULL,		NULL						}
};

/* FYVE, see http://www.prepaid-wiki.de/index.php5/FYVE#Servicefunktionen */
static USSDCode _ussd_codes_de_fyve[] =
{
	{ "*100#",	"Balance enquiry and charging menu"		},
	{ "*106#",	"Balance enquiry"				},
	{ NULL,		NULL						}
};

/* MTN, see http://www.mtn.co.za/Support/faq/Pages/USSD.aspx */
static USSDCode _ussd_codes_za_mtn[] =
{
	{ "*141#",	"Balance enquiry"				},
	{ "*141*4#",	"Tariff Analyser and Priceplan Migrations menu"	},
	{ "*141*4*0#",	"Tariff Analyser"				},
	{ "*141*6*0#",	"Data bundle cancellation"			},
	{ "*141*7*0#",	"SMS bundle cancellation"			},
	{ "*141*8#",	"Yello Fortune Entries"				},
	{ NULL,		NULL						}
};

/* Telekom */
static USSDCode _ussd_codes_de_telekom[] =
{
	{ "*2221#",	"Multi-SIM: current status"			},
	{ "*2222#",	"Multi-SIM: set card as default"		},
	{ NULL,		NULL						}
};

/* Virgin Mobile, see
 * http://fr.wikipedia.org/wiki/Unstructured_Supplementary_Service_Data */
static USSDCode _ussd_codes_fr_virgin[] =
{
	{ "*144#",	"Balance enquiry"				},
	{ NULL,		NULL						}
};

/* Vodafone India, see
 * http://broadbandforum.in/vodafone-3g/66861-important-vodafone-sms-codes-ussd/
 */
static USSDCode _ussd_codes_in_vodafone[] =
{
	{ "*141#",	"Balance enquiry"				},
	{ "*444#",	"Self-service portal"				},
	{ "*225*6#",	"See unused GPRS data and expiry"		},
	{ "*225*3#",	"See unused 3G data and expiry"			},
	{ "*444*5#",	"Activate 30MB, 1 day, GPRS plan"		},
	{ NULL,		NULL						}
};

static const struct
{
	char const * name;
	char const * _operator;
	USSDCode * codes;
} _ussd_operators[] =
{
	/* FIXME obtain the corresponding operator names */
	{ "CCC 32C3",	NULL,		_ussd_codes_de_ccc_32c3		},
	{ "E-Plus",	NULL,		_ussd_codes_de_eplus		},
	{ "FYVE",	NULL,		_ussd_codes_de_fyve		},
	{ "Monacell",	NULL,		_ussd_codes_fr_virgin		},
	{ "MTN",	NULL,		_ussd_codes_za_mtn		},
	{ "NRJ",	NULL,		_ussd_codes_fr_virgin		},
	{ "Telekom",	"Telekom.de",	_ussd_codes_de_telekom		},
	{ "Virgin",	NULL,		_ussd_codes_fr_virgin		},
	{ "Vodafone",	NULL,		_ussd_codes_in_vodafone		},
	{ NULL,		NULL,		NULL				}
};


/* prototypes */
/* plugins */
static USSD * _ussd_init(PhonePluginHelper * helper);
static void _ussd_destroy(USSD * ussd);
static int _ussd_event(USSD * ussd, PhoneEvent * event);
static void _ussd_settings(USSD * ussd);

/* useful */
static int _ussd_load_operator(USSD * ussd, char const * _operator);

/* callbacks */
static void _ussd_on_operators_changed(gpointer data);
static void _ussd_on_settings_close(gpointer data);
static void _ussd_on_settings_send(gpointer data);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"USSD",
	"user-info",
	NULL,
	_ussd_init,
	_ussd_destroy,
	_ussd_event,
	_ussd_settings
};


/* private */
/* functions */
/* ussd_init */
static USSD * _ussd_init(PhonePluginHelper * helper)
{
	USSD * ussd;

	if((ussd = object_new(sizeof(*ussd))) == NULL)
		return NULL;
	ussd->helper = helper;
	ussd->_operator = 0;
	ussd->window = NULL;
	ussd->operators = NULL;
	ussd->codes = NULL;
	return ussd;
}


/* ussd_destroy */
static void _ussd_destroy(USSD * ussd)
{
	if(ussd->window != NULL)
		gtk_widget_destroy(ussd->window);
	object_delete(ussd);
}


/* ussd_event */
static int _event_modem(USSD * ussd, ModemEvent * event);

static int _ussd_event(USSD * ussd, PhoneEvent * event)
{
	switch(event->type)
	{
		case PHONE_EVENT_TYPE_MODEM_EVENT:
			return _event_modem(ussd, event->modem_event.event);
		default:
			/* not relevant */
			return 0;
	}
}

static int _event_modem(USSD * ussd, ModemEvent * event)
{
	switch(event->type)
	{
		case MODEM_EVENT_TYPE_REGISTRATION:
			_ussd_load_operator(ussd,
					event->registration._operator);
			/* XXX ignore errors */
			return 0;
		default:
			/* not relevant */
			return 0;
	}
}


/* ussd_settings */
static void _settings_window(USSD * ussd);

static void _ussd_settings(USSD * ussd)
{
	if(ussd->window == NULL)
		_settings_window(ussd);
	gtk_window_present(GTK_WINDOW(ussd->window));
}

static void _settings_window(USSD * ussd)
{
	GtkSizeGroup * group;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * image;
	GtkWidget * widget;
	size_t i;

	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	ussd->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(ussd->window), 4);
	gtk_window_set_default_size(GTK_WINDOW(ussd->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(ussd->window), "gnome-settings");
#endif
	gtk_window_set_title(GTK_WINDOW(ussd->window), "USSD");
	g_signal_connect(ussd->window, "delete-event", G_CALLBACK(
				gtk_widget_hide), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	/* operators */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Operator:");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	ussd->operators = gtk_combo_box_text_new();
	for(i = 0; _ussd_operators[i].name != NULL; i++)
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ussd->operators),
				NULL, _ussd_operators[i].name);
#else
	ussd->operators = gtk_combo_box_new_text();
	for(i = 0; _ussd_operators[i].name != NULL; i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(ussd->operators),
				_ussd_operators[i].name);
#endif
	g_signal_connect_swapped(ussd->operators, "changed", G_CALLBACK(
				_ussd_on_operators_changed), ussd);
	gtk_box_pack_start(GTK_BOX(hbox), ussd->operators, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* codes */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Code:");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(3, 0, 0)
	ussd->codes = gtk_combo_box_text_new();
#else
	ussd->codes = gtk_combo_box_new_text();
#endif
	gtk_box_pack_start(GTK_BOX(hbox), ussd->codes, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* send */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_button_new_with_label("Send request");
	image = gtk_image_new_from_icon_name("mail-send", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget), image);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_ussd_on_settings_send), ussd);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* button box */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_ussd_on_settings_close), ussd);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ussd->window), vbox);
	gtk_combo_box_set_active(GTK_COMBO_BOX(ussd->operators),
			ussd->_operator);
	gtk_widget_show_all(vbox);
}


/* useful */
/* ussd_load_operator */
static int _ussd_load_operator(USSD * ussd, char const * _operator)
{
	size_t i;

	if(_operator == NULL)
		return -1;
	for(i = 0; _ussd_operators[i].name != NULL; i++)
		if(_ussd_operators[i]._operator == NULL)
			continue;
		else if(strcmp(_ussd_operators[i]._operator, _operator) == 0)
		{
			ussd->_operator = i;
			if(ussd->window != NULL)
				gtk_combo_box_set_active(GTK_COMBO_BOX(
							ussd->operators), i);
			break;
		}
	return 0;
}


/* callbacks */
/* ussd_on_operators_changed */
static void _ussd_on_operators_changed(gpointer data)
{
	USSD * ussd = data;
	GtkTreeModel * model;
	int i;
	USSDCode * codes;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(ussd->codes));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	i = gtk_combo_box_get_active(GTK_COMBO_BOX(ussd->operators));
	codes = _ussd_operators[i].codes;
	for(i = 0; codes[i].name != NULL; i++)
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ussd->codes),
				NULL, codes[i].name);
#else
		gtk_combo_box_append_text(GTK_COMBO_BOX(ussd->codes),
				codes[i].name);
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(ussd->codes), 0);
}


/* ussd_on_settings_close */
static void _ussd_on_settings_close(gpointer data)
{
	USSD * ussd = data;

	gtk_widget_hide(ussd->window);
}


/* ussd_on_settings_send */
static void _ussd_on_settings_send(gpointer data)
{
	USSD * ussd = data;
	PhonePluginHelper * helper = ussd->helper;
	int i;
	USSDCode * codes;
	ModemRequest request;

	i = gtk_combo_box_get_active(GTK_COMBO_BOX(ussd->operators));
	codes = _ussd_operators[i].codes;
	i = gtk_combo_box_get_active(GTK_COMBO_BOX(ussd->codes));
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, codes[i].number);
#endif
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_CALL;
	request.call.call_type = MODEM_CALL_TYPE_VOICE;
	request.call.number = codes[i].number;
	helper->request(helper->phone, &request);
}
