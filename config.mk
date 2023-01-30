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
LOGPREFIX= .local/share/tomato

DFLAGS = -DLOGPREFIX=\"$(LOGPREFIX)\" -DLOGFILE=\"$(LOGPREFIX)/tomato.log\" -DTMPFILE=\"$(LOGPREFIX)/tmp.log\" -DTIMERFILE=\"$(LOGPREFIX)/time.log\"
CPPFLAGS = -I/usr/local/include
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -Wunused-result -Wno-unused-variable -Os ${DFLAGS}
LDFLAGS = -L/usr/local/lib

ifdef __APPLE__
LDLIBS  = -lncurses
else
LDLIBS  = `pkg-config --libs ncursesw`
endif

