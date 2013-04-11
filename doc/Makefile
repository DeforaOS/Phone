TARGETS	= index.html phone.1 phone.html phonectl.1 phonectl.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

index.html: index.xml index.xsl phone.xml phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "index.html"

phone.1: phone.xml
	./docbook.sh -P "$(PREFIX)" -- "phone.1"

phone.html: phone.xml
	./docbook.sh -P "$(PREFIX)" -- "phone.html"

phonectl.1: phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "phonectl.1"

phonectl.html: phonectl.xml
	./docbook.sh -P "$(PREFIX)" -- "phonectl.html"

clean:
	$(RM) -- $(index.html_OBJS) $(phone.1_OBJS) $(phone.html_OBJS) $(phonectl.1_OBJS) $(phonectl.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "phonectl.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phone.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phone.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phonectl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "phonectl.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phone.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phone.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phonectl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "phonectl.html"

.PHONY: all clean distclean install uninstall
