/**
 * @file test_init.c
 * @brief Unit tests for the init module (InitBorder).
 *
 * Verifies that InitBorder reads configured border characters
 * from g_config.visual.ui.icons.misc.border_chars.
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "init.h"
#include "test_helpers.h"

Config g_config;

/**
 * ---------------------------------------------------------------------------
 * InitBorder
 * ---------------------------------------------------------------------------
 */
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

int main(void) {
  test_begin("init");
  RUN_TEST(test_init_border,
           "InitBorder returns border with configured characters");
  return test_end();
}
