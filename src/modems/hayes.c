/* $Id$ */
/* Copyright (c) 2011-2020 Pierre Pronchery <khorben@defora.org> */
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
 * - implement priorities again
 * - parse messages from within +CMGL already
 * - add MCT callbacks/buttons to change the SIM code (via a helper in phone.c)
 * - implement new contacts
 * - implement +PBREADY?
 * - implement AT+CTZR=1 (timezone updates)
 * - implement AT+CLCK="PN",2? (is SIM-locked)
 * - implement AT+XREG? (band frequency, Sierra Wireless only?)
 * - implement AT+XACT? (band frequency again, same) */



#ifdef __linux__
# include <sys/file.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>
#include <libgen.h>
#include <glib.h>
#include <System.h>
#include <Phone/modem.h>
#include "hayes/channel.h"
#include "hayes/command.h"
#include "hayes/common.h"
#include "hayes/pdu.h"
#include "hayes/quirks.h"
#include "hayes.h"

/* constants */
#ifndef PROGNAME_PPPD
# define PROGNAME_PPPD	"pppd"
#endif

/* macros */
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))


/* Hayes */
/* private */
/* types */
typedef struct _ModemPlugin
{
	ModemPluginHelper * helper;

	unsigned int retry;

	/* modem */
	HayesChannel channel;
} Hayes;

#ifdef DEBUG
static const char * _hayes_command_status[HCS_COUNT] =
{
	"HCS_UNKNOWN", "HCS_QUEUED", "HCS_UNKNOWN", "HCS_ACTIVE", "HCS_TIMEOUT",
	"HCS_ERROR", "HCS_SUCCESS"
};
#endif

typedef struct _HayesRequestContactList
{
	unsigned int from;
	unsigned int to;
} HayesRequestContactList;

typedef struct _HayesRequestMessageData
{
	unsigned int id;
	ModemMessageFolder folder;
	ModemMessageStatus status;
} HayesRequestMessageData;

typedef struct _HayesRequestHandler
{
	unsigned int type;
	char const * attention;
	HayesCommandCallback callback;
} HayesRequestHandler;

typedef struct _HayesCodeHandler
{
	char const * code;
	void (*callback)(HayesChannel * channel, char const * answer);
} HayesCodeHandler;


/* constants */
enum
{
	HAYES_REQUEST_ALIVE = MODEM_REQUEST_COUNT,
	HAYES_REQUEST_CALL_WAITING_UNSOLLICITED_DISABLE,
	HAYES_REQUEST_CALL_WAITING_UNSOLLICITED_ENABLE,
	HAYES_REQUEST_CONNECTED_LINE_DISABLE,
	HAYES_REQUEST_CONNECTED_LINE_ENABLE,
	HAYES_REQUEST_CONTACT_LIST,
	HAYES_REQUEST_EXTENDED_ERRORS,
	HAYES_REQUEST_EXTENDED_RING_REPORTS,
	HAYES_REQUEST_FUNCTIONAL,
	HAYES_REQUEST_FUNCTIONAL_DISABLE,
	HAYES_REQUEST_FUNCTIONAL_ENABLE,
	HAYES_REQUEST_FUNCTIONAL_ENABLE_RESET,
	HAYES_REQUEST_GPRS_ATTACHED,
	HAYES_REQUEST_LOCAL_ECHO_DISABLE,
	HAYES_REQUEST_LOCAL_ECHO_ENABLE,
	HAYES_REQUEST_MESSAGE_FORMAT_PDU,
	HAYES_REQUEST_MESSAGE_LIST_INBOX_READ,
	HAYES_REQUEST_MESSAGE_LIST_INBOX_UNREAD,
	HAYES_REQUEST_MESSAGE_LIST_SENT_READ,
	HAYES_REQUEST_MESSAGE_LIST_SENT_UNREAD,
	HAYES_REQUEST_MESSAGE_UNSOLLICITED_DISABLE,
	HAYES_REQUEST_MESSAGE_UNSOLLICITED_ENABLE,
	HAYES_REQUEST_MODEL,
	HAYES_REQUEST_OPERATOR,
	HAYES_REQUEST_OPERATOR_FORMAT_SHORT,
	HAYES_REQUEST_OPERATOR_FORMAT_LONG,
	HAYES_REQUEST_OPERATOR_FORMAT_NUMERIC,
	HAYES_REQUEST_PHONE_ACTIVE,
	HAYES_REQUEST_REGISTRATION,
	HAYES_REQUEST_REGISTRATION_AUTOMATIC,
	HAYES_REQUEST_REGISTRATION_DISABLED,
	HAYES_REQUEST_REGISTRATION_UNSOLLICITED_DISABLE,
	HAYES_REQUEST_REGISTRATION_UNSOLLICITED_ENABLE,
	HAYES_REQUEST_SERIAL_NUMBER,
	HAYES_REQUEST_SIM_PIN_VALID,
	HAYES_REQUEST_SUBSCRIBER_IDENTITY,
	HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_CANCEL,
	HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_ENABLE,
	HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_DISABLE,
	HAYES_REQUEST_VENDOR,
	HAYES_REQUEST_VERBOSE_DISABLE,
	HAYES_REQUEST_VERBOSE_ENABLE,
	HAYES_REQUEST_VERSION
};


/* prototypes */
/* plug-in */
static ModemPlugin * _hayes_init(ModemPluginHelper * helper);
static void _hayes_destroy(Hayes * hayes);
static int _hayes_start(Hayes * hayes, unsigned int retry);
static int _hayes_stop(Hayes * hayes);
static int _hayes_request(Hayes * hayes, ModemRequest * request);
static int _hayes_trigger(Hayes * hayes, ModemEventType event);

/* accessors */
static void _hayes_set_mode(Hayes * hayes, HayesChannel * channel,
		HayesChannelMode mode);

/* useful */
/* conversions */
static unsigned char _hayes_convert_gsm_to_iso(unsigned char c);
static void _hayes_convert_gsm_string_to_iso(char * str);
static unsigned char _hayes_convert_iso_to_gsm(unsigned char c);
static void _hayes_convert_iso_string_to_gsm(char * str);

/* messages */
static char * _hayes_message_to_pdu(HayesChannel * channel, char const * number,
		ModemMessageEncoding encoding, size_t length,
		char const * content);

/* logging */
static void _hayes_log(Hayes * hayes, HayesChannel * channel,
		char const * prefix, char const * buf, size_t cnt);

/* parser */
static int _hayes_parse(Hayes * hayes, HayesChannel * channel);
static int _hayes_parse_pdu(Hayes * hayes, HayesChannel * channel);
static int _hayes_parse_trigger(HayesChannel * channel, char const * answer,
		HayesCommand * command);

/* queue */
static int _hayes_queue_command(Hayes * hayes, HayesChannel * channel,
		HayesCommand * command);
#if 0 /* XXX no longer used */
static int _hayes_queue_command_full(Hayes * hayes,
		char const * attention, HayesCommandCallback callback);
#endif
static int _hayes_queue_push(Hayes * hayes, HayesChannel * channel);

/* requests */
static int _hayes_request_channel(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void * data);
static int _hayes_request_type(Hayes * hayes, HayesChannel * channel,
		ModemRequestType type);

/* reset */
static void _hayes_reset(Hayes * hayes);

/* callbacks */
static gboolean _on_channel_authenticate(gpointer data);
static gboolean _on_channel_reset(gpointer data);
static gboolean _on_channel_timeout(gpointer data);
static gboolean _on_queue_timeout(gpointer data);
static gboolean _on_reset_settle(gpointer data);
static gboolean _on_reset_settle2(gpointer data);
static gboolean _on_watch_can_read(GIOChannel * source, GIOCondition condition,
		gpointer data);
static gboolean _on_watch_can_read_ppp(GIOChannel * source,
		GIOCondition condition, gpointer data);
static gboolean _on_watch_can_write(GIOChannel * source, GIOCondition condition,
		gpointer data);
static gboolean _on_watch_can_write_ppp(GIOChannel * source,
		GIOCondition condition, gpointer data);

