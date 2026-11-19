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

SRCS = $(SRCDIR)/app/tomato.c \
       $(SRCDIR)/features/animation/anim.c \
       $(SRCDIR)/features/audio/audio.c \
       $(SRCDIR)/features/bar/bar.c \
       $(SRCDIR)/core/config.c \
       $(SRCDIR)/app/draw.c \
       $(SRCDIR)/core/error.c \
       $(SRCDIR)/input/gap_buffer.c \
       $(SRCDIR)/input/history.c \
       $(SRCDIR)/app/init.c \
       $(SRCDIR)/input/input.c \
       $(SRCDIR)/core/log.c \
       $(SRCDIR)/features/notes/notes.c \
       $(SRCDIR)/features/notifications/notify.c \
       $(SRCDIR)/features/pomodoro/pomodoro.c \
       $(SRCDIR)/app/ui/ui.c \
       $(SRCDIR)/app/update.c \
       $(SRCDIR)/core/util.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

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
	rm -rf tomato $(OBJS) $(DEPS)

clean-all: all
	rm -f $(OBJS) $(DEPS)
