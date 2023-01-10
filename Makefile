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

tomato.o: tomato.c util.h input.h update.h anim.h

util.o: util.h

anim.o: anim.h

input.o: input.h

update.o: update.h

clean:
	rm -rf tomato *.o

install: tomato
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tomato ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/tomato

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/tomato

