# Directories
PREFIX = /usr/local
APPPREFIX = $(PREFIX)/share/applications
DATAPREFIX = .local/share/tomato
INCLUDES = -I$(PREFIX)/include
LIBDIR = -L$(PREFIX)/lib

# Combine flags
CPPFLAGS += $(INCLUDES) $(DEPFLAGS)
LDFLAGS += $(LIBDIR)
ifdef __APPLE__
LDLIBS = -lncurses -lm
else
LDLIBS = $(shell pkg-config --libs ncursesw) -lm
endif

# Dependency flags
DEPFLAGS = -MD

# Compiler and flags
CC = gcc
TCCFLAGS = -Wwrite-strings
GCCFLAGS = -Wextra
DFLAGS = -DDATADIR=\"$(DATAPREFIX)\" -D_POSIX_C_SOURCE=199309L
CFLAGS = -std=c99 -g $(GCCFLAGS) -Wall -pedantic -O3 $(DFLAGS)
