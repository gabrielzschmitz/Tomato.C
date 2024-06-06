include config.mk

# Macros for source files and object files
SRCS = tomato.c anim.c draw.c init.c input.c update.c util.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: tomato

clean-all: all
	rm -rf *.o *.d

tomato: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# desktop_file:
# 	mkdir -p ${DESTDIR}${APPPREFIX}
# 	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/icons
# 	sed -i "s|Icon=.*|Icon=${DESTDIR}${PREFIX}/share/tomato/icons/tomato.svg|" tomato.desktop
# 	sudo cp -f icons/tomato.svg ${DESTDIR}${PREFIX}/share/tomato/icons
# 	cp -f tomato.desktop ${DESTDIR}${APPPREFIX}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/
	cp -f tomato ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/tomato/

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/tomato/
	rm -rf ${DESTDIR}${PREFIX}/share/tomato/

clean:
	rm -rf tomato *.o *.d
