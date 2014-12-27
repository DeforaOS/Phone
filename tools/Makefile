TARGETS	= $(OBJDIR)engineering $(OBJDIR)gprs $(OBJDIR)pdu $(OBJDIR)smscrypt
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
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

engineering_OBJS = $(OBJDIR)engineering.o
engineering_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem gtk+-2.0`
engineering_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem gtk+-2.0`

$(OBJDIR)engineering: $(engineering_OBJS)
	$(CC) -o $(OBJDIR)engineering $(engineering_OBJS) $(engineering_LDFLAGS)

gprs_OBJS = $(OBJDIR)gprs.o
gprs_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
gprs_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

$(OBJDIR)gprs: $(gprs_OBJS)
	$(CC) -o $(OBJDIR)gprs $(gprs_OBJS) $(gprs_LDFLAGS)

pdu_OBJS = $(OBJDIR)pdu.o
pdu_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem glib-2.0`
pdu_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem glib-2.0`

$(OBJDIR)pdu: $(pdu_OBJS)
	$(CC) -o $(OBJDIR)pdu $(pdu_OBJS) $(pdu_LDFLAGS)

smscrypt_OBJS = $(OBJDIR)smscrypt.o
smscrypt_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem gtk+-2.0`
smscrypt_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem gtk+-2.0` -lssl

$(OBJDIR)smscrypt: $(smscrypt_OBJS)
	$(CC) -o $(OBJDIR)smscrypt $(smscrypt_OBJS) $(smscrypt_LDFLAGS)

$(OBJDIR)engineering.o: engineering.c ../include/Phone.h ../src/plugins/engineering.c common.c
	$(CC) $(engineering_CFLAGS) -o $(OBJDIR)engineering.o -c engineering.c

$(OBJDIR)gprs.o: gprs.c ../include/Phone.h ../src/plugins/gprs.c common.c
	$(CC) $(gprs_CFLAGS) -o $(OBJDIR)gprs.o -c gprs.c

$(OBJDIR)pdu.o: pdu.c ../include/Phone.h ../src/modems/hayes.c
	$(CC) -I../src/modems $(pdu_CFLAGS) -o $(OBJDIR)pdu.o -c pdu.c

$(OBJDIR)smscrypt.o: smscrypt.c ../include/Phone.h ../src/plugins/smscrypt.c
	$(CC) $(smscrypt_CFLAGS) -o $(OBJDIR)smscrypt.o -c smscrypt.c

clean:
	$(RM) -- $(engineering_OBJS) $(gprs_OBJS) $(pdu_OBJS) $(smscrypt_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)gprs $(DESTDIR)$(BINDIR)/gprs

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/gprs

.PHONY: all clean distclean install uninstall
