#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# config.mk

OS := $(shell uname)

ifeq ($(OS),Darwin)
    PREFIX  = /opt/local
    CPPFLAGS = -I/opt/local/include
    LDFLAGS = -L/opt/local/lib
else
    PREFIX  = /usr/local
    CPPFLAGS = -I/usr/local/include
    LDFLAGS = -L/usr/local/lib
endif
APPPREFIX = $(PREFIX)/share/applications
LOGPREFIX = .local/share/tomato

# To remove mpv as a dependencie
# comment this line below,
# and toggle off the sound and noise
# at config.h
MPVTOGGLE = 1

DFLAGS = -D_ISOC99_SOURCE -DTOMATONOISE=\"$(PREFIX)/bin/tomatonoise\" -DLOGPREFIX=\"$(LOGPREFIX)\" -DLOGFILE=\"$(LOGPREFIX)/tomato.log\" -DTMPFILE=\"$(LOGPREFIX)/tmp.log\" -DTIMERFILE=\"$(LOGPREFIX)/time.log\" -DNOTEPADFILE=\"$(LOGPREFIX)/notepad.log\"
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -Wunused-result -Wno-unused-variable -Os ${DFLAGS}

ifeq ($(OS),Darwin)
    ifdef MPVTOGGLE
        LDLIBS  = -lncurses -lmpv
    else
        LDLIBS  = -lncurses
    endif
else
    ifdef MPVTOGGLE
        LDLIBS  = `pkg-config --libs ncursesw mpv`
    else
        LDLIBS  = `pkg-config --libs ncursesw`
    endif
endif