static HayesCommandStatus _on_request_authenticate(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_battery_level(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_call(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_call_incoming(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_call_outgoing(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_call_status(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_contact_delete(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_contact_list(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_functional(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_functional_enable(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_functional_enable_reset(
		HayesCommand * command, HayesCommandStatus status,
		HayesChannel * channel);
static HayesCommandStatus _on_request_generic(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_message(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_message_delete(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_message_list(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_message_send(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_model(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_registration(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_registration_automatic(
		HayesCommand * command, HayesCommandStatus status,
		HayesChannel * channel);
static HayesCommandStatus _on_request_registration_disabled(
		HayesCommand * command, HayesCommandStatus status,
		HayesChannel * channel);
static HayesCommandStatus _on_request_sim_pin_valid(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);
static HayesCommandStatus _on_request_unsupported(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);

static void _on_code_call_error(HayesChannel * channel, char const * answer);
static void _on_code_cbc(HayesChannel * channel, char const * answer);
static void _on_code_cfun(HayesChannel * channel, char const * answer);
static void _on_code_cgatt(HayesChannel * channel, char const * answer);
static void _on_code_cgmi(HayesChannel * channel, char const * answer);
static void _on_code_cgmm(HayesChannel * channel, char const * answer);
static void _on_code_cgmr(HayesChannel * channel, char const * answer);
static void _on_code_cgsn(HayesChannel * channel, char const * answer);
static void _on_code_cimi(HayesChannel * channel, char const * answer);
static void _on_code_clip(HayesChannel * channel, char const * answer);
static void _on_code_cme_error(HayesChannel * channel, char const * answer);
static void _on_code_cmgl(HayesChannel * channel, char const * answer);
static void _on_code_cmgr(HayesChannel * channel, char const * answer);
static void _on_code_cmgs(HayesChannel * channel, char const * answer);
static void _on_code_cms_error(HayesChannel * channel, char const * answer);
static void _on_code_cmti(HayesChannel * channel, char const * answer);
static void _on_code_connect(HayesChannel * channel, char const * answer);
static void _on_code_colp(HayesChannel * channel, char const * answer);
static void _on_code_cops(HayesChannel * channel, char const * answer);
static void _on_code_cpas(HayesChannel * channel, char const * answer);
static void _on_code_cpbr(HayesChannel * channel, char const * answer);
static void _on_code_cpin(HayesChannel * channel, char const * answer);
static void _on_code_creg(HayesChannel * channel, char const * answer);
static void _on_code_cring(HayesChannel * channel, char const * answer);
static void _on_code_csq(HayesChannel * channel, char const * answer);
static void _on_code_cusd(HayesChannel * channel, char const * answer);
static void _on_code_ext_error(HayesChannel * channel, char const * answer);

/* helpers */
static int _is_ussd_code(char const * number);


/* variables */
static ModemConfig _hayes_config[] =
{
	{ "device",	"Device",		MCT_FILENAME	},
	{ "baudrate",	"Baudrate",		MCT_UINT32	},
	{ "hwflow",	"Hardware flow control",MCT_BOOLEAN	},
	{ NULL,		"Advanced",		MCT_SUBSECTION	},
	{ "logfile",	"Log file",		MCT_FILENAME	},
	{ NULL,		NULL,			MCT_NONE	},
};

static struct
{
	unsigned char gsm;
	unsigned char iso;
} _hayes_gsm_iso[] =
{
	{ '\0', '@'	},
	{ 0x01, 163	}, /* £ */
	{ 0x02, '$'	},
	{ 0x03, 165	}, /* ¥ */
	{ 0x04, 232	}, /* è */
	{ 0x05, 233	}, /* é */
	{ 0x06, 249	}, /* ù */
	{ 0x07, 236	}, /* ì */
	{ 0x08, 242	}, /* ò */
	{ 0x09, 199	}, /* Ç */
	{ 0x0b, 216	}, /* Ø */
	{ 0x0c, 248	}, /* ø */
	{ 0x0e, 197	}, /* Å */
	{ 0x0f, 229	}, /* å */
	{ 0x10, ' '	}, /* XXX delta */
	{ 0x11, '_'	},
	{ 0x12, ' '	}, /* XXX phi */
	{ 0x13, ' '	}, /* XXX gamma */
	{ 0x14, ' '	}, /* XXX lambda */
	{ 0x15, ' '	}, /* XXX omega */
	{ 0x16, ' '	}, /* XXX pi */
	{ 0x17, ' '	}, /* XXX psi */
	{ 0x18, ' '	}, /* XXX sigma */
	{ 0x19, ' '	}, /* XXX theta */
	{ 0x1a, ' '	}, /* XXX xi */
	{ 0x1b, ' '	}, /* FIXME escape */
	{ 0x1c, 198	},
	{ 0x1d, 230	},
	{ 0x1e, 223	},
	{ 0x1f, 201	},
	{ 0x24, 164	}, /* $ */
	{ 0x40, 161	}, /* @ */
	{ 0x5b, 196	}, /* [ */
	{ 0x5c, 214	}, /* \ */
	{ 0x5d, 209	}, /* ] */
	{ 0x5e, 220	}, /* ^ */
	{ 0x5f, 167	}, /* _ */
	{ 0x60, 191	}, /* ` */
	{ 0x7b, 228	}, /* { */
	{ 0x7c, 246	}, /* | */
	{ 0x7d, 241	}, /* } */
	{ 0x7e, 252	}, /* ~ */
	{ 0x7f, 224	}
};

static HayesRequestHandler _hayes_request_handlers[] =
{
	{ HAYES_REQUEST_ALIVE,				"AT",
		_on_request_generic },
	{ HAYES_REQUEST_CALL_WAITING_UNSOLLICITED_DISABLE,"AT+CCWA=1",
		_on_request_generic },
	{ HAYES_REQUEST_CALL_WAITING_UNSOLLICITED_ENABLE,"AT+CCWA=1",
		_on_request_generic },
	{ HAYES_REQUEST_CONNECTED_LINE_DISABLE,		"AT+COLP=0",
		_on_request_generic },
	{ HAYES_REQUEST_CONNECTED_LINE_ENABLE,		"AT+COLP=1",
		_on_request_generic },
	{ HAYES_REQUEST_CONTACT_LIST,			NULL,
		_on_request_generic },
	{ HAYES_REQUEST_EXTENDED_ERRORS,		"AT+CMEE=1",
		_on_request_generic },
	{ HAYES_REQUEST_EXTENDED_RING_REPORTS,		"AT+CRC=1",
		_on_request_generic },
	{ HAYES_REQUEST_FUNCTIONAL,			"AT+CFUN?",
		_on_request_functional },
	{ HAYES_REQUEST_FUNCTIONAL_DISABLE,		"AT+CFUN=0",
		_on_request_generic },
	{ HAYES_REQUEST_FUNCTIONAL_ENABLE,		"AT+CFUN=1",
		_on_request_functional_enable },
	/* XXX AT+CFUN=16 on Sierra Wireless? */
	{ HAYES_REQUEST_FUNCTIONAL_ENABLE_RESET,	"AT+CFUN=1,1",
		_on_request_functional_enable_reset },
	{ HAYES_REQUEST_GPRS_ATTACHED,			"AT+CGATT?",
		_on_request_generic },
	{ HAYES_REQUEST_LOCAL_ECHO_DISABLE,		"ATE0",
		_on_request_generic },
	{ HAYES_REQUEST_LOCAL_ECHO_ENABLE,		"ATE1",
		_on_request_generic },
	{ HAYES_REQUEST_MESSAGE_FORMAT_PDU,		"AT+CMGF=0",
		_on_request_generic },
	{ HAYES_REQUEST_MESSAGE_LIST_INBOX_UNREAD,	"AT+CMGL=0",
		_on_request_message_list },
	{ HAYES_REQUEST_MESSAGE_LIST_INBOX_READ,	"AT+CMGL=1",
		_on_request_message_list },
	{ HAYES_REQUEST_MESSAGE_LIST_SENT_UNREAD,	"AT+CMGL=2",
		_on_request_message_list },
	{ HAYES_REQUEST_MESSAGE_LIST_SENT_READ,		"AT+CMGL=3",
		_on_request_message_list },
	{ HAYES_REQUEST_MESSAGE_UNSOLLICITED_DISABLE,	"AT+CNMI=0",
		_on_request_generic },
	{ HAYES_REQUEST_MESSAGE_UNSOLLICITED_ENABLE,	"AT+CNMI=1,1",
		_on_request_generic }, /* XXX report error? */
	{ HAYES_REQUEST_MODEL,				"AT+CGMM",
		_on_request_model },
	{ HAYES_REQUEST_OPERATOR,			"AT+COPS?",
		_on_request_generic },
	{ HAYES_REQUEST_OPERATOR_FORMAT_LONG,		"AT+COPS=3,0",
		_on_request_registration },
	{ HAYES_REQUEST_OPERATOR_FORMAT_NUMERIC,	"AT+COPS=3,2",
		_on_request_registration },
	{ HAYES_REQUEST_OPERATOR_FORMAT_SHORT,		"AT+COPS=3,1",
		_on_request_registration },
	{ HAYES_REQUEST_PHONE_ACTIVE,			"AT+CPAS",
		_on_request_call },
	{ HAYES_REQUEST_REGISTRATION,			"AT+CREG?",
		_on_request_generic },
	{ HAYES_REQUEST_REGISTRATION_AUTOMATIC,		"AT+COPS=0",
		_on_request_registration_automatic },
	{ HAYES_REQUEST_REGISTRATION_DISABLED,		"AT+COPS=2",
		_on_request_registration_disabled },
	{ HAYES_REQUEST_REGISTRATION_UNSOLLICITED_DISABLE,"AT+CREG=0",
		_on_request_generic },
	{ HAYES_REQUEST_REGISTRATION_UNSOLLICITED_ENABLE,"AT+CREG=2",
		_on_request_registration },
	{ HAYES_REQUEST_SERIAL_NUMBER,			"AT+CGSN",
		_on_request_generic },
	{ HAYES_REQUEST_SIM_PIN_VALID,			"AT+CPIN?",
		_on_request_sim_pin_valid },
	{ HAYES_REQUEST_SUBSCRIBER_IDENTITY,		"AT+CIMI",
		_on_request_generic },
	{ HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_CANCEL,"AT+CUSD=2",
		_on_request_generic },
	{ HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_DISABLE,"AT+CUSD=0",
		_on_request_generic },
	{ HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_ENABLE,"AT+CUSD=1",
		_on_request_generic },
	{ HAYES_REQUEST_VENDOR,				"AT+CGMI",
		_on_request_generic },
	{ HAYES_REQUEST_VERBOSE_DISABLE,		"ATV0",
		_on_request_generic },
	{ HAYES_REQUEST_VERBOSE_ENABLE,			"ATV1",
		_on_request_generic },
	{ HAYES_REQUEST_VERSION,			"AT+CGMR",
		_on_request_generic },
	{ MODEM_REQUEST_AUTHENTICATE,			NULL,
		_on_request_authenticate },
	{ MODEM_REQUEST_BATTERY_LEVEL,			"AT+CBC",
		_on_request_battery_level },
	{ MODEM_REQUEST_CALL,				NULL,
		_on_request_call_outgoing },
	{ MODEM_REQUEST_CALL_ANSWER,			"ATA",
		_on_request_call_incoming },
	{ MODEM_REQUEST_CALL_HANGUP,			NULL,
		_on_request_call_status },
	{ MODEM_REQUEST_CALL_PRESENTATION,		NULL,
		_on_request_generic },
	{ MODEM_REQUEST_CONNECTIVITY,			NULL,
		_on_request_generic },
	{ MODEM_REQUEST_CONTACT_DELETE,			NULL,
		_on_request_contact_delete },
	{ MODEM_REQUEST_CONTACT_EDIT,			NULL,
		_on_request_contact_list },
	{ MODEM_REQUEST_CONTACT_LIST,			"AT+CPBR=?",
		_on_request_generic },
	{ MODEM_REQUEST_CONTACT_NEW,			NULL,
		_on_request_contact_list },
	{ MODEM_REQUEST_DTMF_SEND,			NULL,
		_on_request_generic },
	{ MODEM_REQUEST_MESSAGE,			NULL,
		_on_request_message },
	{ MODEM_REQUEST_MESSAGE_DELETE,			NULL,
		_on_request_message_delete },
	{ MODEM_REQUEST_MESSAGE_LIST,			NULL,
		_on_request_message_list },
	{ MODEM_REQUEST_MESSAGE_SEND,			NULL,
		_on_request_message_send },
	{ MODEM_REQUEST_PASSWORD_SET,			NULL,
		_on_request_generic },
	{ MODEM_REQUEST_REGISTRATION,			NULL,
		_on_request_registration },
	{ MODEM_REQUEST_SIGNAL_LEVEL,			"AT+CSQ",
		_on_request_generic },
	{ MODEM_REQUEST_UNSUPPORTED,			NULL,
		_on_request_unsupported }
};

static HayesCodeHandler _hayes_code_handlers[] =
{
	{ "+CBC",	_on_code_cbc		},
	{ "+CFUN",	_on_code_cfun		},
	{ "+CGATT",	_on_code_cgatt		},
	{ "+CGMI",	_on_code_cgmi		},
	{ "+CGMM",	_on_code_cgmm		},
	{ "+CGMR",	_on_code_cgmr		},
	{ "+CGSN",	_on_code_cgsn		},
	{ "+CIMI",	_on_code_cimi		},
	{ "+CLIP",	_on_code_clip		},
	{ "+CME ERROR",	_on_code_cme_error	},
	{ "+CMGL",	_on_code_cmgl		},
	{ "+CMGR",	_on_code_cmgr		},
	{ "+CMGS",	_on_code_cmgs		},
	{ "+CMS ERROR",	_on_code_cms_error	},
	{ "+CMTI",	_on_code_cmti		},
	{ "+COLP",	_on_code_colp		},
	{ "+COPS",	_on_code_cops		},
	{ "+CPAS",	_on_code_cpas		},
	{ "+CPBR",	_on_code_cpbr		},
	{ "+CPIN",	_on_code_cpin		},
	{ "+CREG",	_on_code_creg		},
	{ "+CRING",	_on_code_cring		},
	{ "+CSQ",	_on_code_csq		},
	{ "+CUSD",	_on_code_cusd		},
	{ "+EXT ERROR",	_on_code_ext_error	},
	{ "BUSY",	_on_code_call_error	},
	{ "CONNECT",	_on_code_connect	},
	{ "NO CARRIER",	_on_code_call_error	},
	{ "NO DIALTONE",_on_code_call_error	},
	{ "RING",	_on_code_cring		}
};


/* public */
/* variables */
ModemPluginDefinition plugin =
{
	"Hayes",
	"phone",
	_hayes_config,
	_hayes_init,
	_hayes_destroy,
	_hayes_start,
	_hayes_stop,
	_hayes_request,
	_hayes_trigger
};


/* private */
/* plug-in */
/* functions */
static ModemPlugin * _hayes_init(ModemPluginHelper * helper)
{
	Hayes * hayes;

	if((hayes = object_new(sizeof(*hayes))) == NULL)
		return NULL;
	memset(hayes, 0, sizeof(*hayes));
	hayes->helper = helper;
	hayeschannel_init(&hayes->channel, hayes);
	return hayes;
}


/* hayes_destroy */
static void _hayes_destroy(Hayes * hayes)
{
	_hayes_stop(hayes);
	hayeschannel_destroy(&hayes->channel);
	object_delete(hayes);
}


/* hayes_request */
static int _hayes_request(Hayes * hayes, ModemRequest * request)
{
	return _hayes_request_channel(hayes, &hayes->channel, request, NULL);
}


/* hayes_start */
static int _start_is_started(Hayes * hayes);

static int _hayes_start(Hayes * hayes, unsigned int retry)
{
	hayes->retry = retry;
	if(_start_is_started(hayes))
		return 0;
	if(hayes->channel.source != 0)
		g_source_remove(hayes->channel.source);
	hayes->channel.source = g_idle_add(_on_channel_reset, &hayes->channel);
	return 0;
}

static int _start_is_started(Hayes * hayes)
{
	if(hayes->channel.source != 0)
		return 1;
	if(hayeschannel_is_started(&hayes->channel))
		return 1;
	return 0;
}


/* hayes_stop */
static int _hayes_stop(Hayes * hayes)
{
	HayesChannel * channel = &hayes->channel;
	ModemEvent * event;

	hayescommon_source_reset(&channel->source);
	hayeschannel_stop(channel);
	/* report disconnection if already connected */
	event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];
	if(event->connection.connected)
	{
		event->connection.connected = 0;
		event->connection.in = 0;
		event->connection.out = 0;
		hayes->helper->event(hayes->helper->modem, event);
	}
	/* reset battery information */
	event = &channel->events[MODEM_EVENT_TYPE_BATTERY_LEVEL];
	if(event->battery_level.status != MODEM_BATTERY_STATUS_UNKNOWN)
	{
		event->battery_level.status = MODEM_BATTERY_STATUS_UNKNOWN;
		event->battery_level.level = 0.0 / 0.0;
		event->battery_level.charging = 0;
		hayes->helper->event(hayes->helper->modem, event);
	}
	return 0;
}


/* hayes_trigger */
static int _hayes_trigger(Hayes * hayes, ModemEventType event)
{
	int ret = 0;
	HayesChannel * channel = &hayes->channel;
	ModemEvent * e;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, event);
#endif
	switch(event)
	{
		case MODEM_EVENT_TYPE_BATTERY_LEVEL: /* use the existing data */
		case MODEM_EVENT_TYPE_CALL:
		case MODEM_EVENT_TYPE_CONNECTION:
		case MODEM_EVENT_TYPE_STATUS:
			e = &channel->events[event];
			hayes->helper->event(hayes->helper->modem, e);
			break;
		case MODEM_EVENT_TYPE_AUTHENTICATION:
			return _hayes_request_type(hayes, channel,
					HAYES_REQUEST_SIM_PIN_VALID);
		case MODEM_EVENT_TYPE_CONTACT:
			return _hayes_request_type(hayes, channel,
					MODEM_REQUEST_CONTACT_LIST);
		case MODEM_EVENT_TYPE_MESSAGE:
			return _hayes_request_type(hayes, channel,
					MODEM_REQUEST_MESSAGE_LIST);
		case MODEM_EVENT_TYPE_MODEL:
			ret |= _hayes_request_type(hayes, channel,
					HAYES_REQUEST_VENDOR);
			ret |= _hayes_request_type(hayes, channel,
					HAYES_REQUEST_VERSION);
			ret |= _hayes_request_type(hayes, channel,
					HAYES_REQUEST_SERIAL_NUMBER);
			ret |= _hayes_request_type(hayes, channel,
					HAYES_REQUEST_SUBSCRIBER_IDENTITY);
			ret |= _hayes_request_type(hayes, channel,
					HAYES_REQUEST_MODEL);
			break;
		case MODEM_EVENT_TYPE_REGISTRATION:
			e = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			if(e->registration.status
					== MODEM_REGISTRATION_STATUS_UNKNOWN)
				ret |= _hayes_request_type(hayes, channel,
						HAYES_REQUEST_REGISTRATION);
			else
				hayes->helper->event(hayes->helper->modem, e);
			break;
		case MODEM_EVENT_TYPE_CONTACT_DELETED: /* do not make sense */
		case MODEM_EVENT_TYPE_ERROR:
		case MODEM_EVENT_TYPE_NOTIFICATION:
		case MODEM_EVENT_TYPE_MESSAGE_DELETED:
		case MODEM_EVENT_TYPE_MESSAGE_SENT:
			ret = -1;
			break;
	}
	return ret;
}


/* accessors */
/* hayes_set_mode */
static void _hayes_set_mode(Hayes * hayes, HayesChannel * channel,
		HayesChannelMode mode)
{
	ModemEvent * event;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, mode);
#endif
	if(channel->mode == mode)
		return;
	switch(channel->mode)
	{
		case HAYESCHANNEL_MODE_INIT:
		case HAYESCHANNEL_MODE_COMMAND:
		case HAYESCHANNEL_MODE_PDU:
			break; /* nothing to do */
		case HAYESCHANNEL_MODE_DATA:
			hayescommon_source_reset(&channel->rd_ppp_source);
			hayescommon_source_reset(&channel->wr_ppp_source);
			/* reset registration media */
			event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			free(channel->registration_media);
			channel->registration_media = NULL;
			event->registration.media = NULL;
			/* reset modem */
			_hayes_reset(hayes);
			break;
	}
	switch(mode)
	{
		case HAYESCHANNEL_MODE_INIT:
		case HAYESCHANNEL_MODE_COMMAND:
		case HAYESCHANNEL_MODE_PDU:
			break; /* nothing to do */
		case HAYESCHANNEL_MODE_DATA:
			/* report GPRS registration */
			event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			free(channel->registration_media);
			channel->registration_media = NULL;
			event->registration.media = "GPRS";
			hayes->helper->event(hayes->helper->modem, event);
			break;
	}
	channel->mode = mode;
}


/* conversions */
/* hayes_convert_gsm_to_iso */
static unsigned char _hayes_convert_gsm_to_iso(unsigned char c)
{
	size_t i;

	for(i = 0; i < sizeof(_hayes_gsm_iso) / sizeof(*_hayes_gsm_iso); i++)
		if(_hayes_gsm_iso[i].gsm == c)
			return _hayes_gsm_iso[i].iso;
	return c;
}


/* hayes_convert_gsm_string_to_iso */
static void _hayes_convert_gsm_string_to_iso(char * str)
{
	unsigned char * ustr = (unsigned char *)str;
	size_t i;

	for(i = 0; str[i] != '\0'; i++)
		ustr[i] = _hayes_convert_gsm_to_iso(ustr[i]);
}


/* hayes_convert_iso_to_gsm */
static unsigned char _hayes_convert_iso_to_gsm(unsigned char c)
{
	size_t i;

	for(i = 0; i < sizeof(_hayes_gsm_iso) / sizeof(*_hayes_gsm_iso); i++)
		if(_hayes_gsm_iso[i].iso == c)
			return _hayes_gsm_iso[i].gsm;
	return c;
}


/* hayes_convert_iso_string_to_gsm */
static void _hayes_convert_iso_string_to_gsm(char * str)
{
	unsigned char * ustr = (unsigned char *)str;
	size_t i;

	for(i = 0; str[i] != '\0'; i++)
		ustr[i] = _hayes_convert_iso_to_gsm(ustr[i]);
}


/* logging */
/* hayes_log */
static void _hayes_log(Hayes * hayes, HayesChannel * channel,
		char const * prefix, char const * buf, size_t cnt)
{
	ModemPluginHelper * helper = hayes->helper;

	if(channel->fp == NULL)
		return;
	if(fprintf(channel->fp, "\n%s", (prefix != NULL) ? prefix : "") == EOF
			|| fwrite(buf, sizeof(*buf), cnt, channel->fp) < cnt)
	{
		helper->error(NULL, strerror(errno), 1);
		fclose(channel->fp);
		channel->fp = NULL;
	}
}


/* messages */
/* hayes_message_to_pdu */
static char * _hayes_message_to_pdu(HayesChannel * channel, char const * number,
		ModemMessageEncoding encoding, size_t length,
		char const * content)
{
	unsigned int flags = 0;

	flags |= hayeschannel_has_quirks(channel, HAYES_QUIRK_WANT_SMSC_IN_PDU)
		? HAYESPDU_FLAG_WANT_SMSC : 0;
	return hayespdu_encode(number, encoding, length, content, flags);
}


/* parser */
/* hayes_parse */
static int _parse_do(Hayes * hayes, HayesChannel * channel);

static int _hayes_parse(Hayes * hayes, HayesChannel * channel)
{
	int ret = 0;
	size_t i;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() cnt=%zu\n", __func__, channel->rd_buf_cnt);
#endif
	for(i = 0; i < channel->rd_buf_cnt;)
	{
		if(channel->rd_buf[i] == '\r')
		{
			if(i + 1 < channel->rd_buf_cnt
					&& channel->rd_buf[i + 1] == '\n')
				channel->rd_buf[i++] = '\0';
		}
		else if(channel->rd_buf[i] != '\n')
		{
			i++;
			continue;
		}
		channel->rd_buf[i++] = '\0';
		if(channel->rd_buf[0] != '\0')
			ret |= _parse_do(hayes, channel);
		channel->rd_buf_cnt -= i;
		memmove(channel->rd_buf, &channel->rd_buf[i],
				channel->rd_buf_cnt);
		i = 0;
	}
	if((p = realloc(channel->rd_buf, channel->rd_buf_cnt)) != NULL)
		/* we can ignore errors... */
		channel->rd_buf = p;
	else if(channel->rd_buf_cnt == 0)
		/* ...except when it's not one */
		channel->rd_buf = NULL;
	return ret;
}

static int _parse_do(Hayes * hayes, HayesChannel * channel)
{
	char const * line = channel->rd_buf;
	HayesCommand * command = (channel->queue != NULL) ? channel->queue->data
		: NULL;
	HayesCommandStatus status;

	if(command == NULL || hayes_command_get_status(command) != HCS_ACTIVE)
		/* this was most likely unsollicited */
		return _hayes_parse_trigger(channel, line, NULL);
	_hayes_parse_trigger(channel, line, command);
	if(hayes_command_answer_append(command, line) != 0)
		return -1;
	if((status = hayes_command_get_status(command)) == HCS_ACTIVE)
		hayes_command_callback(command);
	/* unqueue if complete */
	if(hayes_command_is_complete(command))
	{
		hayeschannel_queue_pop(channel);
		_hayes_queue_push(hayes, channel);
	}
	return 0;
}


/* hayes_parse_pdu */
static int _parse_pdu_resume(Hayes * hayes, HayesChannel * channel);
static int _parse_pdu_send(Hayes * hayes, HayesChannel * channel);

static int _hayes_parse_pdu(Hayes * hayes, HayesChannel * channel)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() cnt=%zu\n", __func__, channel->rd_buf_cnt);
	for(i = 0; i < channel->rd_buf_cnt; i++)
		fprintf(stderr, " %02x", channel->rd_buf[i]);
	fputc('\n', stderr);
#endif
	for(i = 0; i < channel->rd_buf_cnt;)
	{
		if(channel->rd_buf[i] == '\r'
				|| channel->rd_buf[i] == '\n')
		{
			/* ignore carriage returns */
			i++;
			continue;
		}
		/* look for the PDU prompt */
		if(channel->rd_buf[i] != '>')
			return _parse_pdu_resume(hayes, channel);
		if(i + 1 >= channel->rd_buf_cnt)
			/* we need more data */
			break;
		if(channel->rd_buf[++i] != ' ')
			return _parse_pdu_resume(hayes, channel);
		_parse_pdu_send(hayes, channel);
		i++;
		break;
	}
	channel->rd_buf_cnt -= i;
	memmove(channel->rd_buf, &channel->rd_buf[i], channel->rd_buf_cnt);
	return 0;
}

static int _parse_pdu_resume(Hayes * hayes, HayesChannel * channel)
{
	char eop = 0x1a;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* XXX check for errors and report them */
	hayeschannel_queue_data(channel, &eop, sizeof(eop));
	if(channel->channel != NULL && channel->wr_source == 0)
		channel->wr_source = g_io_add_watch(channel->channel, G_IO_OUT,
				_on_watch_can_write, channel);
	_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_COMMAND);
	return 0;
}

static int _parse_pdu_send(Hayes * hayes, HayesChannel * channel)
{
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	char * pdu;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(command != NULL && hayes_command_get_status(command) == HCS_ACTIVE
			&& (pdu = hayes_command_get_data(command)) != NULL)
	{
		/* XXX check for errors and report them */
		hayeschannel_queue_data(channel, pdu, strlen(pdu));
		free(pdu);
		hayes_command_set_data(command, NULL);
	}
	return _parse_pdu_resume(hayes, channel);
}


/* hayes_parse_trigger */
static int _hayes_parse_trigger(HayesChannel * channel, char const * answer,
		HayesCommand * command)
{
	size_t i;
	const size_t count = sizeof(_hayes_code_handlers)
		/ sizeof(*_hayes_code_handlers);
	size_t len;
	HayesCodeHandler * hch;
	char const * p;
	int j;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", command)\n", __func__, answer);
#endif
	/* if the handler is obvious return directly */
	for(i = 0; i < count; i++)
	{
		hch = &_hayes_code_handlers[i];
		len = strlen(hch->code);
		if(strncmp(hch->code, answer, len) != 0)
			continue;
		if(answer[len] == ':')
		{
			if(answer[++len] == ' ') /* skip the optional space */
				len++;
		}
		else if(answer[len] != '\0')
			continue;
		hch->callback(channel, &answer[len]);
		return 0;
	}
	/* if the answer has no prefix choose it from the command issued */
	if(command == NULL
			|| (p = hayes_command_get_attention(command)) == NULL
			|| strncmp(p, "AT", 2) != 0)
		return 0;
	for(i = 0; i < count; i++)
	{
		hch = &_hayes_code_handlers[i];
		len = strlen(hch->code);
		if(strncmp(hch->code, &p[2], len) != 0
				|| isalnum((j = p[2 + len])))
			continue;
		hch->callback(channel, answer);
		return 0;
	}
	return 0;
}


/* queue */
/* hayes_queue_command */
static int _hayes_queue_command(Hayes * hayes, HayesChannel * channel,
		HayesCommand * command)
{
	GSList * queue;

	switch(channel->mode)
	{
		case HAYESCHANNEL_MODE_INIT:
			/* ignore commands besides initialization */
			if(hayes_command_get_priority(command)
					!= HCP_IMMEDIATE)
				return -1;
			/* fallthrough */
		case HAYESCHANNEL_MODE_COMMAND:
		case HAYESCHANNEL_MODE_DATA:
		case HAYESCHANNEL_MODE_PDU:
			if(hayes_command_set_status(command, HCS_QUEUED)
					!= HCS_QUEUED)
				return -1;
			queue = channel->queue;
			channel->queue = g_slist_append(channel->queue,
					command);
			if(queue == NULL)
				_hayes_queue_push(hayes, channel);
			break;
	}
	return 0;
}


#if 0 /* XXX no longer used */
/* hayes_queue_command_full */
static int _hayes_queue_command_full(Hayes * hayes,
		char const * attention, HayesCommandCallback callback)
{
	HayesCommand * command;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, attention);
#endif
	if((command = hayes_command_new(attention)) == NULL)
		return -hayes->helper->error(hayes->helper->modem,
				error_get(NULL), 1);
	hayes_command_set_callback(command, callback, hayes);
	if(_hayes_queue_command(hayes, command) != 0)
	{
		hayes_command_delete(command);
		return -1;
	}
	return 0;
}
#endif


/* hayes_queue_push */
static int _queue_push_do(Hayes * hayes, HayesChannel * channel);

static int _hayes_queue_push(Hayes * hayes, HayesChannel * channel)
{
	while(_queue_push_do(hayes, channel) != 0);
	return 0;
}

static int _queue_push_do(Hayes * hayes, HayesChannel * channel)
{
	HayesCommand * command;
	char const * prefix = "";
	char const * attention;
	const char suffix[] = "\r\n";
	size_t size;
	char * buf;
	guint timeout;

	if(channel->queue == NULL) /* nothing to send */
		return 0;
	command = channel->queue->data;
	if(channel->mode == HAYESCHANNEL_MODE_DATA)
#if 0 /* FIXME does not seem to work (see ATS2, ATS12) */
		prefix = "+++\r\n";
#else
		return 0; /* XXX keep commands in the queue in DATA mode */
#endif
	if(hayes_command_set_status(command, HCS_PENDING) != HCS_PENDING)
	{
		/* no longer push the command */
		hayeschannel_queue_pop(channel);
		return -1;
	}
	attention = hayes_command_get_attention(command);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() pushing \"%s\"\n", __func__, attention);
#endif
	size = strlen(prefix) + strlen(attention) + sizeof(suffix);
	if((buf = malloc(size)) == NULL
			|| snprintf(buf, size, "%s%s%s", prefix, attention,
				suffix) != (int)size - 1
			|| hayeschannel_queue_data(channel, buf, size - 1) != 0)
	{
		free(buf);
		hayes_command_set_status(command, HCS_ERROR);
		hayeschannel_queue_pop(channel);
		return -hayes->helper->error(hayes->helper->modem, strerror(
					errno), 1);
	}
	free(buf);
	if(channel->channel != NULL && channel->wr_source == 0)
		channel->wr_source = g_io_add_watch(channel->channel, G_IO_OUT,
				_on_watch_can_write, channel);
	hayescommon_source_reset(&channel->timeout);
	if((timeout = hayes_command_get_timeout(command)) != 0)
		channel->timeout = g_timeout_add(timeout, _on_channel_timeout,
				channel);
	return 0;
}


/* hayes_request_channel */
static char * _request_attention(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void ** data);
static char * _request_attention_apn(char const * protocol, char const * apn);
static char * _request_attention_call(HayesChannel * channel,
		ModemRequest * request);
static char * _request_attention_call_data(HayesChannel * channel,
		ModemRequest * request);
static char * _request_attention_call_ussd(ModemRequest * request);
static char * _request_attention_call_hangup(Hayes * hayes,
		HayesChannel * channel);
static char * _request_attention_connectivity(Hayes * hayes,
		HayesChannel * channel, unsigned int enabled);
static char * _request_attention_contact_delete(HayesChannel * channel,
		unsigned int id);
static char * _request_attention_contact_edit(unsigned int id,
		char const * name, char const * number);
static char * _request_attention_contact_list(ModemRequest * request);
static char * _request_attention_contact_new(char const * name,
		char const * number);
static char * _request_attention_dtmf_send(ModemRequest * request);
static char * _request_attention_gprs(Hayes * hayes, HayesChannel * channel,
		char const * username, char const * password);
static char * _request_attention_message(unsigned int id);
static char * _request_attention_message_delete(HayesChannel * channel,
		unsigned int id);
static char * _request_attention_message_list(Hayes * hayes,
		HayesChannel * channel);
static char * _request_attention_message_send(Hayes * hayes,
		HayesChannel * channel, char const * number,
		ModemMessageEncoding encoding, size_t length,
		char const * content, void ** data);
static char * _request_attention_password_set(Hayes * hayes, char const * name,
		char const * oldpassword, char const * newpassword);
static char * _request_attention_registration(Hayes * hayes,
		HayesChannel * channel, ModemRegistrationMode mode,
		char const * _operator);
static char * _request_attention_sim_pin(Hayes * hayes, HayesChannel * channel,
		char const * password);
static char * _request_attention_sim_puk(Hayes * hayes, HayesChannel * channel,
		char const * password);
static char * _request_attention_unsupported(Hayes * hayes,
		ModemRequest * request);
static int _request_channel_handler(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void * data,
		HayesRequestHandler * handler);

static int _hayes_request_channel(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void * data)
{
	unsigned int type = request->type;
	size_t i;
	const size_t count = sizeof(_hayes_request_handlers)
		/ sizeof(*_hayes_request_handlers);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__,
			(request != NULL) ? request->type : (unsigned)-1);
#endif
	if(request == NULL)
		return -1;
	if(hayeschannel_has_quirks(channel, HAYES_QUIRK_CONNECTED_LINE_DISABLED)
			&& type == HAYES_REQUEST_CONNECTED_LINE_ENABLE)
		request->type = HAYES_REQUEST_CONNECTED_LINE_DISABLE;
	for(i = 0; i < count; i++)
		if(_hayes_request_handlers[i].type == request->type)
			break;
	if(i == count)
#ifdef DEBUG
		return -hayes->helper->error(hayes->helper->modem,
				"Unable to handle request", 1);
#else
		return -hayes->helper->error(NULL, "Unable to handle request",
				1);
#endif
	return _request_channel_handler(hayes, channel, request, data,
			&_hayes_request_handlers[i]);
}

static int _request_channel_handler(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void * data,
		HayesRequestHandler * handler)
{
	HayesCommand * command;
	char const * attention;
	char * p = NULL;

	if((attention = handler->attention) == NULL)
	{
		if((p = _request_attention(hayes, channel, request, &data))
				== NULL)
			return 0; /* XXX errors should not be ignored */
		attention = p;
	}
	/* XXX using _hayes_queue_command_full() was more elegant */
	command = hayes_command_new(attention);
	free(p);
	if(command == NULL)
		return -1;
	hayes_command_set_callback(command, handler->callback, channel);
	if(_hayes_queue_command(hayes, channel, command) != 0)
	{
		hayes_command_delete(command);
		return -1;
	}
	if(data != NULL)
		hayes_command_set_data(command, data);
	return 0;
}

static char * _request_attention(Hayes * hayes, HayesChannel * channel,
		ModemRequest * request, void ** data)
{
	unsigned int type = request->type;
	char buf[32];

	switch(type)
	{
		case HAYES_REQUEST_CONTACT_LIST:
			return _request_attention_contact_list(request);
		case MODEM_REQUEST_AUTHENTICATE:
			if(strcmp(request->authenticate.name, "APN") == 0)
				return _request_attention_apn(
						request->authenticate.username,
						request->authenticate.password);
			if(strcmp(request->authenticate.name, "GPRS") == 0)
				return _request_attention_gprs(hayes, channel,
						request->authenticate.username,
						request->authenticate.password);
			if(strcmp(request->authenticate.name, "SIM PIN") == 0)
				return _request_attention_sim_pin(hayes,
						channel,
						request->authenticate.password);
			if(strcmp(request->authenticate.name, "SIM PUK") == 0)
				return _request_attention_sim_puk(hayes,
						channel,
						request->authenticate.password);
			break;
		case MODEM_REQUEST_CALL:
			if(request->call.call_type == MODEM_CALL_TYPE_VOICE
					&& _is_ussd_code(request->call.number))
				return _request_attention_call_ussd(request);
			else if(request->call.call_type == MODEM_CALL_TYPE_DATA)
				return _request_attention_call_data(channel,
						request);
			else
				return _request_attention_call(channel, request);
		case MODEM_REQUEST_CALL_HANGUP:
			return _request_attention_call_hangup(hayes, channel);
		case MODEM_REQUEST_CALL_PRESENTATION:
			snprintf(buf, sizeof(buf), "%s%u", "AT+CLIP=",
					request->call_presentation.enabled
					? 1 : 0);
			return strdup(buf);
		case MODEM_REQUEST_CONNECTIVITY:
			return _request_attention_connectivity(hayes, channel,
					request->connectivity.enabled);
		case MODEM_REQUEST_CONTACT_DELETE:
			return _request_attention_contact_delete(channel,
					request->contact_delete.id);
		case MODEM_REQUEST_CONTACT_EDIT:
			return _request_attention_contact_edit(
					request->contact_edit.id,
					request->contact_edit.name,
					request->contact_edit.number);
		case MODEM_REQUEST_CONTACT_NEW:
			return _request_attention_contact_new(
					request->contact_new.name,
					request->contact_new.number);
		case MODEM_REQUEST_DTMF_SEND:
			return _request_attention_dtmf_send(request);
		case MODEM_REQUEST_MESSAGE:
			return _request_attention_message(request->message.id);
		case MODEM_REQUEST_MESSAGE_LIST:
			return _request_attention_message_list(hayes, channel);
		case MODEM_REQUEST_MESSAGE_DELETE:
			return _request_attention_message_delete(channel,
					request->message_delete.id);
		case MODEM_REQUEST_MESSAGE_SEND:
			return _request_attention_message_send(hayes, channel,
					request->message_send.number,
					request->message_send.encoding,
					request->message_send.length,
					request->message_send.content, data);
		case MODEM_REQUEST_PASSWORD_SET:
			return _request_attention_password_set(hayes,
					request->password_set.name,
					request->password_set.oldpassword,
					request->password_set.newpassword);
		case MODEM_REQUEST_REGISTRATION:
			return _request_attention_registration(hayes, channel,
					request->registration.mode,
					request->registration._operator);
		case MODEM_REQUEST_UNSUPPORTED:
			return _request_attention_unsupported(hayes, request);
		default:
			break;
	}
	return NULL;
}

static char * _request_attention_apn(char const * protocol, char const * apn)
{
	char * ret;
	const char cmd[] = "AT+CGDCONT=1,";
	size_t len;

	if(protocol == NULL || apn == NULL)
		return NULL;
	len = sizeof(cmd) + strlen(protocol) + 2 + strlen(apn) + 3;
	if((ret = malloc(len)) == NULL)
		return NULL;
	snprintf(ret, len, "%s\"%s\",\"%s\"", cmd, protocol, apn);
	return ret;
}

static char * _request_attention_call(HayesChannel * channel,
		ModemRequest * request)
{
	char * ret;
	char const * number = request->call.number;
	ModemEvent * event;
	const char cmd[] = "ATD";
	const char anonymous[] = "I";
	const char voice[] = ";";
	size_t len;

	if(request->call.number == NULL)
		request->call.number = "";
	if(request->call.number[0] == '\0')
		number = "L";
	else if(!hayescommon_number_is_valid(request->call.number))
		return NULL;
	event = &channel->events[MODEM_EVENT_TYPE_CALL];
	/* XXX should really be set at the time of the call */
	event->call.call_type = request->call.call_type;
	free(channel->call_number);
	if(request->call.call_type == MODEM_CALL_TYPE_DATA)
		channel->call_number = NULL;
	else if((channel->call_number = strdup(request->call.number)) == NULL)
		return NULL;
	event->call.number = channel->call_number;
	len = sizeof(cmd) + strlen(number) + sizeof(anonymous) + sizeof(voice);
	if((ret = malloc(len)) == NULL)
		return NULL;
	snprintf(ret, len, "%s%s%s%s", cmd, number,
			(request->call.anonymous) ? anonymous : "",
			(request->call.call_type == MODEM_CALL_TYPE_VOICE)
			? voice : "");
	return ret;
}

static char * _request_attention_call_data(HayesChannel * channel,
		ModemRequest * request)
{
	if(request->call.number == NULL)
		return strdup("AT+CGDATA=\"PPP\"");
	return _request_attention_call(channel, request);
}

static char * _request_attention_call_ussd(ModemRequest * request)
{
	char * ret;
	char const * number = request->call.number;
	const char cmd[] = "AT+CUSD=1,";
	size_t len;

	if(request->call.number == NULL || request->call.number[0] == '\0')
		return NULL;
	len = sizeof(cmd) + strlen(number) + 2;
	if((ret = malloc(len)) == NULL)
		return NULL;
	/* XXX may also require setting dcs */
	snprintf(ret, len, "%s\"%s\"", cmd, number);
	return ret;
}

static char * _request_attention_call_hangup(Hayes * hayes,
		HayesChannel * channel)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];

	/* FIXME check that this works on all phones, including:
	 * - while calling:
	 *   . still ringing => simply inject "\r\n"?
	 *   . in the queue => simply remove?
	 * - while ringing (incoming) */
	if(channel->mode == HAYESCHANNEL_MODE_DATA)
	{
		event->connection.connected = 0;
		event->connection.in = 0;
		event->connection.out = 0;
		hayes->helper->event(hayes->helper->modem, event);
		_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_INIT);
		return NULL;
	}
	/* return "ATH" if currently ringing */
	event = &channel->events[MODEM_EVENT_TYPE_CALL];
	if(event->call.direction == MODEM_CALL_DIRECTION_INCOMING
			&& event->call.status == MODEM_CALL_STATUS_RINGING)
		return strdup("ATH");
	/* force all calls to terminate */
	return strdup("AT+CHUP");
}

static char * _request_attention_connectivity(Hayes * hayes,
		HayesChannel * channel, unsigned int enabled)
{
	_hayes_request_type(hayes, channel, enabled
			? HAYES_REQUEST_FUNCTIONAL_ENABLE
			: HAYES_REQUEST_FUNCTIONAL_DISABLE);
	return NULL;
}

static char * _request_attention_contact_delete(HayesChannel * channel,
		unsigned int id)
{
	char const cmd[] = "AT+CPBW=";
	char buf[32];

	/* FIXME store in the command itself */
	channel->events[MODEM_EVENT_TYPE_CONTACT_DELETED].contact_deleted.id
		= id;
	snprintf(buf, sizeof(buf), "%s%u%s", cmd, id, ",");
	return strdup(buf);
}

static char * _request_attention_contact_edit(unsigned int id,
		char const * name, char const * number)
{
	char const cmd[] = "AT+CPBW=";
	char buf[128];
	char * p;

	if(!hayescommon_number_is_valid(number)
			|| name == NULL || strlen(name) == 0)
		/* XXX report error */
		return NULL;
	if((p = g_convert(name, -1, "ISO-8859-1", "UTF-8", NULL, NULL, NULL))
			!= NULL)
	{
		_hayes_convert_iso_string_to_gsm(p);
		name = p;
	}
	if(snprintf(buf, sizeof(buf), "%s%u%s\"%s\"%s%u%s\"%s\"", cmd, id, ",",
				(number[0] == '+') ? &number[1] : number, ",",
				(number[0] == '+') ? 145 : 129, ",", name)
			>= (int)sizeof(buf))
	{
		g_free(p);
		/* XXX report error */
		return NULL;
	}
	g_free(p);
	return strdup(buf);
}

static char * _request_attention_contact_list(ModemRequest * request)
{
	HayesRequestContactList * list = request->plugin.data;
	const char cmd[] = "AT+CPBR=";
	char buf[32];

	if(list->to < list->from)
		list->to = list->from;
	snprintf(buf, sizeof(buf), "%s%u,%u", cmd, list->from, list->to);
	return strdup(buf);
}

static char * _request_attention_contact_new(char const * name,
		char const * number)
{
	char const cmd[] = "AT+CPBW=";
	char buf[128];
	char * p;

	if(!hayescommon_number_is_valid(number)
			|| name == NULL || strlen(name) == 0)
		/* XXX report error */
		return NULL;
	if((p = g_convert(name, -1, "ISO-8859-1", "UTF-8", NULL, NULL, NULL))
			!= NULL)
	{
		_hayes_convert_iso_string_to_gsm(p);
		name = p;
	}
	if(snprintf(buf, sizeof(buf), "%s%s\"%s\"%s%u%s\"%s\"", cmd, ",",
				(number[0] == '+') ? &number[1] : number, ",",
				(number[0] == '+') ? 145 : 129, ",", name)
			>= (int)sizeof(buf))
	{
		g_free(p);
		/* XXX report error */
		return NULL;
	}
	g_free(p);
	return strdup(buf);
}

static char * _request_attention_dtmf_send(ModemRequest * request)
{
	const char cmd[] = "AT+VTS=";
	char buf[32];
	unsigned int dtmf = request->dtmf_send.dtmf;

	if((dtmf < '0' || dtmf > '9') && (dtmf < 'A' || dtmf > 'D')
			&& dtmf != '*' && dtmf != '#')
		return NULL;
	snprintf(buf, sizeof(buf), "%s%c", cmd, dtmf);
	return strdup(buf);
}

static char * _request_attention_gprs(Hayes * hayes, HayesChannel * channel,
		char const * username, char const * password)
{
	free(channel->gprs_username);
	channel->gprs_username = (username != NULL) ? strdup(username) : NULL;
	free(channel->gprs_password);
	channel->gprs_password = (password != NULL) ? strdup(password) : NULL;
	/* check for errors */
	if((username != NULL && channel->gprs_username == NULL)
			|| (password != NULL && channel->gprs_password == NULL))
		hayes->helper->error(NULL, strerror(errno), 1);
	return NULL; /* we don't need to issue any command */
}

static char * _request_attention_message(unsigned int id)
{
	char const cmd[] = "AT+CMGR=";
	char buf[32];

	/* FIXME force the message format to be in PDU mode? */
	snprintf(buf, sizeof(buf), "%s%u", cmd, id);
	return strdup(buf);
}

static char * _request_attention_message_delete(HayesChannel * channel,
		unsigned int id)
{
	char const cmd[] = "AT+CMGD=";
	char buf[32];

	/* FIXME store in the command itself */
	channel->events[MODEM_EVENT_TYPE_MESSAGE_DELETED].message_deleted.id
		= id;
	snprintf(buf, sizeof(buf), "%s%u", cmd, id);
	return strdup(buf);
}

static char * _request_attention_message_list(Hayes * hayes,
		HayesChannel * channel)
{
	ModemRequest request;
	HayesRequestMessageData * data;

	memset(&request, 0, sizeof(request));
	/* request received unread messages */
	request.type = HAYES_REQUEST_MESSAGE_LIST_INBOX_UNREAD;
	if((data = malloc(sizeof(*data))) != NULL)
	{
		data->id = 0;
		data->folder = MODEM_MESSAGE_FOLDER_INBOX;
		data->status = MODEM_MESSAGE_STATUS_UNREAD;
	}
	if(_hayes_request_channel(hayes, channel, &request, data) != 0)
		free(data);
	/* request received read messages */
	request.type = HAYES_REQUEST_MESSAGE_LIST_INBOX_READ;
	if((data = malloc(sizeof(*data))) != NULL)
	{
		data->id = 0;
		data->folder = MODEM_MESSAGE_FOLDER_INBOX;
		data->status = MODEM_MESSAGE_STATUS_READ;
	}
	if(_hayes_request_channel(hayes, channel, &request, data) != 0)
		free(data);
	/* request sent unread messages */
	request.type = HAYES_REQUEST_MESSAGE_LIST_SENT_UNREAD;
	if((data = malloc(sizeof(*data))) != NULL)
	{
		data->id = 0;
		data->folder = MODEM_MESSAGE_FOLDER_OUTBOX;
		data->status = MODEM_MESSAGE_STATUS_UNREAD;
	}
	if(_hayes_request_channel(hayes, channel, &request, data) != 0)
		free(data);
	/* request sent read messages */
	request.type = HAYES_REQUEST_MESSAGE_LIST_SENT_READ;
	if((data = malloc(sizeof(*data))) != NULL)
	{
		data->id = 0;
		data->folder = MODEM_MESSAGE_FOLDER_OUTBOX;
		data->status = MODEM_MESSAGE_STATUS_READ;
	}
	if(_hayes_request_channel(hayes, channel, &request, data) != 0)
		free(data);
	return NULL;
}

static char * _request_attention_message_send(Hayes * hayes,
		HayesChannel * channel, char const * number,
		ModemMessageEncoding encoding, size_t length,
		char const * content, void ** data)
{
	char * ret;
	char const cmd[] = "AT+CMGS=";
	char * pdu;
	size_t pdulen;
	size_t len;

	if(_hayes_request_type(hayes, channel, HAYES_REQUEST_MESSAGE_FORMAT_PDU)
			!= 0)
		return NULL;
	if((pdu = _hayes_message_to_pdu(channel, number, encoding, length,
					content)) == NULL)
		return NULL;
	len = sizeof(cmd) + 11;
	if((ret = malloc(len)) == NULL)
	{
		free(pdu);
		return NULL;
	}
	pdulen = strlen(pdu);
	if(hayeschannel_has_quirks(channel, HAYES_QUIRK_WANT_SMSC_IN_PDU))
		pdulen -= 2;
	snprintf(ret, len, "%s%zu", cmd, pdulen / 2);
	*data = pdu;
	return ret;
}

static char * _request_attention_password_set(Hayes * hayes, char const * name,
		char const * oldpassword, char const * newpassword)
{
	char * ret;
	size_t len;
	char const cpwd[] = "AT+CPWD=";
	char const * n;

	if(name == NULL || oldpassword == NULL || newpassword == NULL)
		return NULL;
	if(strcmp(name, "SIM PIN") == 0)
		n = "SC";
	else
		return NULL;
	len = sizeof(cpwd) + strlen(n) + 2 + strlen(oldpassword) + 3
		+ strlen(newpassword) + 3;
	if((ret = malloc(len)) == NULL)
	{
		hayes->helper->error(NULL, strerror(errno), 1);
		return NULL;
	}
	snprintf(ret, len, "%s\"%s\",\"%s\",\"%s\"", cpwd, n, oldpassword,
			newpassword);
	return ret;
}

static char * _request_attention_registration(Hayes * hayes,
		HayesChannel * channel, ModemRegistrationMode mode,
		char const * _operator)
{
	char const cops[] = "AT+COPS=";
	size_t len = sizeof(cops) + 5;
	char * p;

	switch(mode)
	{
		case MODEM_REGISTRATION_MODE_AUTOMATIC:
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_REGISTRATION_AUTOMATIC);
			break;
		case MODEM_REGISTRATION_MODE_DISABLED:
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_REGISTRATION_DISABLED);
			break;
		case MODEM_REGISTRATION_MODE_MANUAL:
			if(_operator == NULL)
				return NULL;
			len += strlen(_operator);
			if((p = malloc(len)) == NULL)
				return NULL;
			snprintf(p, len, "%s=1,0,%s", cops, _operator);
			return p;
		case MODEM_REGISTRATION_MODE_UNKNOWN:
			break;
	}
	return NULL;
}

static char * _request_attention_sim_pin(Hayes * hayes, HayesChannel * channel,
		char const * password)
{
	char * ret;
	const char cmd[] = "AT+CPIN=";
	size_t len;
	char const * format;

	if(password == NULL)
		return NULL;
	len = sizeof(cmd) + strlen(password) + 2;
	if((ret = malloc(len)) == NULL)
	{
		hayes->helper->error(NULL, strerror(errno), 1);
		return NULL;
	}
	format = hayeschannel_has_quirks(channel, HAYES_QUIRK_CPIN_NO_QUOTES)
		? "%s%s" : "%s\"%s\"";
	snprintf(ret, len, format, cmd, password);
	return ret;
}

static char * _request_attention_sim_puk(Hayes * hayes, HayesChannel * channel,
		char const * password)
{
	char * ret;
	const char cmd[] = "AT+CPIN=";
	size_t len;
	char const * format;

	if(password == NULL)
		return NULL;
	len = sizeof(cmd) + strlen(password) + 3;
	if((ret = malloc(len)) == NULL)
	{
		hayes->helper->error(NULL, strerror(errno), 1);
		return NULL;
	}
	format = hayeschannel_has_quirks(channel, HAYES_QUIRK_CPIN_NO_QUOTES)
		? "%s%s," : "%s\"%s\",";
	snprintf(ret, len, format, cmd, password);
	return ret;
}

static char * _request_attention_unsupported(Hayes * hayes,
		ModemRequest * request)
{
	HayesRequest * hrequest = request->unsupported.request;
	(void) hayes;

	if(strcmp(request->unsupported.modem, plugin.name) != 0)
		return NULL;
	if(request->unsupported.size != sizeof(*hrequest))
		return NULL;
	switch(request->unsupported.request_type)
	{
		case HAYES_REQUEST_COMMAND_QUEUE:
			return strdup(hrequest->command_queue.command);
		default:
			return NULL;
	}
}


/* hayes_request_type */
static int _hayes_request_type(Hayes * hayes, HayesChannel * channel,
		ModemRequestType type)
{
	ModemRequest request;

	memset(&request, 0, sizeof(request));
	request.type = type;
	return _hayes_request_channel(hayes, channel, &request, NULL);
}


/* hayes_reset */
static void _hayes_reset(Hayes * hayes)
{
	_hayes_stop(hayes);
	_hayes_start(hayes, hayes->retry);
}


/* callbacks */
/* on_channel_authenticate */
static gboolean _on_channel_authenticate(gpointer data)
{
	const guint timeout = 1000;
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_AUTHENTICATION];

	if(channel->authenticate_count++ < 10)
	{
		channel->authenticate_source = g_timeout_add(timeout,
				_on_channel_authenticate, channel);
		/* FIXME this must stop the "checking for SIM PIN" dialog */
		_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
	}
	else
	{
		channel->authenticate_count = 0;
		channel->authenticate_source = 0;
		event->authentication.status
			= MODEM_AUTHENTICATION_STATUS_ERROR;
		hayes->helper->event(hayes->helper->modem, event);
	}
	return FALSE;
}


/* on_channel_reset */
static int _reset_open(Hayes * hayes);
static int _reset_configure(Hayes * hayes, char const * device, int fd);
static unsigned int _reset_configure_baudrate(Hayes * hayes,
		unsigned int baudrate);

static gboolean _on_channel_reset(gpointer data)
{
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_STATUS];
	GError * error = NULL;
	int fd;
	char const * logfile;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_hayes_stop(hayes);
	if((fd = _reset_open(hayes)) < 0)
	{
		if(event->status.status != MODEM_STATUS_UNAVAILABLE)
		{
			event->status.status = MODEM_STATUS_UNAVAILABLE;
			hayes->helper->event(hayes->helper->modem, event);
		}
		hayes->helper->error(NULL, error_get(NULL), 1);
		if(hayes->retry > 0)
			channel->source = g_timeout_add(hayes->retry,
					_on_channel_reset, channel);
		return FALSE;
	}
	event->status.status = MODEM_STATUS_UNKNOWN;
	/* logging */
	logfile = helper->config_get(helper->modem, "logfile");
	if(logfile != NULL)
	{
		if((channel->fp = fopen(logfile, "w")) == NULL)
			hayes->helper->error(NULL, strerror(errno), 1);
		else
			setvbuf(channel->fp, NULL, _IONBF, BUFSIZ);
	}
	channel->channel = g_io_channel_unix_new(fd);
	if(g_io_channel_set_encoding(channel->channel, NULL, &error)
			!= G_IO_STATUS_NORMAL)
	{
		hayes->helper->error(hayes->helper->modem, error->message, 1);
		g_error_free(error);
	}
	g_io_channel_set_buffered(channel->channel, FALSE);
	channel->rd_source = g_io_add_watch(channel->channel, G_IO_IN,
			_on_watch_can_read, channel);
	channel->source = g_idle_add(_on_reset_settle, channel);
	return FALSE;
}

static int _reset_open(Hayes * hayes)
{
	ModemPluginHelper * helper = hayes->helper;
	char const * device;
	int fd;

	if((device = helper->config_get(helper->modem, "device")) == NULL)
		device = "/dev/modem";
	if((fd = open(device, O_RDWR | O_NONBLOCK)) < 0)
		return -error_set_code(1, "%s: %s", device, strerror(errno));
	if(_reset_configure(hayes, device, fd) != 0)
	{
		close(fd);
		return -1;
	}
	return fd;
}

static int _reset_configure(Hayes * hayes, char const * device, int fd)
{
	ModemPluginHelper * helper = hayes->helper;
	unsigned int baudrate;
	unsigned int hwflow;
	struct stat st;
	int fl;
	struct termios term;
	char const * p;

	/* baud rate */
	if((p = helper->config_get(helper->modem, "baudrate")) == NULL
			|| (baudrate = strtoul(p, NULL, 10)) == 0)
		baudrate = 115200;
	baudrate = _reset_configure_baudrate(hayes, baudrate);
	/* hardware flow */
	if((p = helper->config_get(helper->modem, "hwflow")) == NULL
			|| (hwflow = strtoul(p, NULL, 10)) != 0)
		hwflow = 1;
	/* lock the port for exclusive access */
	if(flock(fd, LOCK_EX | LOCK_NB) != 0)
		return 1;
	/* set the port as blocking */
	fl = fcntl(fd, F_GETFL, 0);
	if((fl & ~O_NONBLOCK) != fl
			&& fcntl(fd, F_SETFL, fl & ~O_NONBLOCK) == -1)
		return 1;
	if(fstat(fd, &st) != 0)
		return 1;
	if(st.st_mode & S_IFCHR) /* character special */
	{
		if(tcgetattr(fd, &term) != 0)
			return 1;
		term.c_cflag &= ~(CSIZE | PARENB);
		term.c_cflag |= CS8;
		term.c_cflag |= CREAD;
		term.c_cflag |= CLOCAL;
		if(hwflow != 0)
			term.c_cflag |= CRTSCTS;
		else
			term.c_cflag &= ~CRTSCTS;
		term.c_iflag = (IGNPAR | IGNBRK);
		term.c_lflag = 0;
		term.c_oflag = 0;
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		if(cfsetispeed(&term, 0) != 0) /* same speed as output speed */
			error_set("%s", device); /* go on anyway */
		if(cfsetospeed(&term, baudrate) != 0)
			error_set("%s", device); /* go on anyway */
		if(tcsetattr(fd, TCSAFLUSH, &term) != 0)
			return 1;
	}
	return 0;
}

static unsigned int _reset_configure_baudrate(Hayes * hayes,
		unsigned int baudrate)
{
	switch(baudrate)
	{
		case 1200:
			return B1200;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
#ifdef B76800
		case 76800:
			return B76800;
#endif
#ifdef B14400
		case 14400:
			return B14400;
#endif
#ifdef B28800
		case 28800:
			return B28800;
#endif
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
#ifdef B460800
		case 460800:
			return B460800;
#endif
#ifdef B921600
		case 921600:
			return B921600;
#endif
		default:
			error_set("%u%s%u%s", baudrate,
					": Unsupported baudrate (using ",
					115200, ")");
			hayes->helper->error(NULL, error_get(NULL), 1);
			return B115200;
	}
}


/* on_channel_timeout */
static gboolean _on_channel_timeout(gpointer data)
{
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	HayesCommand * command;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	channel->timeout = 0;
	if(channel->queue == NULL || (command = channel->queue->data) == NULL)
		return FALSE;
	hayes_command_set_status(command, HCS_TIMEOUT);
	hayeschannel_queue_pop(channel);
	_hayes_queue_push(hayes, channel);
	return FALSE;
}


/* on_queue_timeout */
static gboolean _on_queue_timeout(gpointer data)
{
	const guint timeout = 1000;
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	HayesCommand * command;

	channel->source = 0;
	if(channel->queue_timeout == NULL) /* nothing to send */
		return FALSE;
	command = channel->queue_timeout->data;
	_hayes_queue_command(hayes, channel, command);
	channel->queue_timeout = g_slist_remove(channel->queue_timeout,
			command);
	if(channel->queue_timeout != NULL)
		channel->source = g_timeout_add(timeout, _on_queue_timeout,
				channel);
	else
		/* XXX check the registration again to be safe */
		_hayes_request_type(hayes, channel, HAYES_REQUEST_REGISTRATION);
	return FALSE;
}


/* on_reset_settle */
static void _reset_settle_command(HayesChannel * channel, char const * string);
static HayesCommandStatus _on_reset_settle_callback(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel);

static gboolean _on_reset_settle(gpointer data)
{
	HayesChannel * channel = data;

	channel->source = 0;
	_reset_settle_command(channel, "ATZE0V1");
	return FALSE;
}

static gboolean _on_reset_settle2(gpointer data)
{
	HayesChannel * channel = data;

	channel->source = 0;
	/* try an alternative initialization string */
	_reset_settle_command(channel, "ATE0V1");
	return FALSE;
}

static void _reset_settle_command(HayesChannel * channel, char const * string)
{
	const unsigned int timeout = 500;
	Hayes * hayes = channel->hayes;
	HayesCommand * command;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, string);
#endif
	if((command = hayes_command_new(string)) == NULL)
	{
		hayes->helper->error(hayes->helper->modem, error_get(NULL), 1);
		return;
	}
	hayes_command_set_callback(command, _on_reset_settle_callback, channel);
	hayes_command_set_priority(command, HCP_IMMEDIATE);
	hayes_command_set_timeout(command, timeout);
	if(_hayes_queue_command(hayes, channel, command) != 0)
	{
		hayes->helper->error(hayes->helper->modem, error_get(NULL), 1);
		hayes_command_delete(command);
	}
}

