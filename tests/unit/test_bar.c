/**
 * @file test_bar.c
 * @brief Unit tests for the status bar module.
 *
 * Tests CreateStatusBar, FreeStatusBar, AddStatusBarModule, and
 * InvertModulesOrder.
 */

#include <stdlib.h>
#include <string.h>

#include "bar.h"
#include "config.h"
#include "error.h"
#include "test_helpers.h"
#include "ui.h"

Config g_config;

void RegisterClickRegion(AppData* app, int x, int y, int width, int height,
                         RegionType type, MenuAction action, int menu_index,
                         int item_index, int group_index) {
  (void)app;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  (void)type;
  (void)action;
  (void)menu_index;
  (void)item_index;
  (void)group_index;
}
void LogError(const char* context, ErrorType error) {
  (void)context;
  (void)error;
}
void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }
int GetNoteLines(int start_line, int max_lines, int* cursor_line) {
  (void)start_line;
  (void)max_lines;
  (void)cursor_line;
  return 1;
}
int GetNoteLinesFromText(const char* text) {
  (void)text;
  return 1;
}

static void dummy_update(AppData* app, StatusBarModule* m, Panel* panel) {
  (void)app;
  (void)m;
  (void)panel;
}

/**
 * ---------------------------------------------------------------------------
 * CreateStatusBar / FreeStatusBar
 * ---------------------------------------------------------------------------
 */

static void test_create_status_bar(void) {
  TEST("CreateStatusBar creates a status bar at given position");
  StatusBar* bar = CreateStatusBar(TOP);
  ASSERT_NOT_NULL(bar);
  ASSERT_EQ(bar->position, TOP);
  ASSERT_NULL(bar->left_modules);
  ASSERT_NULL(bar->center_modules);
  ASSERT_NULL(bar->right_modules);
  FreeStatusBar(bar);
}

static void test_create_status_bar_bottom(void) {
  TEST("CreateStatusBar with BOTTOM position");
  StatusBar* bar = CreateStatusBar(BOTTOM);
  ASSERT_NOT_NULL(bar);
  ASSERT_EQ(bar->position, BOTTOM);
  FreeStatusBar(bar);
}

static void test_free_status_bar_null(void) {
  TEST("FreeStatusBar NULL is safe");
  FreeStatusBar(NULL);
}

/**
 * ---------------------------------------------------------------------------
 * AddStatusBarModule
 * ---------------------------------------------------------------------------
 */

static void test_add_module_left(void) {
  TEST("AddStatusBarModule adds module to left list");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, LEFT, dummy_update);
  ASSERT_NOT_NULL(bar->left_modules);
  ASSERT_EQ(bar->left_modules->position, LEFT);
  ASSERT_EQ(bar->left_modules->id, 0);
  FreeStatusBar(bar);
}

static void test_add_module_center(void) {
  TEST("AddStatusBarModule adds module to center list");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, CENTER, dummy_update);
  ASSERT_NOT_NULL(bar->center_modules);
  FreeStatusBar(bar);
}

static void test_add_module_right(void) {
  TEST("AddStatusBarModule adds module to right list");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, RIGHT, dummy_update);
  ASSERT_NOT_NULL(bar->right_modules);
  FreeStatusBar(bar);
}

static void test_add_module_multiple_left(void) {
  TEST("AddStatusBarModule chains multiple modules on same side");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, LEFT, dummy_update);
  AddStatusBarModule(bar, LEFT, dummy_update);
  AddStatusBarModule(bar, LEFT, dummy_update);
  int count = 0;
  StatusBarModule* m = bar->left_modules;
  while (m) {
    count++;
    m = m->next;
  }
  ASSERT_EQ(count, 3);
  FreeStatusBar(bar);
}

static void test_add_module_null_bar(void) {
  TEST("AddStatusBarModule with NULL bar is safe");
  AddStatusBarModule(NULL, LEFT, dummy_update);
}

/**
 * ---------------------------------------------------------------------------
 * InvertModulesOrder
 * ---------------------------------------------------------------------------
 */

static void test_invert_modules_order(void) {
  TEST("InvertModulesOrder reverses linked list of modules");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, LEFT, dummy_update);
  AddStatusBarModule(bar, LEFT, dummy_update);
  AddStatusBarModule(bar, LEFT, dummy_update);
  StatusBarModule* reversed = InvertModulesOrder(bar->left_modules);
  ASSERT_NOT_NULL(reversed);
  int ids[3];
  StatusBarModule* m = reversed;
  for (int i = 0; i < 3; i++) {
    ids[i] = m->id;
    m = m->next;
  }
  ASSERT_EQ(ids[0], 2);
  ASSERT_EQ(ids[1], 1);
  ASSERT_EQ(ids[2], 0);
  ASSERT_NULL(m);
  bar->left_modules = reversed;
  FreeStatusBar(bar);
}

static void test_invert_modules_order_single(void) {
  TEST("InvertModulesOrder with single module");
  StatusBar* bar = CreateStatusBar(TOP);
  AddStatusBarModule(bar, LEFT, dummy_update);
  StatusBarModule* reversed = InvertModulesOrder(bar->left_modules);
  ASSERT_EQ(reversed->id, 0);
  FreeStatusBar(bar);
}

static void test_invert_modules_order_null(void) {
  TEST("InvertModulesOrder with NULL returns NULL");
  ASSERT_NULL(InvertModulesOrder(NULL));
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("bar");
  RUN_TEST(test_create_status_bar, "CreateStatusBar creates status bar");
  RUN_TEST(test_create_status_bar_bottom, "CreateStatusBar with BOTTOM");
  RUN_TEST(test_free_status_bar_null, "FreeStatusBar NULL is safe");
  RUN_TEST(test_add_module_left, "AddStatusBarModule adds to left list");
  RUN_TEST(test_add_module_center, "AddStatusBarModule adds to center list");
  RUN_TEST(test_add_module_right, "AddStatusBarModule adds to right list");
  RUN_TEST(test_add_module_multiple_left,
           "AddStatusBarModule chains multiple modules");
  RUN_TEST(test_add_module_null_bar, "AddStatusBarModule with NULL bar");
  RUN_TEST(test_invert_modules_order, "InvertModulesOrder reverses list");
  RUN_TEST(test_invert_modules_order_single,
           "InvertModulesOrder single module");
  RUN_TEST(test_invert_modules_order_null, "InvertModulesOrder with NULL");
  return test_end();
}
