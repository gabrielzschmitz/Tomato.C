include config.mk

# Macros for source files and object files
SRCS = $(SRCDIR)/tomato.c $(SRCDIR)/anim.c $(SRCDIR)/bar.c $(SRCDIR)/draw.c $(SRCDIR)/error.c $(SRCDIR)/init.c $(SRCDIR)/input.c $(SRCDIR)/update.c $(SRCDIR)/ui.c $(SRCDIR)/util.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean clean-all install uninstall desktop_file

all: tomato

tomato: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -rf tomato $(SRCDIR)/*.o $(SRCDIR)/*.d

clean-all: all
	rm -rf $(SRCDIR)/*.o $(SRCDIR)/*.d

# desktop_file:
# 	mkdir -p ${DESTDIR}${APPPREFIX}
# 	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/icons
# 	sed -i "s|Icon=.*|Icon=${DESTDIR}${PREFIX}/share/tomato/icons/tomato.svg|" tomato.desktop
# 	sudo cp -f icons/tomato.svg ${DESTDIR}${PREFIX}/share/tomato/icons
# 	cp -f tomato.desktop ${DESTDIR}${APPPREFIX}

# install: all
# 	mkdir -p ${DESTDIR}${PREFIX}/bin
# 	mkdir -p ${DESTDIR}${PREFIX}/share/tomato/
# 	cp -f tomato ${DESTDIR}${PREFIX}/bin
# 	chmod 755 ${DESTDIR}${PREFIX}/bin/tomato/
#
# uninstall:
# 	rm -f ${DESTDIR}${PREFIX}/bin/tomato/
# 	rm -rf ${DESTDIR}${PREFIX}/share/tomato/