static HayesCommandStatus _on_reset_settle_callback(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%s (%u))\n", __func__,
			_hayes_command_status[status], status);
#endif
	status = _on_request_generic(command, status, channel);
	switch(status)
	{
		case HCS_UNKNOWN: /* ignore */
		case HCS_QUEUED:
		case HCS_PENDING:
			break;
		case HCS_ACTIVE: /* give it another chance */
			break;
		case HCS_SUCCESS: /* we can initialize */
			_hayes_set_mode(hayes, channel,
					HAYESCHANNEL_MODE_COMMAND);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_LOCAL_ECHO_DISABLE);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_VERBOSE_ENABLE);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_VENDOR);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_MODEL);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_EXTENDED_ERRORS);
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_FUNCTIONAL);
			break;
		case HCS_TIMEOUT: /* try again */
		case HCS_ERROR:
			if(channel->source != 0)
				g_source_remove(channel->source);
			channel->source = g_timeout_add(hayes->retry,
					_on_reset_settle2, channel);
			break;
	}
	return status;
}


/* on_watch_can_read */
static gboolean _on_watch_can_read(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	const int inc = 256;
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

	if(condition != G_IO_IN || source != channel->channel)
		return FALSE; /* should not happen */
	if((p = realloc(channel->rd_buf, channel->rd_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	channel->rd_buf = p;
	status = g_io_channel_read_chars(source,
			&channel->rd_buf[channel->rd_buf_cnt], inc, &cnt,
			&error);
	_hayes_log(hayes, channel, "MODEM: ",
			&channel->rd_buf[channel->rd_buf_cnt], cnt);
	channel->rd_buf_cnt += cnt;
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->modem, error->message, 1);
			g_error_free(error);
			/* fallthrough */
		case G_IO_STATUS_EOF:
		default: /* should not happen... */
			channel->rd_source = 0;
			if(hayes->retry > 0)
				_hayes_reset(hayes);
			return FALSE;
	}
	switch(channel->mode)
	{
		case HAYESCHANNEL_MODE_INIT:
		case HAYESCHANNEL_MODE_COMMAND:
			_hayes_parse(hayes, channel);
			break;
		case HAYESCHANNEL_MODE_PDU:
			_hayes_parse_pdu(hayes, channel);
			break;
		case HAYESCHANNEL_MODE_DATA:
			if(channel->wr_ppp_channel == NULL
					|| channel->wr_ppp_source != 0)
				break;
			channel->wr_ppp_source = g_io_add_watch(
					channel->wr_ppp_channel, G_IO_OUT,
					_on_watch_can_write_ppp, channel);
			break;
	}
	return TRUE;
}


/* on_watch_can_read_ppp */
static gboolean _on_watch_can_read_ppp(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];
	const int inc = 256;
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

	if(condition != G_IO_IN || source != channel->rd_ppp_channel)
		return FALSE; /* should not happen */
	if((p = realloc(channel->wr_buf, channel->wr_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	channel->wr_buf = p;
	status = g_io_channel_read_chars(source,
			&channel->wr_buf[channel->wr_buf_cnt], inc, &cnt,
			&error);
	channel->wr_buf_cnt += cnt;
	event->connection.out += cnt;
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->modem, error->message, 1);
			g_error_free(error);
			/* fallthrough */
		case G_IO_STATUS_EOF:
		default:
			channel->rd_ppp_source = 0;
			event->connection.connected = 0;
			helper->event(helper->modem, event);
			_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_INIT);
			return FALSE;
	}
	if(channel->channel != NULL && channel->wr_source == 0)
		channel->wr_source = g_io_add_watch(channel->channel, G_IO_OUT,
				_on_watch_can_write, channel);
	return TRUE;
}


