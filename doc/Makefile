MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f
TARGETS	= gprs.1 gprs.html index.html phone.1 phone.html phonectl.1 phonectl.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

gprs.1: gprs.xml
	./docbook.sh -P "$(PREFIX)" -- "gprs.1"

gprs.html: manual.css.xml gprs.css.xml gprs.xml
	./docbook.sh -P "$(PREFIX)" -- "gprs.html"

index.html: gprs.xml index.xml index.xsl phone.xml phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "index.html"

phone.1: phone.xml
	./docbook.sh -P "$(PREFIX)" -- "phone.1"

phone.html: manual.css.xml phone.css.xml phone.xml
	./docbook.sh -P "$(PREFIX)" -- "phone.html"

phonectl.1: phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "phonectl.1"

phonectl.html: manual.css.xml phonectl.css.xml phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "phonectl.html"

clean:
	$(RM) -- $(gprs.1_OBJS) $(gprs.html_OBJS) $(index.html_OBJS) $(phone.1_OBJS) $(phone.html_OBJS) $(phonectl.1_OBJS) $(phonectl.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "gprs.1"
	./docbook.sh -c -P "$(PREFIX)" -- "gprs.html"
	./docbook.sh -c -P "$(PREFIX)" -- "index.html"
	./docbook.sh -c -P "$(PREFIX)" -- "phone.1"
	./docbook.sh -c -P "$(PREFIX)" -- "phone.html"
	./docbook.sh -c -P "$(PREFIX)" -- "phonectl.1"
	./docbook.sh -c -P "$(PREFIX)" -- "phonectl.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "gprs.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "gprs.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phone.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phone.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phonectl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phonectl.html"
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/doc/Phone
	$(INSTALL) -m 0644 gprs.conf $(DESTDIR)$(PREFIX)/share/doc/Phone/gprs.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/doc/Phone
	$(INSTALL) -m 0644 phone.conf $(DESTDIR)$(PREFIX)/share/doc/Phone/phone.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/doc/Phone
	$(INSTALL) -m 0644 pppd-chat_gprs $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-chat_gprs
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/doc/Phone
	$(INSTALL) -m 0644 pppd-peers_gprs $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-peers_gprs
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/doc/Phone
	$(INSTALL) -m 0644 pppd-peers_phone $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-peers_phone

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gprs.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gprs.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phone.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phone.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phonectl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phonectl.html"
	$(RM) -- $(DESTDIR)$(PREFIX)/share/doc/Phone/gprs.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/doc/Phone/phone.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-chat_gprs
	$(RM) -- $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-peers_gprs
	$(RM) -- $(DESTDIR)$(PREFIX)/share/doc/Phone/pppd-peers_phone

.PHONY: all clean distclean install uninstall
