/**
 * @file test_config.c
 * @brief Unit tests for config.h scene-bitmask macros and constants.
 *
 * Verifies POMODORO_SCENES, ALL_SCENES mask values and other
 * config-level invariants.
 */

#include "config.h"
#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------------
 */

static void test_pomodoro_scenes_mask(void) {
  TEST("POMODORO_SCENES mask is correct");
  ASSERT_TRUE(POMODORO_SCENES & SCENE_WORK_TIME);
  ASSERT_TRUE(POMODORO_SCENES & SCENE_SHORT_PAUSE);
  ASSERT_TRUE(POMODORO_SCENES & SCENE_LONG_PAUSE);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_MAIN_MENU);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_NOTES);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_HELP);
  ASSERT_FALSE(POMODORO_SCENES & SCENE_NOISE);
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

static void test_all_scenes_excludes_some(void) {
  TEST("ALL_SCENES does NOT cover NOISE or HISTORY scenes");
  ASSERT_FALSE(ALL_SCENES & SCENE_NOISE);
  ASSERT_FALSE(ALL_SCENES & SCENE_HISTORY_OVERVIEW);
  ASSERT_FALSE(ALL_SCENES & SCENE_HISTORY_DAY);
  ASSERT_FALSE(ALL_SCENES & SCENE_HISTORY_STATS);
  ASSERT_FALSE(ALL_SCENES & SCENE_PREFS_STEPPER);
  ASSERT_FALSE(ALL_SCENES & SCENE_PREFS_SELECT);
}

static void test_scene_bit_positions(void) {
  TEST("scene enum bit positions are powers of two");
  ASSERT_EQ(SCENE_MAIN_MENU, 1 << 0);
  ASSERT_EQ(SCENE_WORK_TIME, 1 << 1);
  ASSERT_EQ(SCENE_SHORT_PAUSE, 1 << 2);
  ASSERT_EQ(SCENE_LONG_PAUSE, 1 << 3);
  ASSERT_EQ(SCENE_NOTES, 1 << 4);
  ASSERT_EQ(SCENE_NOTES_TRANSITION, 1 << 5);
  ASSERT_EQ(SCENE_HELP, 1 << 6);
  ASSERT_EQ(SCENE_CONTINUE, 1 << 7);
  ASSERT_EQ(SCENE_NOISE, 1 << 8);
  ASSERT_EQ(SCENE_PREFERENCES, 1 << 12);
}

static void test_mode_constants(void) {
  TEST("mode constants have correct values");
  ASSERT_EQ(DEFAULT, 1 << 0);
  ASSERT_EQ(NORMAL, 1 << 1);
  ASSERT_EQ(INSERT, 1 << 2);
  ASSERT_EQ(VISUAL, 1 << 3);
}

static void test_keycode_aliases(void) {
  TEST("CTRL aliases match ASCII control codes");
  ASSERT_EQ(CTRLC, 3);  /* Ctrl-C */
  ASSERT_EQ(CTRLR, 18); /* Ctrl-R */
  ASSERT_EQ(CTRLW, 23); /* Ctrl-W */
  ASSERT_EQ(CTRLH, 8);  /* Ctrl-H */
}

static void test_separator_length(void) {
  TEST("SEPARATOR is correct length");
  ASSERT_EQ(strlen(SEPARATOR), 75);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("config");
  RUN_TEST(test_pomodoro_scenes_mask, "POMODORO_SCENES mask is correct");
  RUN_TEST(test_all_scenes_mask, "ALL_SCENES mask covers all scene bits");
  RUN_TEST(test_all_scenes_excludes_some,
           "ALL_SCENES does NOT cover NOISE or HISTORY scenes");
  RUN_TEST(test_scene_bit_positions,
           "scene enum bit positions are powers of two");
  RUN_TEST(test_mode_constants, "mode constants have correct values");
  RUN_TEST(test_keycode_aliases, "CTRL aliases match ASCII control codes");
  RUN_TEST(test_separator_length, "SEPARATOR is correct length");
  return test_end();
}
