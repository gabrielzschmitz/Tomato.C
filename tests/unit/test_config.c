/**
 * @file test_config.c
 * @brief Unit tests for config.h scene-bitmask macros.
 *
 * Verifies POMODORO_SCENES and ALL_SCENES mask values.
 */

#include "config.h"
#include "test_helpers.h"

static void test_pomodoro_scenes_mask(void) {
  TEST("POMODORO_SCENES mask is correct");
  ASSERT_TRUE(POMODORO_SCENES & SCENE_WORK_TIME);
  ASSERT_TRUE(POMODORO_SCENES & SCENE_SHORT_PAUSE);
  ASSERT_TRUE(POMODORO_SCENES & SCENE_LONG_PAUSE);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_MAIN_MENU);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_NOTES);
}

static void test_all_scenes_mask(void) {
  TEST("ALL_SCENES mask covers all scene bits");
  ASSERT_TRUE(ALL_SCENES & SCENE_MAIN_MENU);
  ASSERT_TRUE(ALL_SCENES & SCENE_WORK_TIME);
  ASSERT_TRUE(ALL_SCENES & SCENE_SHORT_PAUSE);
  ASSERT_TRUE(ALL_SCENES & SCENE_LONG_PAUSE);
  ASSERT_TRUE(ALL_SCENES & SCENE_NOTES);
  ASSERT_TRUE(ALL_SCENES & SCENE_HELP);
  ASSERT_TRUE(ALL_SCENES & SCENE_CONTINUE);
  ASSERT_TRUE(ALL_SCENES & SCENE_PREFERENCES);
}

int main(void) {
  test_begin("config");
  RUN_TEST(test_pomodoro_scenes_mask, "POMODORO_SCENES mask is correct");
  RUN_TEST(test_all_scenes_mask, "ALL_SCENES mask covers all scene bits");
  return test_end();
}
