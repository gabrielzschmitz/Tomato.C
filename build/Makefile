#/**
# * @file build/Makefile
# * @brief Top-level build rules for the Tomato.C application.
# *
# * Compiles all sources under src/ and links the tomato binary.
# * Supports clean, clean-all, and commented-out install targets.
# *
# * Targets:
# *   all        — build the tomato binary (default)
# *   clean      — remove tomato and all .o / .d files
# *   clean-all  — same as clean (signals a build is needed first)
# */

include config.mk

#/**
# * ---------------------------------------------------------------------------
# * Source and object files
# * ---------------------------------------------------------------------------
# */

SRCS = $(SRCDIR)/tomato.c $(SRCDIR)/anim.c $(SRCDIR)/audio.c $(SRCDIR)/bar.c \
       $(SRCDIR)/config.c $(SRCDIR)/draw.c $(SRCDIR)/error.c \
       $(SRCDIR)/gap_buffer.c $(SRCDIR)/history.c $(SRCDIR)/init.c \
       $(SRCDIR)/input.c $(SRCDIR)/log.c $(SRCDIR)/notes.c \
       $(SRCDIR)/notify.c $(SRCDIR)/ui.c $(SRCDIR)/update.c $(SRCDIR)/util.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean clean-all

#/**
# * ---------------------------------------------------------------------------
# * Targets
# * ---------------------------------------------------------------------------
# */

#/** @brief Build the tomato binary */
all: tomato

tomato: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

#/** @brief Remove all build artifacts */
clean:
	rm -rf tomato $(SRCDIR)/*.o $(SRCDIR)/*.d

clean-all: all
	rm -rf $(SRCDIR)/*.o $(SRCDIR)/*.d
