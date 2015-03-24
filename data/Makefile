SUBDIRS	= 16x16 22x22 24x24 32x32 48x48
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && \
		if [ -n "$(OBJDIR)" ]; then \
		([ -d "$(OBJDIR)$$i" ] || $(MKDIR) -- "$(OBJDIR)$$i") && \
		$(MAKE) OBJDIR="$(OBJDIR)$$i/"; \
		else $(MAKE); fi) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done

install:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 0.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/0.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 1.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/1.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 2.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/2.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 3.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/3.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 4.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/4.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 5.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/5.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 6.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/6.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 7.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/7.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 8.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/8.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 9.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/9.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 a.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/a.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 b.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/b.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 busy.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/busy.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 c.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/c.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 d.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/d.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-contacts.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-contacts.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-dialer.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-dialer.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-log.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-log.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-messages.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-messages.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-settings.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-settings.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-phone-sofia.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-sofia.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 hash.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/hash.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 keytone.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/keytone.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 ringback.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/ringback.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 ringtone.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/ringtone.wav
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/sounds/Phone
	$(INSTALL) -m 0644 star.wav $(DESTDIR)$(PREFIX)/share/sounds/Phone/star.wav

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/0.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/1.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/2.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/3.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/4.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/5.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/6.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/7.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/8.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/9.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/a.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/b.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/busy.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/c.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/d.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-contacts.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-dialer.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-log.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-messages.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-settings.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-phone-sofia.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/hash.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/keytone.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/ringback.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/ringtone.wav
	$(RM) -- $(DESTDIR)$(PREFIX)/share/sounds/Phone/star.wav

.PHONY: all subdirs clean distclean install uninstall
