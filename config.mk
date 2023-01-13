#         .             .              .		  
#         |             |              |           .	  
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
#  ,|							  
#  `'							  
# config.mk

PREFIX  = /usr/local
APPPREFIX  = $(PREFIX)/share/applications
CPPFLAGS = -I/usr/local/include
CFLAGS  = -Wall -Wextra -pedantic -Os
LDFLAGS = -L/usr/local/lib
ifdef __APPLE__
LDLIBS  = -lncurses
else
LDLIBS  = `pkg-config --libs ncursesw`
endif

