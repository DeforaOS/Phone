TARGETS	= $(OBJDIR)debug.so $(OBJDIR)hayes.so $(OBJDIR)purple.so $(OBJDIR)sofia.so $(OBJDIR)template.so
OBJDIR	=
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

debug_OBJS = $(OBJDIR)debug.o
debug_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags gtk+-2.0 libSystem`
debug_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs gtk+-2.0 libSystem`

$(OBJDIR)debug.so: $(debug_OBJS)
	$(CCSHARED) -o $(OBJDIR)debug.so $(debug_OBJS) $(debug_LDFLAGS)

hayes_OBJS = $(OBJDIR)hayes/command.o $(OBJDIR)hayes/quirks.o $(OBJDIR)hayes.o
hayes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem`
hayes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

$(OBJDIR)hayes.so: $(hayes_OBJS)
	$(CCSHARED) -o $(OBJDIR)hayes.so $(hayes_OBJS) $(hayes_LDFLAGS)

purple_OBJS = $(OBJDIR)purple.o
purple_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem purple`
purple_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem purple`

$(OBJDIR)purple.so: $(purple_OBJS)
	$(CCSHARED) -o $(OBJDIR)purple.so $(purple_OBJS) $(purple_LDFLAGS)

sofia_OBJS = $(OBJDIR)sofia.o
sofia_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem sofia-sip-ua-glib`
sofia_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem sofia-sip-ua-glib`

$(OBJDIR)sofia.so: $(sofia_OBJS)
	$(CCSHARED) -o $(OBJDIR)sofia.so $(sofia_OBJS) $(sofia_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem`
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

$(OBJDIR)debug.o: debug.c ../../config.h
	$(CC) $(debug_CFLAGS) -o $(OBJDIR)debug.o -c debug.c

$(OBJDIR)hayes/command.o: hayes/command.c hayes/command.h
	@[ -d "$(OBJDIR)hayes" ] || $(MKDIR) -- "$(OBJDIR)hayes"
	$(CC) $(hayes_CFLAGS) -o $(OBJDIR)hayes/command.o -c hayes/command.c

$(OBJDIR)hayes/quirks.o: hayes/quirks.c hayes/quirks.h
	@[ -d "$(OBJDIR)hayes" ] || $(MKDIR) -- "$(OBJDIR)hayes"
	$(CC) $(hayes_CFLAGS) -o $(OBJDIR)hayes/quirks.o -c hayes/quirks.c

$(OBJDIR)hayes.o: hayes.c hayes/command.h hayes/quirks.h hayes.h
	$(CC) $(hayes_CFLAGS) -o $(OBJDIR)hayes.o -c hayes.c

$(OBJDIR)purple.o: purple.c ../../config.h
	$(CC) $(purple_CFLAGS) -o $(OBJDIR)purple.o -c purple.c

$(OBJDIR)sofia.o: sofia.c
	$(CC) $(sofia_CFLAGS) -o $(OBJDIR)sofia.o -c sofia.c

$(OBJDIR)template.o: template.c
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

clean:
	$(RM) -- $(debug_OBJS) $(hayes_OBJS) $(purple_OBJS) $(sofia_OBJS) $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0755 $(OBJDIR)debug.so $(DESTDIR)$(LIBDIR)/Phone/modem/debug.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0755 $(OBJDIR)hayes.so $(DESTDIR)$(LIBDIR)/Phone/modem/hayes.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0755 $(OBJDIR)purple.so $(DESTDIR)$(LIBDIR)/Phone/modem/purple.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/modem
	$(INSTALL) -m 0755 $(OBJDIR)sofia.so $(DESTDIR)$(LIBDIR)/Phone/modem/sofia.so
	$(MKDIR) $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems
	$(INSTALL) -m 0644 hayes.h $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems/hayes.h

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/debug.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/hayes.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/purple.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/modem/sofia.so
	$(RM) -- $(DESTDIR)$(INCLUDEDIR)/Desktop/Phone/modems/hayes.h

.PHONY: all clean distclean install uninstall
