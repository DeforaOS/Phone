/* $Id$ */
/* Copyright (c) 2010-2020 Pierre Pronchery <khorben@defora.org> */
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
#include <errno.h>
#include <openssl/sha.h>
#include <gtk/gtk.h>
#include <System.h>
#include "Phone.h"


/* SMSCrypt */
/* private */
/* types */
typedef enum _SMSCryptColumn
{
	SMSCC_NUMBER = 0,
	SMSCC_SECRET,
	SMSCC_NUMBER_PLACEHOLDER,
	SMSCC_SECRET_PLACEHOLDER
} SMSCryptColumn;
#define SMSCC_LAST SMSCC_SECRET_PLACEHOLDER
#define SMSCC_COUNT (SMSCC_LAST + 1)

typedef struct _PhonePlugin
{
	PhonePluginHelper * helper;

	/* internal */
	unsigned char buf[20];
	size_t len;
	/* widgets */
	GtkWidget * window;
	GtkListStore * store;
	GtkWidget * view;
} SMSCrypt;


/* prototypes */
static void _smscrypt_clear(SMSCrypt * smscrypt);
static gboolean _smscrypt_confirm(SMSCrypt * smscrypt, char const * message);
static SMSCrypt * _smscrypt_init(PhonePluginHelper * helper);
static void _smscrypt_destroy(SMSCrypt * smscrypt);
static int _smscrypt_event(SMSCrypt * smscrypt, PhoneEvent * event);
static int _smscrypt_secret(SMSCrypt * smscrypt, char const * number);
static void _smscrypt_settings(SMSCrypt * smscrypt);


/* public */
/* variables */
PhonePluginDefinition plugin =
{
	"SMS encryption",
	"application-certificate",
	NULL,
	_smscrypt_init,
	_smscrypt_destroy,
	_smscrypt_event,
	_smscrypt_settings
};


/* private */
/* functions */
/* smscrypt_clear */
static void _smscrypt_clear(SMSCrypt * smscrypt)
{
	memset(smscrypt->buf, 0, smscrypt->len);
}


/* smscrypt_confirm */
static gboolean _smscrypt_confirm(SMSCrypt * smscrypt, char const * message)
{
	GtkWidget * dialog;
	int res;
	(void) smscrypt;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO, "%s",
#if GTK_CHECK_VERSION(2, 6, 0)
			"Question");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
			"%s",
#endif
			message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Question");
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return (res == GTK_RESPONSE_YES) ? TRUE : FALSE;
}


/* smscrypt_init */
static void _init_foreach(char const * variable, char const * value,
		void * priv);

static SMSCrypt * _smscrypt_init(PhonePluginHelper * helper)
{
	SMSCrypt * smscrypt;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((smscrypt = object_new(sizeof(*smscrypt))) == NULL)
		return NULL;
	smscrypt->helper = helper;
	smscrypt->len = sizeof(smscrypt->buf);
	smscrypt->window = NULL;
	smscrypt->store = gtk_list_store_new(SMSCC_COUNT, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	helper->config_foreach(helper->phone, "smscrypt", _init_foreach,
			smscrypt);
	return smscrypt;
}

static void _init_foreach(char const * variable, char const * value,
		void * priv)
{
	SMSCrypt * smscrypt = priv;
	GtkTreeIter iter;

	gtk_list_store_append(smscrypt->store, &iter);
	gtk_list_store_set(smscrypt->store, &iter, SMSCC_NUMBER, variable,
			SMSCC_SECRET, value, -1);
}


/* smscrypt_destroy */
static void _smscrypt_destroy(SMSCrypt * smscrypt)
{
	if(smscrypt->window != NULL)
		gtk_widget_destroy(smscrypt->window);
	object_delete(smscrypt);
}


/* smscrypt_event */
static int _smscrypt_event_sms_receiving(SMSCrypt * smscrypt,
		char const * number, PhoneEncoding * encoding, char * buf,
		size_t * len);
static int _smscrypt_event_sms_sending(SMSCrypt * smscrypt,
		char const * number, PhoneEncoding * encoding, char * buf,
		size_t * len);

static int _smscrypt_event(SMSCrypt * smscrypt, PhoneEvent * event)
{
	int ret = 0;
#if 0 /* FIXME re-implement */
	char const * number;
	PhoneEncoding * encoding;
	char ** buf;
	size_t * len;

	switch(event->type)
	{
		/* our deal */
		case PHONE_EVENT_TYPE_SMS_RECEIVING:
			number = va_arg(ap, char const *);
			encoding = va_arg(ap, PhoneEncoding *);
			buf = va_arg(ap, char **);
			len = va_arg(ap, size_t *);
			ret = _smscrypt_event_sms_receiving(plugin, number,
					encoding, *buf, len);
			break;
		case PHONE_EVENT_TYPE_SMS_SENDING:
			number = va_arg(ap, char const *);
			encoding = va_arg(ap, PhoneEncoding *);
			buf = va_arg(ap, char **);
			len = va_arg(ap, size_t *);
			ret = _smscrypt_event_sms_sending(plugin, number,
					encoding, *buf, len);
			break;
		/* ignore the rest */
		default:
			break;
	}
#endif
	return ret;
}

static int _smscrypt_event_sms_receiving(SMSCrypt * smscrypt,
		char const * number, PhoneEncoding * encoding, char * buf,
		size_t * len)
{
	PhonePluginHelper * helper = smscrypt->helper;
	char const * error = "There is no known secret for this number."
		" The message could not be decrypted.";
	size_t i;
	size_t j = 0;
	SHA_CTX sha1;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, buf, %lu)\n", __func__, *encoding,
			(unsigned long)*len);
