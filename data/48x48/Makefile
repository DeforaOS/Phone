PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 0644 phone-gprs.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-gprs.png
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 0644 phone-inbox.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-inbox.png
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 0644 phone-sent.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-sent.png

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-gprs.png
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-inbox.png
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/phone-sent.png

.PHONY: all clean distclean install uninstall
