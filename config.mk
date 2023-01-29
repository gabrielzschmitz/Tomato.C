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

CPPFLAGS = -I/usr/local/include
CFLAGS  = -Wall -Wextra -pedantic -Wno-unused-result -Wno-unused-variable -Os -DLOGPREFIX=\"$(LOGPREFIX)\" -DLOGFILE=\"$(LOGPREFIX)/tomato.log\" -DTMPFILE=\"$(LOGPREFIX)/tmp.log\" -DTIMERFILE=\"$(LOGPREFIX)/time.log\"
LDFLAGS = -L/usr/local/lib

ifdef __APPLE__
LDLIBS  = -lncurses
else
LDLIBS  = `pkg-config --libs ncursesw`
endif

