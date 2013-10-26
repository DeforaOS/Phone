TARGETS	= modems plugins tests.log
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

modems_OBJS = modems.o
modems_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
modems_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

modems: $(modems_OBJS)
	$(CC) -o modems $(modems_OBJS) $(modems_LDFLAGS)

plugins_OBJS = plugins.o
plugins_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
plugins_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

plugins: $(plugins_OBJS)
	$(CC) -o plugins $(plugins_OBJS) $(plugins_LDFLAGS)

tests.log: modems plugins tests.sh
	./tests.sh -P "$(PREFIX)" -- "tests.log"

modems.o: modems.c
	$(CC) $(modems_CFLAGS) -c modems.c

plugins.o: plugins.c
	$(CC) $(plugins_CFLAGS) -c plugins.c

clean:
	$(RM) -- $(modems_OBJS) $(plugins_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