/* on_watch_can_write */
static gboolean _on_watch_can_write(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	HayesChannel * channel = data;
	HayesCommand * command = (channel->queue) != NULL
		? channel->queue->data : NULL;
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

	if(condition != G_IO_OUT || source != channel->channel)
		return FALSE; /* should not happen */
	status = g_io_channel_write_chars(source, channel->wr_buf,
			channel->wr_buf_cnt, &cnt, &error);
	_hayes_log(hayes, channel, "PHONE: ", channel->wr_buf, cnt);
	if(cnt != 0) /* some data may have been written anyway */
	{
		channel->wr_buf_cnt -= cnt;
		memmove(channel->wr_buf, &channel->wr_buf[cnt],
				channel->wr_buf_cnt);
		if((p = realloc(channel->wr_buf, channel->wr_buf_cnt)) != NULL)
			/* we can ignore errors... */
			channel->wr_buf = p;
		else if(channel->wr_buf_cnt == 0)
			/* ...except when it's not one */
			channel->wr_buf = NULL;
	}
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->modem, error->message, 1);
			g_error_free(error);
			/* fallthrough */
		case G_IO_STATUS_EOF:
		default: /* should not happen */
			channel->wr_source = 0;
			if(hayes->retry > 0)
				_hayes_reset(hayes);
			return FALSE;
	}
	if(channel->wr_buf_cnt > 0) /* there is more data to write */
		return TRUE;
	channel->wr_source = 0;
	if(command != NULL)
		hayes_command_set_status(command, HCS_ACTIVE);
	return FALSE;
}


