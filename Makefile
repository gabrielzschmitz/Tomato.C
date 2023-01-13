#         .             .              .		  
#         |             |              |           .	  
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
#  ,|							  
#  `'							  
#  Makefile

include config.mk

tomato: tomato.o util.o input.o update.o anim.o

tomato.o: tomato.c util.h input.h update.h anim.h config.h

util.o: util.h

anim.o: anim.h

input.o: input.h

update.o: update.h

clean:
	rm -rf tomato *.o

install: tomato
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tomato ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${APPPREFIX}
	cp -f tomato.desktop ${DESTDIR}${APPPREFIX}
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/sounds
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/icons
	cp -f sounds/dfltnotify.mp3 sounds/pausenotify.mp3 ${DESTDIR}${PREFIX}/share/tomato/sounds
	cp -f icons/tomato.svg ${DESTDIR}${PREFIX}/share/tomato/icons
	chmod 755 ${DESTDIR}${PREFIX}/bin/tomato

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/tomato
	rm -rf ${DESTDIR}${PREFIX}/share/tomato
	rm -rf ${DESTDIR}${APPPREFIX}/tomato.desktop

