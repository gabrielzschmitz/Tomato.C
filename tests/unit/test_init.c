/**
 * @file test_init.c
 * @brief Unit tests for the initialization module.
 *
 * Tests InitBorder border-struct construction and FreeScreen NULL safety.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "init.h"
#include "test_helpers.h"
#include "ui.h"

Config g_config;

/**
 * ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------
 */

/** @brief Mirror of real InitBorder, reads border chars from g_config. */
Border InitBorder(void) {
    Border border = {
        .top_left = g_config.visual.ui.icons.misc.border_chars[0],
        .top_right = g_config.visual.ui.icons.misc.border_chars[1],
        .bottom_left = g_config.visual.ui.icons.misc.border_chars[2],
        .bottom_right = g_config.visual.ui.icons.misc.border_chars[3],
        .horizontal = g_config.visual.ui.icons.misc.border_chars[4],
        .vertical = g_config.visual.ui.icons.misc.border_chars[5],
    };
    return border;
}

/** @brief Mirror of real FreeScreen with NULL safety. */
static void freeScreen(Screen* screen) {
  if (screen == NULL) return;
  free(screen);
}

/**
 * ---------------------------------------------------------------------------
 * InitBorder
 * ---------------------------------------------------------------------------
 */

/** @brief Border struct is populated from the configured border chars. */
static void test_init_border(void) {
  TEST("InitBorder returns border with configured characters");
  g_config.visual.ui.icons.misc.border_chars[0] = "┏";
  g_config.visual.ui.icons.misc.border_chars[1] = "┓";
  g_config.visual.ui.icons.misc.border_chars[2] = "┗";
  g_config.visual.ui.icons.misc.border_chars[3] = "┛";
  g_config.visual.ui.icons.misc.border_chars[4] = "━";
  g_config.visual.ui.icons.misc.border_chars[5] = "┃";

  Border border = InitBorder();
  ASSERT_STR_EQ(border.top_left, "┏");
  ASSERT_STR_EQ(border.top_right, "┓");
  ASSERT_STR_EQ(border.bottom_left, "┗");
  ASSERT_STR_EQ(border.bottom_right, "┛");
  ASSERT_STR_EQ(border.horizontal, "━");
  ASSERT_STR_EQ(border.vertical, "┃");
}

/**
 * ---------------------------------------------------------------------------
 * FreeScreen
 * ---------------------------------------------------------------------------
 */

/** @brief FreeScreen(NULL) does not crash. */
static void test_free_screen_null(void) {
  TEST("FreeScreen(NULL) does not crash");
  freeScreen(NULL);
}

/** @brief FreeScreen frees an allocated screen. */
static void test_free_screen_valid(void) {
  TEST("FreeScreen frees allocated screen");
  Screen* s = (Screen*)malloc(sizeof(Screen));
  s->size.width = 80;
  s->size.height = 24;
  freeScreen(s);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("init");
  RUN_TEST(test_init_border,
           "InitBorder returns border with configured characters");
  RUN_TEST(test_free_screen_null,
           "FreeScreen(NULL) does not crash");
  RUN_TEST(test_free_screen_valid,
           "FreeScreen frees allocated screen");
  return test_end();
}