/* on_watch_can_write_ppp */
static gboolean _on_watch_can_write_ppp(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	HayesChannel * channel = data;
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

	if(condition != G_IO_OUT || source != channel->wr_ppp_channel)
		return FALSE; /* should not happen */
	status = g_io_channel_write_chars(source, channel->rd_buf,
			channel->rd_buf_cnt, &cnt, &error);
	event->connection.in += cnt;
	if(cnt != 0) /* some data may have been written anyway */
	{
		channel->rd_buf_cnt -= cnt;
		memmove(channel->rd_buf, &channel->rd_buf[cnt],
				channel->rd_buf_cnt);
		if((p = realloc(channel->rd_buf, channel->rd_buf_cnt)) != NULL)
			/* we can ignore errors... */
			channel->rd_buf = p;
		else if(channel->rd_buf_cnt == 0)
			/* ...except when it's not one */
			channel->rd_buf = NULL;
	}
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->modem, error->message, 1);
			g_error_free(error);
			/* fallthrough */
		case G_IO_STATUS_EOF:
		default:
			channel->wr_ppp_source = 0;
			event->connection.connected = 0;
			helper->event(helper->modem, event);
			_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_INIT);
			return FALSE;
	}
	if(channel->rd_buf_cnt > 0) /* there is more data to write */
		return TRUE;
	channel->wr_ppp_source = 0;
	return FALSE;
}


