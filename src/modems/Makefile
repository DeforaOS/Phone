TARGETS	= debug.so hayes.so purple.so sofia.so template.so
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags glib-2.0`
CFLAGS	= -Wall -g -O2 -pedantic -fPIC
LDFLAGSF= `pkg-config --libs glib-2.0`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
INCLUDEDIR= $(PREFIX)/include


all: $(TARGETS)

debug_OBJS = debug.o
debug_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags gtk+-2.0 libSystem`
debug_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs gtk+-2.0 libSystem`

debug.so: $(debug_OBJS)
	$(CCSHARED) -o debug.so $(debug_OBJS) $(debug_LDFLAGS)

hayes_OBJS = hayes/command.o hayes/quirks.o hayes.o
hayes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem`
hayes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

hayes.so: $(hayes_OBJS)
	$(CCSHARED) -o hayes.so $(hayes_OBJS) $(hayes_LDFLAGS)

purple_OBJS = purple.o
purple_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem purple`
purple_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem purple`

purple.so: $(purple_OBJS)
	$(CCSHARED) -o purple.so $(purple_OBJS) $(purple_LDFLAGS)

sofia_OBJS = sofia.o
sofia_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem sofia-sip-ua-glib`
sofia_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem sofia-sip-ua-glib`

sofia.so: $(sofia_OBJS)
	$(CCSHARED) -o sofia.so $(sofia_OBJS) $(sofia_LDFLAGS)

template_OBJS = template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem`
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

template.so: $(template_OBJS)
	$(CCSHARED) -o template.so $(template_OBJS) $(template_LDFLAGS)

debug.o: debug.c ../../config.h
	$(CC) $(debug_CFLAGS) -c debug.c

hayes/command.o: hayes/command.c hayes/command.h
	$(CC) $(hayes_CFLAGS) -o hayes/command.o -c hayes/command.c

hayes/quirks.o: hayes/quirks.c hayes/quirks.h
	$(CC) $(hayes_CFLAGS) -o hayes/quirks.o -c hayes/quirks.c

hayes.o: hayes.c hayes/command.h hayes/quirks.h hayes.h
	$(CC) $(hayes_CFLAGS) -c hayes.c

purple.o: purple.c ../../config.h
	$(CC) $(purple_CFLAGS) -c purple.c

sofia.o: sofia.c
	$(CC) $(sofia_CFLAGS) -c sofia.c

template.o: template.c
	$(CC) $(template_CFLAGS) -c template.c

clean:
	$(RM) -- $(debug_OBJS) $(hayes_OBJS) $(purple_OBJS) $(sofia_OBJS) $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0644 debug.so $(DESTDIR)$(LIBDIR)/Phone/modem/debug.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0644 hayes.so $(DESTDIR)$(LIBDIR)/Phone/modem/hayes.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0644 purple.so $(DESTDIR)$(LIBDIR)/Phone/modem/purple.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0644 sofia.so $(DESTDIR)$(LIBDIR)/Phone/modem/sofia.so
	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems
	$(INSTALL) -m 0644 hayes.h $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems/hayes.h

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/debug.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/hayes.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/purple.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/sofia.so
	$(RM) -- $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems/hayes.h

.PHONY: all clean distclean install uninstall
