#         .             .              .		  
#         |             |              |           .	  
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
#  ,|							  
#  `'							  
# Makefile

include config.mk

all: tomato tomatonoise

tomatonoise: tomatonoise.c

tomato: tomato.o anim.o draw.o input.o notify.o update.o util.o

tomato.o: tomato.c tomato.h anim.c draw.c input.c notify.c update.c util.c config.h

anim.o: anim.h

draw.o: draw.h

input.o: input.h

notify.o: notify.h

update.o: update.h

util.o: util.h

clean:
	rm -rf tomato tomatonoise *.o

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tomato ${DESTDIR}${PREFIX}/bin
	cp -f tomatonoise ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${APPPREFIX}
	cp -f tomato.desktop ${DESTDIR}${APPPREFIX}
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/sounds
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/icons
	cp -rf sounds ${DESTDIR}${PREFIX}/share/tomato/
	sed -i "s|Icon=.*|Icon=${DESTDIR}${PREFIX}/share/tomato/icons/tomato.svg|" tomato.desktop
	sudo cp -f icons/tomato.svg ${DESTDIR}${PREFIX}/share/tomato/icons
	chmod 755 ${DESTDIR}${PREFIX}/bin/tomato
	chmod 755 ${DESTDIR}${PREFIX}/bin/tomatonoise

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/tomato
	rm -f ${DESTDIR}${PREFIX}/bin/tomatonoise
	rm -rf ${DESTDIR}${PREFIX}/share/tomato
	rm -f ${DESTDIR}${APPPREFIX}/tomato.desktop