/* on_request_authenticate */
static HayesCommandStatus _on_request_authenticate(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	const char sim_pin[] = "SIM PIN";
	const char sim_puk[] = "SIM PUK";
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_AUTHENTICATION];
	guint timeout;

	switch((status = _on_request_generic(command, status, channel)))
	{
		case HCS_SUCCESS:
			break;
		case HCS_ERROR:
			event->authentication.status
				= MODEM_AUTHENTICATION_STATUS_ERROR;
			hayes->helper->event(hayes->helper->modem, event);
			/* fallthrough */
		default:
			return status;
	}
	/* XXX it should be bound to the request instead */
	if(event->authentication.name != NULL && (strcmp(sim_pin,
					event->authentication.name) == 0
				|| strcmp(sim_puk,
					event->authentication.name) == 0))
	{
		/* verify that it really worked */
		timeout = hayeschannel_has_quirks(channel,
				HAYES_QUIRK_CPIN_SLOW) ? 1000 : 0;
		channel->authenticate_count = 0;
		if(channel->authenticate_source != 0)
			g_source_remove(channel->authenticate_source);
		channel->authenticate_source = g_timeout_add(timeout,
				_on_channel_authenticate, channel);
	}
	else
	{
		event->authentication.status = MODEM_AUTHENTICATION_STATUS_OK;
		hayes->helper->event(hayes->helper->modem, event);
	}
	return status;
}


/* on_request_battery_level */
static HayesCommandStatus _on_request_battery_level(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_BATTERY_LEVEL];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_call */
static HayesCommandStatus _on_request_call(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_call_incoming */
static HayesCommandStatus _on_request_call_incoming(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS && status != HCS_ERROR)
		return status;
	event->call.direction = MODEM_CALL_DIRECTION_INCOMING;
	event->call.status = (status == HCS_SUCCESS)
		? MODEM_CALL_STATUS_ACTIVE : MODEM_CALL_STATUS_NONE;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_call_outgoing */
static HayesCommandStatus _on_request_call_outgoing(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS && status != HCS_ERROR)
		return status;
	event->call.direction = MODEM_CALL_DIRECTION_OUTGOING;
	event->call.status = (status == HCS_SUCCESS)
		? MODEM_CALL_STATUS_ACTIVE : MODEM_CALL_STATUS_NONE;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_call_status */
static HayesCommandStatus _on_request_call_status(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS && status != HCS_ERROR)
		return status;
	_hayes_request_type(hayes, channel, HAYES_REQUEST_PHONE_ACTIVE);
	return status;
}


/* on_request_contact_delete */
static HayesCommandStatus _on_request_contact_delete(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONTACT_DELETED];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_contact_list */
static HayesCommandStatus _on_request_contact_list(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	/* XXX could probably be more efficient */
	_hayes_request_type(channel->hayes, channel,
			MODEM_REQUEST_CONTACT_LIST);
	return status;
}


/* on_request_functional */
static HayesCommandStatus _on_request_functional(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

	switch((status = _on_request_generic(command, status, channel)))
	{
		case HCS_ERROR:
			/* try to enable */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_FUNCTIONAL_ENABLE);
			break;
		default:
			break;
	}
	return status;
}


/* on_request_functional_enable */
static HayesCommandStatus _on_request_functional_enable(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

	switch((status = _on_request_generic(command, status, channel)))
	{
		case HCS_ERROR:
#if 0 /* XXX ignore for now (may simply be missing the PIN code) */
			/* force a reset */
			_hayes_request_type(hayes,
					HAYES_REQUEST_FUNCTIONAL_ENABLE_RESET);
#endif
			break;
		case HCS_SUCCESS:
			_on_code_cfun(channel, "1"); /* XXX ugly workaround */
			break;
		case HCS_TIMEOUT:
			/* repeat request */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_FUNCTIONAL_ENABLE);
			break;
		default:
			break;
	}
	return status;
}


/* on_request_functional_enable_reset */
static HayesCommandStatus _on_request_functional_enable_reset(
		HayesCommand * command, HayesCommandStatus status,
		HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

	switch((status = _on_request_generic(command, status, channel)))
	{
		case HCS_SUCCESS:
			_on_code_cfun(channel, "1"); /* XXX ugly workaround */
			break;
		case HCS_TIMEOUT:
			/* repeat request */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_FUNCTIONAL_ENABLE);
			break;
		default:
			break;
	}
	return status;
}


/* on_request_generic */
static HayesCommandStatus _on_request_generic(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	char const * answer;
	char const * p;
	(void) channel;

	if(status != HCS_ACTIVE)
		return status;
	if((answer = hayes_command_get_answer(command)) == NULL)
		return status;
	/* look for the last line */
	while((p = strchr(answer, '\n')) != NULL)
		answer = ++p;
	if(strcmp(answer, "OK") == 0)
		return HCS_SUCCESS;
	else if(strcmp(answer, "ERROR") == 0)
		return HCS_ERROR;
	return status;
}


/* on_request_message */
static HayesCommandStatus _on_request_message(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	HayesRequestMessageData * data;

	if((status = _on_request_generic(command, status, channel))
			== HCS_SUCCESS
			|| status == HCS_ERROR || status == HCS_TIMEOUT)
		if((data = hayes_command_get_data(command)) != NULL)
		{
			free(data);
			hayes_command_set_data(command, NULL);
		}
	return status;
}


/* on_request_message_delete */
static HayesCommandStatus _on_request_message_delete(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MESSAGE_DELETED];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_message_list */
static HayesCommandStatus _on_request_message_list(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	HayesRequestMessageData * data;

	if((status = _on_request_generic(command, status, channel))
			== HCS_SUCCESS
			|| status == HCS_ERROR || status == HCS_TIMEOUT)
		if((data = hayes_command_get_data(command)) != NULL)
		{
			free(data);
			hayes_command_set_data(command, NULL);
		}
	return status;
}


/* on_request_message_send */
static HayesCommandStatus _on_request_message_send(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MESSAGE_SENT];
	char * pdu;

	if((pdu = hayes_command_get_data(command)) != NULL
			&& (status = _on_request_generic(command, status,
					channel)) == HCS_ACTIVE)
		_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_PDU);
	if(status == HCS_SUCCESS || status == HCS_ERROR
			|| status == HCS_TIMEOUT)
	{
		free(pdu);
		hayes_command_set_data(command, NULL);
	}
	if(status == HCS_ERROR)
	{
		event->message_sent.error = "Could not send message";
		event->message_sent.id = 0;
		hayes->helper->event(hayes->helper->modem, event);
	}
	return status;
}


/* on_request_model */
static HayesCommandStatus _on_request_model(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	return status;
}


/* on_request_registration */
static HayesCommandStatus _on_request_registration(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	/* force a registration status */
	_hayes_request_type(hayes, channel, HAYES_REQUEST_REGISTRATION);
	return status;
}


/* on_request_registration_automatic */
static HayesCommandStatus _on_request_registration_automatic(
		HayesCommand * command, HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];

	status = _on_request_generic(command, status, channel);
	switch(status)
	{
		case HCS_UNKNOWN:
		case HCS_QUEUED:
		case HCS_PENDING:
			break;
		case HCS_ACTIVE:
			event->registration.mode
				= MODEM_REGISTRATION_MODE_AUTOMATIC;
			event->registration.status
				= MODEM_REGISTRATION_STATUS_SEARCHING;
			hayes->helper->event(hayes->helper->modem, event);
			break;
		case HCS_ERROR:
		case HCS_TIMEOUT:
			event->registration.mode
				= MODEM_REGISTRATION_MODE_UNKNOWN;
			event->registration.status
				= MODEM_REGISTRATION_STATUS_UNKNOWN;
			hayes->helper->event(hayes->helper->modem, event);
			break;
		case HCS_SUCCESS:
			/* force a registration status */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_REGISTRATION);
			break;
	}
	return status;
}


/* on_request_registration_disabled */
static HayesCommandStatus _on_request_registration_disabled(
		HayesCommand * command, HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];

	if((status = _on_request_generic(command, status, channel))
			!= HCS_SUCCESS)
		return status;
	event->registration.mode = MODEM_REGISTRATION_MODE_DISABLED;
	/* force a registration status */
	_hayes_request_type(hayes, channel, HAYES_REQUEST_REGISTRATION);
	return status;
}


/* on_request_sim_pin_valid */
static HayesCommandStatus _on_request_sim_pin_valid(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_AUTHENTICATION];
	ModemRequest request;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, status);
#endif
	if((status = _on_request_generic(command, status, channel)) == HCS_ERROR
			|| status == HCS_TIMEOUT)
	{
		event->authentication.status
			= MODEM_AUTHENTICATION_STATUS_ERROR;
		hayes->helper->event(hayes->helper->modem, event);
		return status;
	}
	else if(status != HCS_SUCCESS)
		return status;
	hayes->helper->event(hayes->helper->modem, event);
	/* return if not successful */
	if(event->authentication.status != MODEM_AUTHENTICATION_STATUS_OK)
		return status;
	/* apply useful settings */
	_hayes_request_type(hayes, channel, HAYES_REQUEST_EXTENDED_ERRORS);
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_EXTENDED_RING_REPORTS);
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_CALL_PRESENTATION;
	request.call_presentation.enabled = 1;
	_hayes_request(hayes, &request);
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_CALL_WAITING_UNSOLLICITED_ENABLE);
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_CONNECTED_LINE_ENABLE);
	/* report new messages */
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_MESSAGE_UNSOLLICITED_ENABLE);
	/* report new notifications */
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_SUPPLEMENTARY_SERVICE_DATA_ENABLE);
	/* refresh the registration status */
	_hayes_request_type(hayes, channel,
			HAYES_REQUEST_REGISTRATION_UNSOLLICITED_ENABLE);
	/* refresh the current call status */
	_hayes_trigger(hayes, MODEM_EVENT_TYPE_CALL);
	/* refresh the contact list */
	_hayes_request_type(hayes, channel, MODEM_REQUEST_CONTACT_LIST);
	/* refresh the message list */
	_hayes_request_type(hayes, channel, MODEM_REQUEST_MESSAGE_LIST);
	return status;
}


/* on_request_unsupported */
static HayesCommandStatus _on_request_unsupported(HayesCommand * command,
		HayesCommandStatus status, HayesChannel * channel)
{
	/* FIXME report an unsupported event with the result of the command */
	return _on_request_generic(command, status, channel);
}


/* on_code_call_error */
static void _on_code_call_error(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	(void) answer;

	if(command != NULL)
		hayes_command_set_status(command, HCS_ERROR);
	_hayes_request_type(hayes, channel, HAYES_REQUEST_PHONE_ACTIVE);
}


/* on_code_cbc */
static void _on_code_cbc(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_BATTERY_LEVEL];
	int res;
	unsigned int u;
	unsigned int v;
	double f;

	if((res = sscanf(answer, "%u,%u", &u, &v)) != 2)
		return;
	event->battery_level.status = MODEM_BATTERY_STATUS_UNKNOWN;
	event->battery_level.charging = 0;
	if(u == 0)
		u = MODEM_BATTERY_STATUS_CONNECTED;
	else if(u == 1)
		u = MODEM_BATTERY_STATUS_CHARGING;
	else if(u == 2)
		u = MODEM_BATTERY_STATUS_NONE;
	else if(u == 3)
		u = MODEM_BATTERY_STATUS_ERROR;
	else
		u = MODEM_BATTERY_STATUS_UNKNOWN;
	switch((event->battery_level.status = u))
	{
		case MODEM_BATTERY_STATUS_CHARGING:
			event->battery_level.charging = 1;
			/* fallthrough */
		case MODEM_BATTERY_STATUS_CONNECTED:
			f = v;
			if(hayeschannel_has_quirks(channel,
						HAYES_QUIRK_BATTERY_70))
				f /= 70.0;
			else
				f /= 100.0;
			f = max(f, 0.0);
			event->battery_level.level = min(f, 1.0);
			break;
		default:
			event->battery_level.level = 0.0 / 0.0;
			break;
	}
}


/* on_code_cfun */
static void _on_code_cfun(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_STATUS];
	unsigned int u;

	if(sscanf(answer, "%u", &u) != 1)
		return;
	switch(u)
	{
		case 1:
			/* report being online */
			event->status.status = MODEM_STATUS_ONLINE;
			break;
		case 4: /* antennas disabled */
		case 0: /* telephony disabled */
		default:
			/* FIXME this is maybe not the right event type */
			event->status.status = MODEM_STATUS_OFFLINE;
			break;
	}
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cgatt */
static void _on_code_cgatt(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
	unsigned int u;

	if(sscanf(answer, "%u", &u) != 1)
		return;
	free(channel->registration_media);
	channel->registration_media = NULL;
	if(u == 1)
		event->registration.media = "GPRS";
	else
		event->registration.media = NULL;
	/* this is usually worth an event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cgmi */
static void _on_code_cgmi(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];
	char * p;

	if(answer[0] == '\0' || strcmp(answer, "OK") == 0
			|| (p = strdup(answer)) == NULL) /* XXX report error? */
		return;
	free(channel->model_vendor);
	channel->model_vendor = p;
	event->model.vendor = p;
}


/* on_code_cgmm */
static void _on_code_cgmm(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];
	char * p;

	if(answer[0] == '\0' || strcmp(answer, "OK") == 0
			|| (p = strdup(answer)) == NULL) /* XXX report error? */
		return;
	free(channel->model_name);
	channel->model_name = p;
	event->model.name = p;
	hayeschannel_set_quirks(channel, hayes_quirks(event->model.vendor, p));
}


/* on_code_cgmr */
static void _on_code_cgmr(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];
	char * p;

	if(answer[0] == '\0' || strcmp(answer, "OK") == 0
			|| (p = strdup(answer)) == NULL) /* XXX report error? */
		return;
	free(channel->model_version);
	channel->model_version = p;
	event->model.version = p;
}


/* on_code_cgsn */
static void _on_code_cgsn(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];
	char * p;

	if(answer[0] == '\0' || strcmp(answer, "OK") == 0
			|| (p = strdup(answer)) == NULL) /* XXX report error? */
		return;
	free(channel->model_serial);
	channel->model_serial = p;
	event->model.serial = p;
}


/* on_code_cimi */
static void _on_code_cimi(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MODEL];
	char * p;

	if(answer[0] == '\0' || strcmp(answer, "OK") == 0
			|| (p = strdup(answer)) == NULL) /* XXX report error? */
		return;
	free(channel->model_identity);
	channel->model_identity = p;
	event->model.identity = p;
}


/* on_code_clip */
static void _on_code_clip(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];
	char buf[32];
	unsigned int u;

	if(sscanf(answer, "\"%31[^\"]\",%u", buf, &u) != 2)
		return;
	buf[sizeof(buf) - 1] = '\0';
	free(channel->call_number);
	switch(u)
	{
		case 145:
			if((channel->call_number = malloc(sizeof(buf) + 1))
					== NULL)
				break;
			snprintf(channel->call_number, sizeof(buf) + 1, "%s%s",
					"+", buf);
			break;
		default:
			channel->call_number = strdup(buf);
			break;
	}
	/* this is always an unsollicited event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cme_error */
static void _cme_command_repeat(HayesChannel * channel, HayesCommand * command);
static void _cme_error_registration(HayesChannel * channel, char const * error);

