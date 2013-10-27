SUBDIRS	= 16x16 22x22 24x24 32x32 48x48
PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

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
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-contacts.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-contacts.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-dialer.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-dialer.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-log.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-log.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-messages.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-messages.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-settings.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-settings.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 phone-sofia.desktop $(DESTDIR)$(PREFIX)/share/applications/phone-sofia.desktop

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
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-contacts.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-dialer.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-log.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-messages.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-settings.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/phone-sofia.desktop

.PHONY: all subdirs clean distclean install uninstall
