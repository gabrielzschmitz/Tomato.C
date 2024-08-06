# Directories
SRCDIR = src
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
GCCFLAGS = -Wextra -Wno-unused-variable
DFLAGS = -DDATADIR=\"$(DATAPREFIX)\" -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -g -Wall $(GCCFLAGS) $(DFLAGS)

# Debug flag
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    DFLAGS += -DDEBUG_FLAG
    CFLAGS += -g
endif
