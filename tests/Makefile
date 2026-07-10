#/**
# * @file Makefile
# * @brief Build rules for the Tomato.C test suites.
# *
# * Builds each unit/integration test binary by linking the relevant
# * source modules against the test-helpers header.  Produces
# * standalone executables under unit/ and integration/ that are
# * discovered and run by run_tests.sh.
# *
# * Targets:
# *   all / build          — build-unit + build-integration
# *   build-unit           — all unit test binaries
# *   build-integration    — all integration test binaries
# *   clean                — remove all test binaries
# */

SRCDIR = ../src
TEST_CC = gcc
BASE_CFLAGS = -std=c99 -g -O0 -Wall -Wextra -Wno-unused-variable -Wno-unused-function \
              -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 \
              -I$(SRCDIR) -I$(SRCDIR)/external -Ihelpers -DUNIT_TEST
NCURSES_CFLAGS = $(shell pkg-config --cflags ncursesw 2>/dev/null || echo "")
NCURSES_LIBS = $(shell pkg-config --libs ncursesw 2>/dev/null || echo "-lncurses -ltinfo")
TEST_LDLIBS = -lm

TEST_CFLAGS = $(BASE_CFLAGS) $(NCURSES_CFLAGS)

HELPERS = helpers/test_helpers.h

#/**
# * ---------------------------------------------------------------------------
# * Unit test binaries
# * ---------------------------------------------------------------------------
# */

UNIT_BINS = \
	unit/test_gap_buffer \
	unit/test_history \
	unit/test_config \
	unit/test_notes \
	unit/test_util \
	unit/test_error \
	unit/test_log \
	unit/test_anim \
	unit/test_bar \


all: build

build: build-unit build-integration

build-unit: $(UNIT_BINS)

#/** @brief gap_buffer — stubs LogError in the test file */
unit/test_gap_buffer: unit/test_gap_buffer.c $(SRCDIR)/gap_buffer.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LDLIBS)

#/** @brief history — standalone, no ncurses */
unit/test_history: unit/test_history.c $(SRCDIR)/history.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LDLIBS)

#/** @brief config — header-only macros (config.h), no .c linking needed */
unit/test_config: unit/test_config.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief notes — #includes notes.c; links gap_buffer, history */
unit/test_notes: unit/test_notes.c $(SRCDIR)/gap_buffer.c $(SRCDIR)/history.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief util — SetColor calls ncurses attrset */
unit/test_util: unit/test_util.c $(SRCDIR)/util.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief error — links error.c + util.c; stubs RenderCriticalQuitConfirmation */
unit/test_error: unit/test_error.c $(SRCDIR)/error.c $(SRCDIR)/util.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief log — links log.c + deps; stubs remaining deps in test file */
unit/test_log: unit/test_log.c $(SRCDIR)/log.c $(SRCDIR)/error.c $(SRCDIR)/util.c \
               $(SRCDIR)/gap_buffer.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief anim — links util.c and error.c */
unit/test_anim: unit/test_anim.c $(SRCDIR)/anim.c $(SRCDIR)/util.c $(SRCDIR)/error.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief bar — structural tests; stubs notes.c functions */
unit/test_bar: unit/test_bar.c $(SRCDIR)/bar.c $(SRCDIR)/util.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/**
# * ---------------------------------------------------------------------------
# * Integration test binaries
# * ---------------------------------------------------------------------------
# */

INTEGRATION_BINS = \
	integration/test_pomodoro_workflow \
	integration/test_pomodoro_timer \
	integration/test_e2e

build-integration: $(INTEGRATION_BINS)

#/** @brief pomodoro_workflow — links util.c for IsStepEnded / FormatRemainingTime */
integration/test_pomodoro_workflow: integration/test_pomodoro_workflow.c $(SRCDIR)/util.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief pomodoro_timer — same deps as pomodoro_workflow */
integration/test_pomodoro_timer: integration/test_pomodoro_timer.c $(SRCDIR)/util.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

#/** @brief e2e — links log.c + util.c + gap_buffer.c for end-to-end CLI tests */
integration/test_e2e: integration/test_e2e.c $(SRCDIR)/log.c $(SRCDIR)/util.c $(SRCDIR)/gap_buffer.c $(HELPERS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^ $(NCURSES_LIBS) $(TEST_LDLIBS)

.PHONY: all build build-unit build-integration clean

#/** @brief Remove all test binaries */
clean:
	rm -f $(UNIT_BINS) $(INTEGRATION_BINS)
