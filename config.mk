#         .             .              .		  
#         |             |              |           .	  
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_, 
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /  
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"' 
#  ,|							  
#  `'							  
# config.mk

PREFIX  = /usr/local
CPPFLAGS = -I/usr/local/include
CFLAGS  = -Wall -Wextra -pedantic -Os
LDFLAGS = -L/usr/local/lib
LDLIBS  = -lncurses

