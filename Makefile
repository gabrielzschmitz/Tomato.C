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
	gcc -lncurses -o tomato tomato.o util.o input.o update.o anim.o

tomato.o: tomato.c util.h input.h update.h anim.h
	gcc -lncurses -c -g tomato.c

util.o: util.c util.h
	gcc -lncurses -c -g util.c

anim.o: anim.c anim.h
	gcc -lncurses -c -g anim.c

input.o: input.c input.h
	gcc -lncurses -c -g input.c

update.o: update.c update.h
	gcc -lncurses -c -g update.c

clean:
	rm -rf tomato tomato.o util.o input.o update.o anim.o

install: tomato
	mkdir -p ${PREFIX}/bin
	cp -f tomato ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/tomato

uninstall:
	rm -rf ${PREFIX}/bin/tomato