#endif
	if(*encoding != PHONE_ENCODING_DATA)
		return 0; /* not for us */
	if(_smscrypt_secret(smscrypt, number) != 0)
		return helper->error(helper->phone, error, 1);
	for(i = 0; i < *len; i++)
	{
		buf[i] ^= smscrypt->buf[j];
		smscrypt->buf[j++] ^= buf[i];
		if(j != smscrypt->len)
			continue;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, smscrypt->buf, smscrypt->len);
		SHA1_Final(smscrypt->buf, &sha1);
		j = 0;
	}
	*encoding = PHONE_ENCODING_UTF8;
	_smscrypt_clear(smscrypt);
	return 0;
}

static int _smscrypt_event_sms_sending(SMSCrypt * smscrypt,
		char const * number, PhoneEncoding * encoding, char * buf,
		size_t * len)
{
	char const * confirm = "There is no secret defined for this number."
		" The message will be sent unencrypted.\nContinue?";
	size_t i;
	size_t j = 0;
	SHA_CTX sha1;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %u, buf, %lu)\n", __func__, number,
			(unsigned)*encoding, (unsigned long)*len);
#endif
	if(*encoding != PHONE_ENCODING_UTF8)
		return 0; /* not for us */
	if(_smscrypt_secret(smscrypt, number) != 0)
		return (_smscrypt_confirm(smscrypt, confirm) == TRUE) ? 0 : 1;
	*encoding = PHONE_ENCODING_DATA;
	for(i = 0; i < *len; i++)
	{
		buf[i] ^= smscrypt->buf[j];
		smscrypt->buf[j++] = buf[i];
		if(j != smscrypt->len)
			continue;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, smscrypt->buf, smscrypt->len);
		SHA1_Final(smscrypt->buf, &sha1);
		j = 0;
	}
	*encoding = PHONE_ENCODING_DATA;
	_smscrypt_clear(smscrypt);
	return 0;
}


/* smscrypt_secret */
static int _smscrypt_secret(SMSCrypt * smscrypt, char const * number)
{
	PhonePluginHelper * helper = smscrypt->helper;
	char const * secret = NULL;
	SHA_CTX sha1;

	if(number != NULL)
		secret = helper->config_get(helper->phone, "smscrypt", number);
	if(secret == NULL)
		secret = helper->config_get(helper->phone, "smscrypt",
				"secret");
	if(secret == NULL)
		return 1;
	SHA1_Init(&sha1);
	SHA1_Update(&sha1, (unsigned char const *)secret, strlen(secret));
	SHA1_Final(smscrypt->buf, &sha1);
	return 0;
}


/* smscrypt_settings */
static gboolean _on_settings_closex(gpointer data);
static void _on_settings_delete(gpointer data);
static void _on_settings_new(gpointer data);
static void _on_settings_number_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data);
static void _on_settings_secret_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data);

