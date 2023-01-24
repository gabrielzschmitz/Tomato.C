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
	cp -f tomato.log ${DESTDIR}${LOGPREFIX}

install: tomato
	sudo mkdir -p ${DESTDIR}${PREFIX}/bin
	sudo cp -f tomato ${DESTDIR}${PREFIX}/bin
	sudo mkdir -p ${DESTDIR}${APPPREFIX}
	sudo cp -f tomato.desktop ${DESTDIR}${APPPREFIX}
	mkdir -p ${DESTDIR}${LOGPREFIX}
	sudo mkdir -p ${DESTDIR}${PREFIX}/share/tomato
	sudo mkdir -p ${DESTDIR}${PREFIX}/share/tomato/sounds
	sudo mkdir -p ${DESTDIR}${PREFIX}/share/tomato/icons
	cp -n tomato.log ${DESTDIR}${LOGPREFIX}
	sudo cp -f sounds/dfltnotify.mp3 sounds/pausenotify.mp3 ${DESTDIR}${PREFIX}/share/tomato/sounds
	sudo cp -f icons/tomato.svg ${DESTDIR}${PREFIX}/share/tomato/icons
	chmod 666 ${DESTDIR}${LOGPREFIX}/tomato.log
	sudo chmod 755 ${DESTDIR}${PREFIX}/bin/tomato

uninstall:
	sudo rm -f ${DESTDIR}${PREFIX}/bin/tomato
	sudo rm -rf ${DESTDIR}${LOGPREFIX}
	sudo rm -rf ${DESTDIR}${PREFIX}/share/tomato
	sudo rm -f ${DESTDIR}${APPPREFIX}/tomato.desktop

