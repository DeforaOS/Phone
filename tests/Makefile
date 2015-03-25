TARGETS	= $(OBJDIR)hayes $(OBJDIR)modems $(OBJDIR)oss $(OBJDIR)pdu $(OBJDIR)plugins $(OBJDIR)ussd $(OBJDIR)tests.log
OBJDIR	=
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

hayes_OBJS = $(OBJDIR)hayes.o
hayes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
hayes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

$(OBJDIR)hayes: $(hayes_OBJS)
	$(CC) -o $(OBJDIR)hayes $(hayes_OBJS) $(hayes_LDFLAGS)

modems_OBJS = $(OBJDIR)modems.o
modems_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
modems_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

$(OBJDIR)modems: $(modems_OBJS)
	$(CC) -o $(OBJDIR)modems $(modems_OBJS) $(modems_LDFLAGS)

oss_OBJS = $(OBJDIR)oss.o
oss_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
oss_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop` -lossaudio

$(OBJDIR)oss: $(oss_OBJS)
	$(CC) -o $(OBJDIR)oss $(oss_OBJS) $(oss_LDFLAGS)

pdu_OBJS = $(OBJDIR)pdu.o
pdu_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
pdu_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

$(OBJDIR)pdu: $(pdu_OBJS)
	$(CC) -o $(OBJDIR)pdu $(pdu_OBJS) $(pdu_LDFLAGS)

plugins_OBJS = $(OBJDIR)plugins.o
plugins_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
plugins_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

$(OBJDIR)plugins: $(plugins_OBJS)
	$(CC) -o $(OBJDIR)plugins $(plugins_OBJS) $(plugins_LDFLAGS)

ussd_OBJS = $(OBJDIR)ussd.o
ussd_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem`
ussd_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem`

$(OBJDIR)ussd: $(ussd_OBJS)
	$(CC) -o $(OBJDIR)ussd $(ussd_OBJS) $(ussd_LDFLAGS)

$(OBJDIR)tests.log: $(OBJDIR)hayes $(OBJDIR)modems $(OBJDIR)pdu $(OBJDIR)plugins tests.sh $(OBJDIR)ussd
	./tests.sh -P "$(PREFIX)" -- "$(OBJDIR)tests.log"

$(OBJDIR)hayes.o: hayes.c $(OBJDIR)../src/modems/hayes.o ../config.h
	$(CC) $(hayes_CFLAGS) -o $(OBJDIR)hayes.o -c hayes.c

$(OBJDIR)modems.o: modems.c
	$(CC) $(modems_CFLAGS) -o $(OBJDIR)modems.o -c modems.c

$(OBJDIR)oss.o: oss.c ../src/plugins/oss.c
	$(CC) $(oss_CFLAGS) -o $(OBJDIR)oss.o -c oss.c

$(OBJDIR)pdu.o: pdu.c $(OBJDIR)../src/modems/hayes.o
	$(CC) -I ../src/modems $(pdu_CFLAGS) -o $(OBJDIR)pdu.o -c pdu.c

$(OBJDIR)plugins.o: plugins.c
	$(CC) $(plugins_CFLAGS) -o $(OBJDIR)plugins.o -c plugins.c

$(OBJDIR)ussd.o: ussd.c $(OBJDIR)../src/modems/hayes.o
	$(CC) -I ../src/modems $(ussd_CFLAGS) -o $(OBJDIR)ussd.o -c ussd.c

clean:
	$(RM) -- $(hayes_OBJS) $(modems_OBJS) $(oss_OBJS) $(pdu_OBJS) $(plugins_OBJS) $(ussd_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
