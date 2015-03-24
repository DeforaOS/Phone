SUBDIRS	= modems plugins
TARGETS	= $(OBJDIR)phone $(OBJDIR)phonectl
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && \
		if [ -n "$(OBJDIR)" ]; then \
		([ -d "$(OBJDIR)$$i" ] || $(MKDIR) -- "$(OBJDIR)$$i") && \
		$(MAKE) OBJDIR="$(OBJDIR)$$i/"; \
		else $(MAKE); fi) || exit; done

phone_OBJS = $(OBJDIR)callbacks.o $(OBJDIR)main.o $(OBJDIR)modem.o $(OBJDIR)phone.o
phone_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
phone_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)phone: $(phone_OBJS)
	$(CC) -o $(OBJDIR)phone $(phone_OBJS) $(phone_LDFLAGS)

phonectl_OBJS = $(OBJDIR)phonectl.o
phonectl_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
phonectl_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)phonectl: $(phonectl_OBJS)
	$(CC) -o $(OBJDIR)phonectl $(phonectl_OBJS) $(phonectl_LDFLAGS)

$(OBJDIR)callbacks.o: callbacks.c ../include/Phone/phone.h phone.h callbacks.h
	$(CC) $(phone_CFLAGS) -o $(OBJDIR)callbacks.o -c callbacks.c

$(OBJDIR)main.o: main.c ../include/Phone/phone.h phone.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(phone_CFLAGS) -o $(OBJDIR)main.o -c main.c

$(OBJDIR)modem.o: modem.c modem.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(phone_CFLAGS) -o $(OBJDIR)modem.o -c modem.c

$(OBJDIR)phone.o: phone.c ../include/Phone/phone.h modem.h phone.h callbacks.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(phone_CFLAGS) -o $(OBJDIR)phone.o -c phone.c

$(OBJDIR)phonectl.o: phonectl.c ../include/Phone/phone.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(phonectl_CFLAGS) -o $(OBJDIR)phonectl.o -c phonectl.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(phone_OBJS) $(phonectl_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(phone_OBJS) $(phonectl_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)phone $(DESTDIR)$(BINDIR)/phone
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)phonectl $(DESTDIR)$(BINDIR)/phonectl

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/phone
	$(RM) -- $(DESTDIR)$(BINDIR)/phonectl

.PHONY: all subdirs clean distclean install uninstall
