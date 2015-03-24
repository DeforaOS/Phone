SUBDIRS	= 16x16 24x24 32x32 48x48
TARGETS	= $(OBJDIR)blacklist.so $(OBJDIR)debug.so $(OBJDIR)engineering.so $(OBJDIR)gprs.so $(OBJDIR)gps.so $(OBJDIR)locker.so $(OBJDIR)n900.so $(OBJDIR)openmoko.so $(OBJDIR)oss.so $(OBJDIR)panel.so $(OBJDIR)password.so $(OBJDIR)profiles.so $(OBJDIR)smscrypt.so $(OBJDIR)systray.so $(OBJDIR)template.so $(OBJDIR)ussd.so $(OBJDIR)video.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem gtk+-2.0`
CFLAGS	= -Wall -g -O2 -pedantic -fPIC
LDFLAGSF= `pkg-config --cflags libSystem gtk+-2.0`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
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

blacklist_OBJS = $(OBJDIR)blacklist.o
blacklist_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
blacklist_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)blacklist.so: $(blacklist_OBJS)
	$(CCSHARED) -o $(OBJDIR)blacklist.so $(blacklist_OBJS) $(blacklist_LDFLAGS)

debug_OBJS = $(OBJDIR)debug.o
debug_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
debug_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)debug.so: $(debug_OBJS)
	$(CCSHARED) -o $(OBJDIR)debug.so $(debug_OBJS) $(debug_LDFLAGS)

engineering_OBJS = $(OBJDIR)engineering.o
engineering_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
engineering_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)engineering.so: $(engineering_OBJS)
	$(CCSHARED) -o $(OBJDIR)engineering.so $(engineering_OBJS) $(engineering_LDFLAGS)

gprs_OBJS = $(OBJDIR)gprs.o
gprs_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
gprs_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)gprs.so: $(gprs_OBJS)
	$(CCSHARED) -o $(OBJDIR)gprs.so $(gprs_OBJS) $(gprs_LDFLAGS)

gps_OBJS = $(OBJDIR)gps.o
gps_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) -I../modems $(CFLAGSF) $(CFLAGS)
gps_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)gps.so: $(gps_OBJS)
	$(CCSHARED) -o $(OBJDIR)gps.so $(gps_OBJS) $(gps_LDFLAGS)

locker_OBJS = $(OBJDIR)locker.o
locker_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
locker_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)locker.so: $(locker_OBJS)
	$(CCSHARED) -o $(OBJDIR)locker.so $(locker_OBJS) $(locker_LDFLAGS)

n900_OBJS = $(OBJDIR)n900.o
n900_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
n900_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)n900.so: $(n900_OBJS)
	$(CCSHARED) -o $(OBJDIR)n900.so $(n900_OBJS) $(n900_LDFLAGS)

openmoko_OBJS = $(OBJDIR)openmoko.o
openmoko_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) -I../modems $(CFLAGSF) $(CFLAGS) `pkg-config --cflags alsa`
openmoko_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs alsa`

$(OBJDIR)openmoko.so: $(openmoko_OBJS)
	$(CCSHARED) -o $(OBJDIR)openmoko.so $(openmoko_OBJS) $(openmoko_LDFLAGS)

oss_OBJS = $(OBJDIR)oss.o
oss_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
oss_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -lossaudio

$(OBJDIR)oss.so: $(oss_OBJS)
	$(CCSHARED) -o $(OBJDIR)oss.so $(oss_OBJS) $(oss_LDFLAGS)

panel_OBJS = $(OBJDIR)panel.o
panel_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
panel_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

$(OBJDIR)panel.so: $(panel_OBJS)
	$(CCSHARED) -o $(OBJDIR)panel.so $(panel_OBJS) $(panel_LDFLAGS)

password_OBJS = $(OBJDIR)password.o
password_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
password_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)password.so: $(password_OBJS)
	$(CCSHARED) -o $(OBJDIR)password.so $(password_OBJS) $(password_LDFLAGS)

profiles_OBJS = $(OBJDIR)profiles.o
profiles_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
profiles_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)profiles.so: $(profiles_OBJS)
	$(CCSHARED) -o $(OBJDIR)profiles.so $(profiles_OBJS) $(profiles_LDFLAGS)

smscrypt_OBJS = $(OBJDIR)smscrypt.o
smscrypt_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
smscrypt_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)smscrypt.so: $(smscrypt_OBJS)
	$(CCSHARED) -o $(OBJDIR)smscrypt.so $(smscrypt_OBJS) $(smscrypt_LDFLAGS)

systray_OBJS = $(OBJDIR)systray.o
systray_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
systray_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)systray.so: $(systray_OBJS)
	$(CCSHARED) -o $(OBJDIR)systray.so $(systray_OBJS) $(systray_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

ussd_OBJS = $(OBJDIR)ussd.o
ussd_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
ussd_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)ussd.so: $(ussd_OBJS)
	$(CCSHARED) -o $(OBJDIR)ussd.so $(ussd_OBJS) $(ussd_LDFLAGS)

video_OBJS = $(OBJDIR)video.o
video_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
video_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)video.so: $(video_OBJS)
	$(CCSHARED) -o $(OBJDIR)video.so $(video_OBJS) $(video_LDFLAGS)

$(OBJDIR)blacklist.o: blacklist.c ../../include/Phone.h
	$(CC) $(blacklist_CFLAGS) -o $(OBJDIR)blacklist.o -c blacklist.c