static void _on_code_cme_error(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	/* XXX ugly */
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	unsigned int u;
	ModemEvent * event;

	if(command != NULL)
		hayes_command_set_status(command, HCS_ERROR);
	if(sscanf(answer, "%u", &u) != 1)
		return;
	switch(u)
	{
		case 10:  /* SIM not inserted */
			/* FIXME also display an icon in the UI */
			_cme_error_registration(channel, "SIM not inserted");
			break;
		case 11: /* SIM PIN required */
			_on_code_cpin(channel, "SIM PIN");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 12: /* SIM PUK required */
			_on_code_cpin(channel, "SIM PUK");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 100: /* unknown error */
			if(hayeschannel_has_quirks(channel,
						HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR) == 0)
				break;
			/* fallthrough */
		case 14: /* SIM busy */
			_cme_command_repeat(channel, command);
			break;
		case 30:  /* No network service */
			_cme_error_registration(channel, "No network service");
			break;
		case 31:  /* Network timeout */
			event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			event->registration.signal = 0.0 / 0.0;
			helper->event(helper->modem, event);
			break;
		case 32: /* emergency calls only */
			event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			free(channel->registration_media);
			channel->registration_media = NULL;
			event->registration.media = NULL;
			free(channel->registration_operator);
			channel->registration_operator = NULL;
			event->registration._operator = "SOS";
			event->registration.status
				= MODEM_REGISTRATION_STATUS_REGISTERED;
			helper->event(helper->modem, event);
			/* verify the SIM card */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_SIM_PIN_VALID);
			break;
		case 112: /* Location area not allowed */
		case 113: /* Roaming not allowed in this location area */
			event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
			event->registration.status
				= MODEM_REGISTRATION_STATUS_DENIED;
			helper->event(helper->modem, event);
			break;
		case 262: /* SIM blocked */
			_cme_error_registration(channel, "SIM blocked");
			break;
		default:  /* FIXME implement the rest */
		case 3:   /* Operation not allowed */
		case 4:   /* Operation not supported */
		case 16:  /* Incorrect SIM PIN/PUK */
		case 20:  /* Memory full */
		case 107: /* GPRS services not allowed */
		case 148: /* Unspecified GPRS error */
		case 263: /* Invalid block */
			break;
	}
}

static void _cme_command_repeat(HayesChannel * channel, HayesCommand * command)
{
	const guint timeout = 5000;
	HayesCommand * p;

	if(command == NULL)
		return;
	if((p = hayes_command_new_copy(command)) == NULL)
		return;
	hayes_command_set_data(p, hayes_command_get_data(command));
	hayes_command_set_data(command, NULL);
	channel->queue_timeout = g_slist_append(channel->queue_timeout, p);
	if(channel->source == 0)
		channel->source = g_timeout_add(timeout, _on_queue_timeout,
				channel);
}

static void _cme_error_registration(HayesChannel * channel, char const * error)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event;
	ModemPluginHelper * helper = hayes->helper;

	/* update the authentication status */
	event = &channel->events[MODEM_EVENT_TYPE_AUTHENTICATION];
	free(channel->authentication_error);
	channel->authentication_error = NULL;
	event->authentication.error = error;
	/* report the registration error */
	event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
	free(channel->registration_media);
	channel->registration_media = NULL;
	event->registration.media = NULL;
	free(channel->registration_operator);
	channel->registration_operator = NULL;
	event->registration._operator = NULL;
	event->registration.signal = 0.0 / 0.0;
	event->registration.status = MODEM_REGISTRATION_STATUS_DENIED;
	helper->event(helper->modem, event);
}


/* on_code_cmgl */
static void _on_code_cmgl(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	/* XXX ugly */
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	ModemRequest request;
	unsigned int id;
	unsigned int u;
	HayesRequestMessageData * data;
	ModemMessageFolder folder = MODEM_MESSAGE_FOLDER_UNKNOWN;
	ModemMessageStatus status = MODEM_MESSAGE_STATUS_READ;

	/* XXX we could already be reading the message at this point */
	if(sscanf(answer, "%u,%u,%u,%u", &id, &u, &u, &u) != 4
			&& sscanf(answer, "%u,%u,,%u", &id, &u, &u) != 3)
		/* XXX we may be stuck in PDU mode at this point */
		return;
	request.type = MODEM_REQUEST_MESSAGE;
	request.message.id = id;
	if(command != NULL && (data = hayes_command_get_data(command)) != NULL)
	{
		folder = data->folder;
		status = data->status;
	}
	if((data = malloc(sizeof(*data))) != NULL)
	{
		data->id = id;
		data->folder = folder;
		data->status = status;
	}
	if(_hayes_request_channel(hayes, channel, &request, data) != 0)
		free(data);
}


/* on_code_cmgr */
static char * _cmgr_pdu_parse(char const * pdu, time_t * timestamp,
		char * number, ModemMessageEncoding * encoding,
		size_t * length);
static char * _cmgr_pdu_parse_encoding_data(char const * pdu, size_t len,
		size_t i, size_t hdr, ModemMessageEncoding * encoding,
		size_t * length);
static char * _cmgr_pdu_parse_encoding_default(char const * pdu, size_t len,
                size_t i, size_t hdr, ModemMessageEncoding * encoding,
		size_t * length);
static void _cmgr_pdu_parse_number(unsigned int type, char const * number,
		size_t length, char * buf);
static time_t _cmgr_pdu_parse_timestamp(char const * timestamp);

static void _on_code_cmgr(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	/* XXX ugly */
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MESSAGE];
	char buf[32];
	char number[32];
	char date[32];
	struct tm t;
	unsigned int mbox;
	unsigned int alpha = 0;
	unsigned int length;
	char * p;
	HayesRequestMessageData * data;

	/* text mode support */
	if(sscanf(answer, "\"%31[^\"]\",\"%31[^\"]\",,\"%31[^\"]\"", buf,
				number, date) == 3)
	{
		number[sizeof(number) - 1] = '\0';
		string_delete(channel->message_number);
		channel->message_number = strdup(number);
		event->message.number = channel->message_number;
		date[sizeof(date) - 1] = '\0';
		memset(&t, 0, sizeof(t));
		if(strptime(date, "%y/%m/%d,%H:%M:%S", &t) == NULL)
			/* XXX also parse the timezone? */
			localtime_r(NULL, &t);
		event->message.date = mktime(&t);
		event->message.length = 0;
		return; /* we need to wait for the next line */
	}
	/* PDU mode support */
	if(sscanf(answer, "%u,%u,%u", &mbox, &alpha, &length) == 3
			|| sscanf(answer, "%u,,%u", &mbox, &length) == 2)
		return; /* we need to wait for the next line */
	/* message content */
	if(event->message.length == 0) /* XXX assumes this is text mode */
	{
		/* FIXME guarantee this would not happen */
		if(command == NULL || (data = hayes_command_get_data(command))
				== NULL)
			return;
		event->message.id = data->id;
		event->message.folder = data->folder;
		event->message.status = data->status;
		event->message.encoding = MODEM_MESSAGE_ENCODING_UTF8;
		event->message.content = answer;
		event->message.length = strlen(answer);
		hayes->helper->event(hayes->helper->modem, event);
		return;
	}
	if((p = _cmgr_pdu_parse(answer, &event->message.date, number,
					&event->message.encoding,
					&event->message.length)) == NULL)
		return;
	/* FIXME guarantee this would not happen */
	if(command == NULL || (data = hayes_command_get_data(command)) == NULL)
		return;
	event->message.id = data->id;
	event->message.folder = data->folder;
	event->message.status = data->status;
	event->message.number = number; /* XXX */
	event->message.content = p;
	hayes->helper->event(hayes->helper->modem, event);
	free(p);
}

static char * _cmgr_pdu_parse(char const * pdu, time_t * timestamp,
		char * number, ModemMessageEncoding * encoding, size_t * length)
{
	size_t len;
	unsigned int smscl;
	unsigned int tp;
	unsigned int hdr;
	unsigned int addrl;
	unsigned int pid;
	unsigned int dcs;
	unsigned int datal;
	unsigned int u;
	char const * q;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, pdu);
#endif
	len = strlen(pdu);
	if(sscanf(pdu, "%02X", &smscl) != 1) /* SMSC length */
		return NULL;
	if((smscl * 2) + 2 > len)
		return NULL;
	q = pdu + (smscl * 2) + 2;
	if(sscanf(q, "%02X", &tp) != 1)
		return NULL;
	if((tp & 0x03) != 0x00) /* TP-MTI not SMS-DELIVER */
		return NULL;
	hdr = ((tp & 0x40) == 0x40) ? 1 : 0; /* TP-UDHI header present */
	if((smscl * 2) + 4 > len)
		return NULL;
	q = pdu + (smscl * 2) + 4;
	if(sscanf(q, "%02X", &addrl) != 1) /* address length */
		return NULL;
	if((smscl * 2) + 6 > len)
		return NULL;
	q = pdu + (smscl * 2) + 6;
	if(sscanf(q, "%02X", &u) != 1) /* type of address */
		return NULL;
	/* FIXME this probably depends on the type of address */
	if(addrl % 2 == 1)
		addrl++;
	if((smscl * 2) + 2 + 4 + addrl + 2 > len)
		return NULL;
	_cmgr_pdu_parse_number(u, q + 2, addrl, number);
	q = pdu + (smscl * 2) + 2 + 4 + addrl + 2;
	if(sscanf(q, "%02X", &pid) != 1) /* PID */
		return NULL;
	if((smscl * 2) + 2 + 4 + addrl + 4 > len)
		return NULL;
	q = pdu + (smscl * 2) + 2 + 4 + addrl + 4;
	if(sscanf(q, "%02X", &dcs) != 1) /* DCS */
		return NULL;
	if((smscl * 2) + 2 + 4 + addrl + 6 > len)
		return NULL;
	q = pdu + (smscl * 2) + 2 + 4 + addrl + 6;
	if(timestamp != NULL)
		*timestamp = _cmgr_pdu_parse_timestamp(q);
	if((smscl * 2) + 2 + 4 + addrl + 6 + 14 > len)
		return NULL;
	q = pdu + (smscl * 2) + 2 + 4 + addrl + 6 + 14;
	if(sscanf(q, "%02X", &datal) != 1) /* data length */
		return NULL;
	/* XXX check the data length */
	if((i = (smscl * 2) + 2 + 4 + addrl + 6 + 16) > len)
		return NULL;
	if(hdr != 0 && sscanf(&pdu[i], "%02X", &hdr) != 1)
		return NULL;
	if(dcs == 0x00)
		return _cmgr_pdu_parse_encoding_default(pdu, len, i, hdr,
				encoding, length);
	if(dcs == 0x04)
		return _cmgr_pdu_parse_encoding_data(pdu, len, i, hdr,
				encoding, length);

	return NULL;
}

static char * _cmgr_pdu_parse_encoding_data(char const * pdu, size_t len,
		size_t i, size_t hdr, ModemMessageEncoding * encoding,
		size_t * length)
{
	unsigned char * p;
	size_t j;
	unsigned int u;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((p = malloc(len - i + 1)) == NULL) /* XXX 2 times big enough? */
		return NULL;
	/* FIXME actually parse the header */
	if(hdr != 0)
		i += 2 + (hdr * 2);
	for(j = 0; i + 1 < len; i+=2)
	{
		if(sscanf(&pdu[i], "%02X", &u) != 1)
		{
			free(p);
			return NULL;
		}
		p[j++] = u;
	}
	*encoding = MODEM_MESSAGE_ENCODING_DATA;
	*length = j;
	p[j] = '\0';
	return (char *)p;
}

static char * _cmgr_pdu_parse_encoding_default(char const * pdu, size_t len,
                size_t i, size_t hdr, ModemMessageEncoding * encoding,
		size_t * length)
{
	unsigned char * p;
	size_t j;
	unsigned char rest;
	int shift = 0;
	char const * q;
	unsigned int u;
	unsigned char byte;
	char * r;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%zu, %zu)\n", __func__, i, hdr);
#endif
	if((p = malloc(len - i + 1)) == NULL)
		return NULL;
	if(hdr != 0)
	{
		/* FIXME actually parse the header */
		i += 2 + (hdr * 2);
		shift = (hdr + 1) % 7;
	}
	p[0] = '\0';
	for(j = 0, rest = 0; i + 1 < len; i+=2)
	{
		q = &pdu[i];
		if(sscanf(q, "%02X", &u) != 1)
			break; /* FIXME report an error instead? */
		byte = u;
		p[j] = (((byte << (shift + 1)) >> (shift + 1)) << shift) & 0x7f;
		p[j] |= rest;
		p[j] = _hayes_convert_gsm_to_iso(p[j]);
		/* ignore the first character if there is a header */
		if(hdr == 0 || j != 0)
			j++;
		rest = (byte >> (7 - shift)) & 0x7f;
		if(++shift == 7)
		{
			shift = 0;
			p[j++] = rest;
			rest = 0;
		}
	}
	*encoding = MODEM_MESSAGE_ENCODING_UTF8;
	if((r = g_convert((char *)p, j, "UTF-8", "ISO-8859-1", NULL, NULL,
					NULL)) != NULL)
	{
		free(p);
		p = (unsigned char *)r;
		j = strlen(r);
	}
	*length = j;
	return (char *)p;
}


static void _cmgr_pdu_parse_number(unsigned int type, char const * number,
		size_t length, char * buf)
{
	char * b = buf;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(type == 0x91)
		*(b++) = '+';
	for(i = 0; i < length - 1 && i < 32 - 1; i+=2)
	{
		if((number[i] != 'F' && (number[i] < '0' || number[i] > '9'))
				|| number[i + 1] < '0' || number[i + 1] > '9')
			break;
		b[i] = number[i + 1];
		if((b[i + 1] = number[i]) == 'F')
			b[i + 1] = '\0';
	}
	b[i] = '\0';
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %zu) => \"%s\"\n", __func__, number,
			length, b);
#endif
}

static time_t _cmgr_pdu_parse_timestamp(char const * timestamp)
{
	char const * p = timestamp;
	size_t i;
	struct tm tm;
	time_t t;
#if defined(__NetBSD__)
	timezone_t tz;
#endif
#ifdef DEBUG
	char buf[32];
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, timestamp);
#endif
	if(strlen(p) < 14)
		return 0;
	for(i = 0; i < 14; i++)
		if(p[i] < '0' || p[i] > '9')
			return 0;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = (p[0] - '0') + ((p[1] - '0') * 10);
	tm.tm_year = (tm.tm_year > 70) ? tm.tm_year : (100 + tm.tm_year);
	tm.tm_mon = (p[2] - '0') + ((p[3] - '0') * 10);
	if(tm.tm_mon > 0)
		tm.tm_mon--;
	tm.tm_mday = (p[4] - '0') + ((p[5] - '0') * 10);
	tm.tm_hour = (p[6] - '0') + ((p[7] - '0') * 10);
	tm.tm_min = (p[8] - '0') + ((p[9] - '0') * 10);
	tm.tm_sec = (p[10] - '0') + ((p[11] - '0') * 10);
#ifdef DEBUG
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &tm);
	fprintf(stderr, "DEBUG: %s() => \"%s\"\n", __func__, buf);
#endif
#if !defined(__NetBSD__)
	t = mktime(&tm);
#else
	/* XXX assumes UTC */
	tz = tzalloc("UTC");
	t = mktime_z(tz, &tm);
	tzfree(tz);
#endif
	return t;
}


