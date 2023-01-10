#         .             .              .		  
#         |             |              |           .	  
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
#  ,|							  
#  `'							  
#  Makefile

PREFIX = /usr/local

tomato: tomato.o util.o input.o update.o anim.o
	gcc -o tomato tomato.o util.o input.o update.o anim.o -lncurses

tomato.o: tomato.c util.h input.h update.h anim.h
	gcc -c -g tomato.c -lncurses

util.o: util.c util.h
	gcc -c -g util.c -lncurses

anim.o: anim.c anim.h
	gcc -c -g anim.c -lncurses

input.o: input.c input.h
	gcc -c -g input.c -lncurses

update.o: update.c update.h
	gcc -c -g update.c -lncurses

clean:
	rm -rf tomato tomato.o util.o input.o update.o anim.o

install: tomato
	mkdir -p ${PREFIX}/bin
	cp -f tomato ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/tomato

uninstall:
	rm -f ${PREFIX}/bin/tomato

