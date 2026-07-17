#/**
# * @file build/config.mk
# * @brief Shared compiler and linker configuration for the Tomato.C build.
# *
# * Sets up include paths, library flags, compiler warnings, dependency
# * tracking, security hardening, and platform-specific options
# * (macOS vs Linux).  Included by build/Makefile.
# *
# * Overridable variables:
# *   PREFIX  — installation prefix (default /usr/local)
# *   DEBUG   — set to 1 to add -DDEBUG_FLAG and extra -g
# */

#/**
# * ---------------------------------------------------------------------------
# * Directories
# * ---------------------------------------------------------------------------
# */

SRCDIR = ../src
PREFIX = /usr/local
APPPREFIX = $(PREFIX)/share/applications
DATAPREFIX = ./resources
INCLUDES = -I$(PREFIX)/include
LIBDIR = -L$(PREFIX)/lib

#/**
# * ---------------------------------------------------------------------------
# * Combined flags
# * ---------------------------------------------------------------------------
# */

CPPFLAGS += $(INCLUDES) $(DEPFLAGS)
LDFLAGS += $(LIBDIR)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
LDLIBS = -lncurses -lpthread -lm
else
LDLIBS = $(shell pkg-config --libs ncursesw libnotify) -lpthread -lm
endif

#/**
# * ---------------------------------------------------------------------------
# * Dependency tracking
# * ---------------------------------------------------------------------------
# */

DEPFLAGS = -MD

#/**
# * ---------------------------------------------------------------------------
# * Compiler and flags
# * ---------------------------------------------------------------------------
# */

CC = gcc
TCCFLAGS = -Wwrite-strings
GCCFLAGS = -Wextra -Wno-unused-variable
DFLAGS = -DDATADIR=\"$(DATAPREFIX)\" -D_POSIX_C_SOURCE=200809L
ifeq ($(UNAME), Darwin)
CFLAGS = -std=c99 -g -O1 -Wall $(GCCFLAGS) $(DFLAGS)
else
CFLAGS = -std=c99 -g -O1 -Wall $(GCCFLAGS) $(DFLAGS) $(shell pkg-config --cflags libnotify)
endif

#/**
# * ---------------------------------------------------------------------------
# * Debug flag
# * ---------------------------------------------------------------------------
# */

DEBUG :Wall?= 0
ifeq ($(DEBUG), 1)
    DFLAGS += -DDEBUG_FLAG
    CFLAGS += -g
endif

#/**
# * ---------------------------------------------------------------------------
# * Security hardening
# * ---------------------------------------------------------------------------
# */

HARDEN_CFLAGS  = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE
ifneq ($(UNAME), Darwin)
HARDEN_LDFLAGS = -pie -Wl,-z,relro,-z,now -z noexecstack
endif

CFLAGS  += $(HARDEN_CFLAGS)
LDFLAGS += $(HARDEN_LDFLAGS)