/* on_code_cmgs */
static void _on_code_cmgs(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_MESSAGE_SENT];
	unsigned int u;

	if(sscanf(answer, "%u", &u) != 1)
		return;
	event->message_sent.error = NULL;
	event->message_sent.id = u;
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cms_error */
static void _on_code_cms_error(HayesChannel * channel, char const * answer)
{
	const guint timeout = 5000;
	Hayes * hayes = channel->hayes;
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	unsigned int u;
	HayesCommand * p;

	if(command != NULL)
		hayes_command_set_status(command, HCS_ERROR);
	if(sscanf(answer, "%u", &u) != 1)
		return;
	switch(u)
	{
		case 311: /* SIM PIN required */
			_on_code_cpin(channel, "SIM PIN");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 316: /* SIM PUK required */
			_on_code_cpin(channel, "SIM PUK");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 317: /* SIM PIN2 required */
			_on_code_cpin(channel, "SIM PIN2");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 318: /* SIM PUK2 required */
			_on_code_cpin(channel, "SIM PUK2");
			_hayes_trigger(hayes, MODEM_EVENT_TYPE_AUTHENTICATION);
			break;
		case 500: /* Unknown error */
			if(hayeschannel_has_quirks(channel,
						HAYES_QUIRK_REPEAT_ON_UNKNOWN_ERROR) == 0)
				break;
			/* fallthrough */
		case 314: /* SIM busy */
		case 532: /* SIM not ready */
			/* repeat the command */
			/* FIXME duplicated from _on_code_cme_error() */
			if(command == NULL)
				break;
			if((p = hayes_command_new_copy(command)) == NULL)
				break;
			hayes_command_set_data(p,
					hayes_command_get_data(command));
			hayes_command_set_data(command, NULL);
			channel->queue_timeout = g_slist_append(
					channel->queue_timeout, p);
			if(channel->source == 0)
				channel->source = g_timeout_add(timeout,
						_on_queue_timeout, channel);
			break;
		case 21:  /* Short message transfer rejected */
		case 96:  /* Invalid mandatory information */
		case 303: /* Operation not supported */
		case 321: /* Invalid memory index */
		default:  /* FIXME implement the rest */
			break;
	}
}


/* on_code_cmti */
static void _on_code_cmti(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	char buf[32];
	unsigned int u;
	ModemRequest request;

	if(sscanf(answer, "\"%31[^\"]\",%u", buf, &u) != 2)
		return;
	buf[sizeof(buf) - 1] = '\0';
	/* fetch the new message directly */
	memset(&request, 0, sizeof(request));
	request.type = MODEM_REQUEST_MESSAGE;
	request.message.id = u;
	_hayes_request(hayes, &request);
}


/* on_code_connect */
static void _on_code_connect(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	char * argv[] = { "/usr/sbin/" PROGNAME_PPPD, PROGNAME_PPPD,
		"call", "phone", "user", "", "password", "", NULL };
	char const * p;
	gboolean res;
	const GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	int wfd;
	int rfd;
	GError * error = NULL;
	(void) answer;

	if(command != NULL) /* XXX else report error? */
		hayes_command_set_status(command, HCS_SUCCESS);
	_hayes_set_mode(hayes, channel, HAYESCHANNEL_MODE_DATA);
	/* pppd */
	if((p = helper->config_get(helper->modem, "pppd")) != NULL)
	{
		if((argv[0] = strdup(p)) == NULL)
		{
			hayes->helper->error(NULL, strerror(errno), 1);
			_hayes_reset(hayes);
			return;
		}
		argv[1] = basename(argv[0]);
	}
	/* username */
	if(channel->gprs_username != NULL)
		argv[5] = channel->gprs_username;
	/* password */
	if(channel->gprs_password != NULL)
		argv[7] = channel->gprs_password;
	res = g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL,
			NULL, &wfd, &rfd, NULL, &error);
	if(p != NULL)
		free(argv[0]);
	if(res == FALSE)
	{
		hayes->helper->error(NULL, error->message, 1);
		g_error_free(error);
		_hayes_reset(hayes);
		return;
	}
	channel->rd_ppp_channel = g_io_channel_unix_new(rfd);
	if(g_io_channel_set_encoding(channel->rd_ppp_channel, NULL, &error)
			!= G_IO_STATUS_NORMAL)
	{
		hayes->helper->error(NULL, error->message, 1);
		g_error_free(error);
		error = NULL;
	}
	g_io_channel_set_buffered(channel->rd_ppp_channel, FALSE);
	channel->rd_ppp_source = g_io_add_watch(channel->rd_ppp_channel,
			G_IO_IN, _on_watch_can_read_ppp, channel);
	channel->wr_ppp_channel = g_io_channel_unix_new(wfd);
	if(g_io_channel_set_encoding(channel->wr_ppp_channel, NULL, &error)
			!= G_IO_STATUS_NORMAL)
	{
		hayes->helper->error(NULL, error->message, 1);
		g_error_free(error);
	}
	g_io_channel_set_buffered(channel->wr_ppp_channel, FALSE);
	channel->wr_ppp_source = 0;
	event->connection.connected = 1;
	event->connection.in = 0;
	event->connection.out = 0;
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_colp */
static void _on_code_colp(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];
	char buf[32];
	unsigned int u;

	if(sscanf(answer, "\"%31[^\"]\",%u", buf, &u) != 2)
		return; /* FIXME there may be different or more information */
	buf[sizeof(buf) - 1] = '\0';
	free(channel->call_number);
	switch(u)
	{
		case 145:
			if((channel->call_number = malloc(sizeof(buf) + 1))
					== NULL)
				break;
			snprintf(channel->call_number, sizeof(buf) + 1, "%s%s",
					"+", buf);
			break;
		default:
			channel->call_number = strdup(buf);
			break;
	}
	event->call.number = channel->call_number;
}


/* on_code_cops */
static void _on_code_cops(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
	unsigned int u;
	unsigned int v = 0;
	char buf[32] = "";
	unsigned int w;

	if(sscanf(answer, "%u,%u,\"%31[^\"]\",%u", &u, &v, buf, &w) < 1)
		return;
	switch(u)
	{
		case 0:
			u = MODEM_REGISTRATION_MODE_AUTOMATIC;
			break;
		case 1:
			u = MODEM_REGISTRATION_MODE_MANUAL;
			break;
		case 2:
			u = MODEM_REGISTRATION_MODE_DISABLED;
			break;
		case 3: /* only for setting the format */
		default:
			u = event->registration.mode;
			break;
	}
	event->registration.mode = u;
	free(channel->registration_operator);
	channel->registration_operator = NULL;
	event->registration._operator = NULL;
	if(v != 0)
		/* force alphanumeric format */
		_hayes_request_type(hayes, channel,
				HAYES_REQUEST_OPERATOR_FORMAT_LONG);
	else
	{
		buf[sizeof(buf) - 1] = '\0';
		channel->registration_operator = strdup(buf);
		event->registration._operator = channel->registration_operator;
	}
	/* refresh registration data */
	_hayes_request_type(hayes, channel, MODEM_REQUEST_SIGNAL_LEVEL);
	_hayes_request_type(hayes, channel, HAYES_REQUEST_GPRS_ATTACHED);
	/* this is usually worth an event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cpas */
static void _on_code_cpas(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemPluginHelper * helper = hayes->helper;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];
	unsigned int u;

	if(sscanf(answer, "%u", &u) != 1)
		return;
	switch(u)
	{
		case 0:
			event->call.status = MODEM_CALL_STATUS_NONE;
			event->call.direction = MODEM_CALL_DIRECTION_NONE;
			/* report connection status */
			event = &channel->events[MODEM_EVENT_TYPE_CONNECTION];
			event->connection.connected = 0;
			event->connection.in = 0;
			event->connection.out = 0;
			helper->event(helper->modem, event);
			break;
		case 3:
			event->call.status = MODEM_CALL_STATUS_RINGING;
			/* report event */
			helper->event(helper->modem, event);
			break;
		case 4:
			event->call.status = MODEM_CALL_STATUS_ACTIVE;
			event->call.direction = MODEM_CALL_DIRECTION_NONE;
			break;
		case 2: /* XXX unknown */
		default:
			break;
	}
}


/* on_code_cpbr */
static void _on_code_cpbr(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemRequest request;
	HayesRequestContactList list;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CONTACT];
	unsigned int u;
	unsigned int v;
	char number[32];
	char name[32];
	char * p;

	if(sscanf(answer, "(%u-%u)", &u, &v) == 2)
	{
		memset(&request, 0, sizeof(request));
		request.type = HAYES_REQUEST_CONTACT_LIST;
		list.from = u;
		list.to = v;
		request.plugin.data = &list;
		_hayes_request(hayes, &request);
		return;
	}
	if(sscanf(answer, "%u,\"%31[^\"]\",%u,\"%31[^\"]\"",
				&event->contact.id, number, &u, name) != 4)
		return;
	switch(u)
	{
		case 145:
			if(number[0] == '+')
				break;
			/* prefix the number with a "+" */
			memmove(&number[1], number, sizeof(number) - 1);
			number[0] = '+';
			break;
	}
	number[sizeof(number) - 1] = '\0';
	free(channel->contact_number);
	channel->contact_number = strdup(number);
	event->contact.number = channel->contact_number;
	name[sizeof(name) - 1] = '\0';
#if 1 /* FIXME is it really always in GSM? */
	_hayes_convert_gsm_string_to_iso(name);
#endif
	if((p = g_convert(name, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL))
			                        != NULL)
	{
		snprintf(name, sizeof(name), "%s", p);
		g_free(p);
	}
	free(channel->contact_name);
	channel->contact_name = strdup(name);
	event->contact.name = channel->contact_name;
	event->contact.status = MODEM_CONTACT_STATUS_OFFLINE;
	/* send event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cpin */
static void _on_code_cpin(HayesChannel * channel, char const * answer)
{
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_AUTHENTICATION];
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, answer);
#endif
	if(strcmp(answer, "READY") == 0)
	{
		event->authentication.status = MODEM_AUTHENTICATION_STATUS_OK;
		hayescommon_source_reset(&channel->authenticate_source);
		channel->authenticate_count = 0;
	}
	else if(strcmp(answer, "SIM PIN") == 0
			|| strcmp(answer, "SIM PUK") == 0)
	{
		free(channel->authentication_name);
		p = strdup(answer);
		channel->authentication_name = p;
		event->authentication.name = p;
		event->authentication.method = MODEM_AUTHENTICATION_METHOD_PIN;
		event->authentication.status
			= MODEM_AUTHENTICATION_STATUS_REQUIRED;
		/* FIXME also provide remaining retries */
	}
}


/* on_code_creg */
static void _on_code_creg(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
	int res;
	unsigned int u[4] = { 0, 0, 0, 0 };

	res = sscanf(answer, "%u,%u,%X,%X", &u[0], &u[1], &u[2], &u[3]);
	if(res == 1)
		res = sscanf(answer, "%u,\"%X\",\"%X\"", &u[1], &u[2], &u[3]);
	else if(res == 2)
		res = sscanf(answer, "%u,%u,\"%X\",\"%X\"", &u[0], &u[1], &u[2],
				&u[3]);
	else if(res == 3)
		res = sscanf(answer, "%u,%X,%X", &u[1], &u[2], &u[3]);
	if(res == 0)
		return;
	u[0] = event->registration.mode;
	event->registration.roaming = 0;
	switch(u[1])
	{
		case 0:
			u[0] = MODEM_REGISTRATION_MODE_DISABLED;
			u[1] = MODEM_REGISTRATION_STATUS_NOT_SEARCHING;
			break;
		case 1:
			if(u[0] != MODEM_REGISTRATION_MODE_MANUAL)
				u[0] = MODEM_REGISTRATION_MODE_AUTOMATIC;
			u[1] = MODEM_REGISTRATION_STATUS_REGISTERED;
			break;
		case 2:
			if(u[0] != MODEM_REGISTRATION_MODE_MANUAL)
				u[0] = MODEM_REGISTRATION_MODE_AUTOMATIC;
			u[1] = MODEM_REGISTRATION_STATUS_SEARCHING;
			break;
		case 3:
			u[1] = MODEM_REGISTRATION_STATUS_DENIED;
			break;
		case 5:
			if(u[0] != MODEM_REGISTRATION_MODE_MANUAL)
				u[0] = MODEM_REGISTRATION_MODE_AUTOMATIC;
			u[1] = MODEM_REGISTRATION_STATUS_REGISTERED;
			event->registration.roaming = 1;
			break;
		case 4: /* unknown */
		default:
#ifdef DEBUG
			if(u[1] != 4)
				fprintf(stderr, "DEBUG: %s() Unknown CREG %u\n",
						__func__, u[1]);
#endif
			u[0] = MODEM_REGISTRATION_MODE_UNKNOWN;
			u[1] = MODEM_REGISTRATION_STATUS_UNKNOWN;
			break;
	}
	event->registration.mode = u[0];
	switch((event->registration.status = u[1]))
	{
		case MODEM_REGISTRATION_STATUS_REGISTERED:
			/* refresh registration data */
			_hayes_request_type(hayes, channel,
					HAYES_REQUEST_OPERATOR);
			break;
		default:
			free(channel->registration_media);
			channel->registration_media = NULL;
			event->registration.media = NULL;
			free(channel->registration_operator);
			channel->registration_operator = NULL;
			event->registration._operator = NULL;
			event->registration.signal = 0.0 / 0.0;
			break;
	}
	/* this is usually an unsollicited event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cring */
static void _on_code_cring(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_CALL];

	if(strcmp(answer, "VOICE") == 0)
		event->call.call_type = MODEM_CALL_TYPE_VOICE;
	event->call.status = MODEM_CALL_STATUS_RINGING;
	event->call.direction = MODEM_CALL_DIRECTION_INCOMING;
	event->call.number = "";
	/* this is always an unsollicited event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_csq */
static void _on_code_csq(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_REGISTRATION];
	unsigned int u;
	unsigned int v;

	if(sscanf(answer, "%u,%u", &u, &v) != 2)
		return;
	if(u > 31)
		event->registration.signal = 0.0 / 0.0;
	else if(u >= 20)
		event->registration.signal = 1.0;
	else
	{
		event->registration.signal = (u > 4) ? (u - 4) : 0;
		event->registration.signal = event->registration.signal / 16.0;
	}
	/* this is usually worth an event */
	hayes->helper->event(hayes->helper->modem, event);
}


/* on_code_cusd */
static void _on_code_cusd(HayesChannel * channel, char const * answer)
{
	Hayes * hayes = channel->hayes;
	ModemEvent * event = &channel->events[MODEM_EVENT_TYPE_NOTIFICATION];
	unsigned int u;
	char buf[32];

	if(sscanf(answer, "%u,\"%31[^\"]\",%u", &u, buf, &u) >= 2)
	{
		event->notification.ntype = MODEM_NOTIFICATION_TYPE_INFO;
		event->notification.title = NULL;
		buf[sizeof(buf) - 1] = '\0';
		event->notification.content = buf;
		hayes->helper->event(hayes->helper->modem, event);
	}
}


/* on_code_ext_error */
static void _on_code_ext_error(HayesChannel * channel, char const * answer)
{
	/* XXX ugly */
	HayesCommand * command = (channel->queue != NULL)
		? channel->queue->data : NULL;
	unsigned int u;

	if(command != NULL)
		hayes_command_set_status(command, HCS_ERROR);
	if(sscanf(answer, "%u", &u) != 1)
		return;
	switch(u)
	{
		case 0:
		default: /* FIXME implement */
			break;
	}
}


/* helpers */
/* is_ussd_code */
static int _is_ussd_code(char const * number)
{
	if(number == NULL || number[0] != '*')
		return 0;
	for(number++; *number != '\0'; number++)
		if(*number == '#')
		{
			if(*(number + 1) == '\0')
				return 1;
			continue;
		}
		else if((*number >= '0' && *number <= '9') || *number == '*')
			continue;
		else
			return 0;
	return 0;
}
