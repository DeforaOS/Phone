TARGETS	= hayes modems oss pdu plugins ussd tests.log
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W
CFLAGS	= -Wall -g -O2
LDFLAGSF= -W
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

hayes_OBJS = hayes.o
hayes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
hayes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

hayes: $(hayes_OBJS)
	$(CC) -o hayes $(hayes_OBJS) $(hayes_LDFLAGS)

modems_OBJS = modems.o
modems_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
modems_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

modems: $(modems_OBJS)
	$(CC) -o modems $(modems_OBJS) $(modems_LDFLAGS)

oss_OBJS = oss.o
oss_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
oss_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop` -lossaudio

oss: $(oss_OBJS)
	$(CC) -o oss $(oss_OBJS) $(oss_LDFLAGS)

pdu_OBJS = pdu.o
pdu_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
pdu_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

pdu: $(pdu_OBJS)
	$(CC) -o pdu $(pdu_OBJS) $(pdu_LDFLAGS)

plugins_OBJS = plugins.o
plugins_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
plugins_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

plugins: $(plugins_OBJS)
	$(CC) -o plugins $(plugins_OBJS) $(plugins_LDFLAGS)

ussd_OBJS = ussd.o
ussd_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
ussd_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

ussd: $(ussd_OBJS)
	$(CC) -o ussd $(ussd_OBJS) $(ussd_LDFLAGS)

tests.log: hayes modems pdu plugins tests.sh ussd
	./tests.sh -P "$(PREFIX)" -- "tests.log"

hayes.o: hayes.c ../src/modems/hayes.o ../config.h
	$(CC) $(hayes_CFLAGS) -c hayes.c

modems.o: modems.c
	$(CC) $(modems_CFLAGS) -c modems.c

oss.o: oss.c ../src/plugins/oss.c
	$(CC) $(oss_CFLAGS) -c oss.c

pdu.o: pdu.c ../src/modems/hayes.o
	$(CC) -I ../src/modems $(pdu_CFLAGS) -c pdu.c

plugins.o: plugins.c
	$(CC) $(plugins_CFLAGS) -c plugins.c

ussd.o: ussd.c ../src/modems/hayes.o
	$(CC) -I ../src/modems $(ussd_CFLAGS) -c ussd.c

clean:
	$(RM) -- $(hayes_OBJS) $(modems_OBJS) $(oss_OBJS) $(pdu_OBJS) $(plugins_OBJS) $(ussd_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
