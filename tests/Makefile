TARGETS	= hayes modems pdu plugins ussd tests.log
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= -W `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

hayes_OBJS = hayes.o
hayes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
hayes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

hayes: $(hayes_OBJS) ../src/modems/hayes.c
	$(CC) -o hayes $(hayes_OBJS) $(hayes_LDFLAGS)

modems_OBJS = modems.o
modems_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
modems_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

modems: $(modems_OBJS)
	$(CC) -o modems $(modems_OBJS) $(modems_LDFLAGS)

pdu_OBJS = pdu.o
pdu_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
pdu_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

pdu: $(pdu_OBJS)
	$(CC) -o pdu $(pdu_OBJS) $(pdu_LDFLAGS)

plugins_OBJS = plugins.o
plugins_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
plugins_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

plugins: $(plugins_OBJS)
	$(CC) -o plugins $(plugins_OBJS) $(plugins_LDFLAGS)

ussd_OBJS = ussd.o
ussd_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
ussd_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

ussd: $(ussd_OBJS)
	$(CC) -o ussd $(ussd_OBJS) $(ussd_LDFLAGS)

tests.log: hayes modems pdu plugins tests.sh ussd
	./tests.sh -P "$(PREFIX)" -- "tests.log"

hayes.o: hayes.c
	$(CC) $(hayes_CFLAGS) -c hayes.c

modems.o: modems.c
	$(CC) $(modems_CFLAGS) -c modems.c

pdu.o: pdu.c ../src/modems/hayes.c ../src/modems/hayes/command.c ../src/modems/hayes/command.h
	$(CC) -I ../src/modems $(pdu_CFLAGS) -c pdu.c

plugins.o: plugins.c
	$(CC) $(plugins_CFLAGS) -c plugins.c

ussd.o: ussd.c ../src/modems/hayes.c ../src/modems/hayes/command.c ../src/modems/hayes/command.h
	$(CC) -I ../src/modems $(ussd_CFLAGS) -c ussd.c

clean:
	$(RM) -- $(hayes_OBJS) $(modems_OBJS) $(pdu_OBJS) $(plugins_OBJS) $(ussd_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
