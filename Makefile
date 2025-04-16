CFLAGS = -Wall -Wextra -pedantic -I/usr/include/freetype2
LDFLAGS = -lX11 -lXft -lfontconfig -lpthread -lrt

PREFIX ?= /usr/local
CC ?= cc

all: herbe

config.h: config.def.h
	cp config.def.h config.h

herbe: herbe.c herbe.h fontutil.c fontutil.h config.h Makefile
	$(CC) $(CFLAGS) herbe.c fontutil.c -o herbe $(LDFLAGS)

install: herbe
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f herbe ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/herbe

clean:
	rm -f herbe *.rej *.orig

.PHONY: all install uninstall clean
