PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps
	$(INSTALL) -m 0644 phone-gprs.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps/phone-gprs.png

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps/phone-gprs.png

.PHONY: all clean distclean install uninstall