$(OBJDIR)debug.o: debug.c ../../include/Phone.h
	$(CC) $(debug_CFLAGS) -o $(OBJDIR)debug.o -c debug.c

$(OBJDIR)engineering.o: engineering.c ../../include/Phone.h
	$(CC) $(engineering_CFLAGS) -o $(OBJDIR)engineering.o -c engineering.c

$(OBJDIR)gprs.o: gprs.c ../../include/Phone.h ../../config.h
	$(CC) $(gprs_CFLAGS) -o $(OBJDIR)gprs.o -c gprs.c

$(OBJDIR)gps.o: gps.c ../../include/Phone.h
	$(CC) $(gps_CFLAGS) -o $(OBJDIR)gps.o -c gps.c

$(OBJDIR)locker.o: locker.c ../../include/Phone.h
	$(CC) $(locker_CFLAGS) -o $(OBJDIR)locker.o -c locker.c

$(OBJDIR)n900.o: n900.c ../../include/Phone.h
	$(CC) $(n900_CFLAGS) -o $(OBJDIR)n900.o -c n900.c

$(OBJDIR)openmoko.o: openmoko.c ../../include/Phone.h
	$(CC) $(openmoko_CFLAGS) -o $(OBJDIR)openmoko.o -c openmoko.c

$(OBJDIR)oss.o: oss.c ../../include/Phone.h ../../config.h
	$(CC) $(oss_CFLAGS) -o $(OBJDIR)oss.o -c oss.c

$(OBJDIR)panel.o: panel.c ../../include/Phone.h
	$(CC) $(panel_CFLAGS) -o $(OBJDIR)panel.o -c panel.c

$(OBJDIR)password.o: password.c ../../include/Phone.h
	$(CC) $(password_CFLAGS) -o $(OBJDIR)password.o -c password.c

$(OBJDIR)profiles.o: profiles.c ../../include/Phone.h
	$(CC) $(profiles_CFLAGS) -o $(OBJDIR)profiles.o -c profiles.c

$(OBJDIR)smscrypt.o: smscrypt.c ../../include/Phone.h
	$(CC) $(smscrypt_CFLAGS) -o $(OBJDIR)smscrypt.o -c smscrypt.c

$(OBJDIR)systray.o: systray.c ../../include/Phone.h ../../config.h
	$(CC) $(systray_CFLAGS) -o $(OBJDIR)systray.o -c systray.c

$(OBJDIR)template.o: template.c ../../include/Phone.h
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

$(OBJDIR)ussd.o: ussd.c ../../include/Phone.h
	$(CC) $(ussd_CFLAGS) -o $(OBJDIR)ussd.o -c ussd.c

$(OBJDIR)video.o: video.c ../../include/Phone.h
	$(CC) $(video_CFLAGS) -o $(OBJDIR)video.o -c video.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(blacklist_OBJS) $(debug_OBJS) $(engineering_OBJS) $(gprs_OBJS) $(gps_OBJS) $(locker_OBJS) $(n900_OBJS) $(openmoko_OBJS) $(oss_OBJS) $(panel_OBJS) $(password_OBJS) $(profiles_OBJS) $(smscrypt_OBJS) $(systray_OBJS) $(template_OBJS) $(ussd_OBJS) $(video_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(blacklist_OBJS) $(debug_OBJS) $(engineering_OBJS) $(gprs_OBJS) $(gps_OBJS) $(locker_OBJS) $(n900_OBJS) $(openmoko_OBJS) $(oss_OBJS) $(panel_OBJS) $(password_OBJS) $(profiles_OBJS) $(smscrypt_OBJS) $(systray_OBJS) $(template_OBJS) $(ussd_OBJS) $(video_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)blacklist.so $(DESTDIR)$(LIBDIR)/Phone/plugins/blacklist.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)debug.so $(DESTDIR)$(LIBDIR)/Phone/plugins/debug.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)engineering.so $(DESTDIR)$(LIBDIR)/Phone/plugins/engineering.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)gprs.so $(DESTDIR)$(LIBDIR)/Phone/plugins/gprs.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)gps.so $(DESTDIR)$(LIBDIR)/Phone/plugins/gps.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)locker.so $(DESTDIR)$(LIBDIR)/Phone/plugins/locker.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)n900.so $(DESTDIR)$(LIBDIR)/Phone/plugins/n900.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)openmoko.so $(DESTDIR)$(LIBDIR)/Phone/plugins/openmoko.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)oss.so $(DESTDIR)$(LIBDIR)/Phone/plugins/oss.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)panel.so $(DESTDIR)$(LIBDIR)/Phone/plugins/panel.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)password.so $(DESTDIR)$(LIBDIR)/Phone/plugins/password.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)profiles.so $(DESTDIR)$(LIBDIR)/Phone/plugins/profiles.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)smscrypt.so $(DESTDIR)$(LIBDIR)/Phone/plugins/smscrypt.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)systray.so $(DESTDIR)$(LIBDIR)/Phone/plugins/systray.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Phone/plugins
	$(INSTALL) -m 0755 $(OBJDIR)ussd.so $(DESTDIR)$(LIBDIR)/Phone/plugins/ussd.so

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/blacklist.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/debug.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/engineering.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/gprs.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/gps.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/locker.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/n900.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/openmoko.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/oss.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/panel.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/password.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/profiles.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/smscrypt.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/systray.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Phone/plugins/ussd.so

.PHONY: all subdirs clean distclean install uninstall