static void _smscrypt_settings(SMSCrypt * smscrypt)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	if(smscrypt->window != NULL)
	{
		gtk_window_present(GTK_WINDOW(smscrypt->window));
		return;
	}
	smscrypt->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(smscrypt->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	/* XXX find something more appropriate */
	gtk_window_set_icon_name(GTK_WINDOW(smscrypt->window), "smscrypt");
#endif
	gtk_window_set_title(GTK_WINDOW(smscrypt->window), "SMS encryption");
	g_signal_connect_swapped(smscrypt->window, "delete-event", G_CALLBACK(
				_on_settings_closex), smscrypt);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* toolbar */
	widget = gtk_toolbar_new();
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_on_settings_new), smscrypt);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_on_settings_delete), smscrypt);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	smscrypt->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				smscrypt->store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(smscrypt->view), TRUE);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_on_settings_number_edited), smscrypt);
	column = gtk_tree_view_column_new_with_attributes("Number", renderer,
			"text", SMSCC_NUMBER,
			"placeholder-text", SMSCC_NUMBER_PLACEHOLDER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(smscrypt->view), column);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_on_settings_secret_edited), smscrypt);
	column = gtk_tree_view_column_new_with_attributes("Secret", renderer,
			"text", SMSCC_SECRET,
			"placeholder-text", SMSCC_SECRET_PLACEHOLDER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(smscrypt->view), column);
	gtk_container_add(GTK_CONTAINER(widget), smscrypt->view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(smscrypt->window), vbox);
	gtk_widget_show_all(smscrypt->window);
}

static gboolean _on_settings_closex(gpointer data)
{
	SMSCrypt * smscrypt = data;

	gtk_widget_hide(smscrypt->window);
	return TRUE;
}

static void _on_settings_delete(gpointer data)
{
	SMSCrypt * smscrypt = data;
	PhonePluginHelper * helper = smscrypt->helper;
	GtkTreeSelection * treesel;
	GtkTreeIter iter;
	char * number = NULL;

	if((treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
						smscrypt->view))) == NULL)
		return;
	if(gtk_tree_selection_get_selected(treesel, NULL, &iter) != TRUE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(smscrypt->store), &iter,
			SMSCC_NUMBER, &number, -1);
	if(number == NULL)
		return;
	helper->config_set(helper->phone, "smscrypt", number, NULL);
	gtk_list_store_remove(smscrypt->store, &iter);
	g_free(number);
}

static void _on_settings_new(gpointer data)
{
	SMSCrypt * smscrypt = data;
	GtkTreeIter iter;

	gtk_list_store_append(smscrypt->store, &iter);
	gtk_list_store_set(smscrypt->store, &iter,
			SMSCC_NUMBER_PLACEHOLDER, "Number",
			SMSCC_SECRET_PLACEHOLDER, "Secret", -1);
}

static void _on_settings_number_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data)
{
	SMSCrypt * smscrypt = data;
	PhonePluginHelper * helper = smscrypt->helper;
	GtkTreeModel * model = GTK_TREE_MODEL(smscrypt->store);
	GtkTreeIter iter;
	char * number = NULL;
	char const * secret;

	if(gtk_tree_model_get_iter_from_string(model, &iter, arg1) == TRUE)
		gtk_tree_model_get(model, &iter, SMSCC_NUMBER, &number, -1);
	if(number == NULL)
		return;
	/* FIXME check that there are no duplicates */
	secret = helper->config_get(helper->phone, "smscrypt", number);
	/* XXX report errors */
	if(helper->config_set(helper->phone, "smscrypt", arg2, secret) == 0
			&& helper->config_set(helper->phone, "smscrypt", number,
				NULL) == 0)
		gtk_list_store_set(smscrypt->store, &iter,
				SMSCC_NUMBER, arg2, -1);
	g_free(number);
}

static void _on_settings_secret_edited(GtkCellRenderer * renderer, gchar * arg1,
		gchar * arg2, gpointer data)
{
	SMSCrypt * smscrypt = data;
	PhonePluginHelper * helper = smscrypt->helper;
	GtkTreeModel * model = GTK_TREE_MODEL(smscrypt->store);
	GtkTreeIter iter;
	char * number = NULL;

	if(gtk_tree_model_get_iter_from_string(model, &iter, arg1) == TRUE)
		gtk_tree_model_get(model, &iter, SMSCC_NUMBER, &number, -1);
	if(number == NULL)
		return;
	/* XXX report errors */
	if(helper->config_set(helper->phone, "smscrypt", number, arg2) == 0)
		gtk_list_store_set(smscrypt->store, &iter, SMSCC_SECRET, arg2,
				-1);
	g_free(number);
}
