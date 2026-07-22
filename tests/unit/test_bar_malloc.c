/**
 * @file test_bar_malloc.c
 * @brief OOM unit tests for the status bar module.
 *
 * Verifies that CreateStatusBar and AddStatusBarModule handle malloc
 * failure gracefully.  Linked with -Wl,--wrap,malloc.
 */

#include <stdlib.h>

#include "bar.h"
#include "config.h"
#include "error.h"
#include "test_helpers.h"
#include "test_malloc.h"
#include "ui.h"

Config g_config;

/* stubs */
void RegisterClickRegion(AppData* app, int x, int y, int width, int height,
                         RegionType type, MenuAction action, int menu_index,
                         int item_index, int group_index) {
  (void)app;(void)x;(void)y;(void)width;(void)height;
  (void)type;(void)action;(void)menu_index;(void)item_index;(void)group_index;
}
void LogError(const char* context, ErrorType error) {
  (void)context;(void)error;
}

void HistStreak(const char* path, int year, int month, int day,
                int* current, int* longest) {
  (void)path;(void)year;(void)month;(void)day;
  *current = 0; *longest = 0;
}
void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }
int GetNoteLines(int start_line, int max_lines, int* cursor_line) {
  (void)start_line;(void)max_lines;(void)cursor_line; return 1;
}
int GetNoteLinesFromText(const char* text) { (void)text; return 1; }

static void dummy_update(AppData* app, StatusBarModule* m, Panel* panel) {
  (void)app;(void)m;(void)panel;
}

/**
 * ---------------------------------------------------------------------------
 * OOM tests
 * ---------------------------------------------------------------------------
 */

/** @brief CreateStatusBar returns NULL when the initial malloc fails. */
static void test_create_status_bar_malloc_fail(void) {
  TEST("CreateStatusBar returns NULL when malloc fails");
  TestMallocFailAfter(0);
  StatusBar* bar = CreateStatusBar(TOP);
  ASSERT_NULL(bar);
  TestMallocReset();
}

/** @brief AddStatusBarModule returns error when module-struct malloc fails. */
static void test_add_module_malloc_fail(void) {
  TEST("AddStatusBarModule returns error when malloc fails");
  StatusBar* bar = CreateStatusBar(TOP);
  ASSERT_NOT_NULL(bar);
  TestMallocFailAfter(0);
  ErrorType err = AddStatusBarModule(bar, LEFT, dummy_update);
  ASSERT_NE(err, NO_ERROR);
  TestMallocReset();
  FreeStatusBar(bar);
}

/**
 * @brief AddStatusBarModule handles failure of the second malloc
 *        (module content allocation) without leaking.
 */
static void test_add_module_content_malloc_fail(void) {
  TEST("AddStatusBarModule handles content malloc failure");
  StatusBar* bar = CreateStatusBar(TOP);
  ASSERT_NOT_NULL(bar);
  TestMallocFailAfter(1);
  ErrorType err = AddStatusBarModule(bar, LEFT, dummy_update);
  ASSERT_NE(err, NO_ERROR);
  TestMallocReset();
  FreeStatusBar(bar);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("bar_malloc");
  RUN_TEST(test_create_status_bar_malloc_fail,
           "CreateStatusBar returns NULL when malloc fails");
  RUN_TEST(test_add_module_malloc_fail,
           "AddStatusBarModule returns error when malloc fails");
  RUN_TEST(test_add_module_content_malloc_fail,
           "AddStatusBarModule handles content malloc failure");
  return test_end();
}
