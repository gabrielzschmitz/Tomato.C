/**
 * @file test_config.c
 * @brief Unit tests for config.h scene-bitmask macros and constants.
 *
 * Verifies POMODORO_SCENES, ALL_SCENES mask values and other
 * config-level invariants.
 */

#include <string.h>

#define DATADIR "/usr/share/tomato"

#include "config.h"
#include "test_helpers.h"
#include "tomato.h"

Config g_config;

/**
 * @brief Helper to set FPS for FPMS macro tests.
 */
static void set_fps(int fps) { g_config.misc.fps = fps; }

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

static void test_fpms_macro(void) {
  TEST("FPMS macro evaluates correctly with parentheses");
  set_fps(120);
  ASSERT_EQ((int)(60 * FPMS), (int)(60 * (1000.0 / 120)));
  set_fps(60);
  ASSERT_EQ((int)(60 * FPMS), (int)(60 * (1000.0 / 60)));
  set_fps(1);
  ASSERT_EQ((int)(60 * FPMS), (int)(60 * (1000.0 / 1)));
}

static char* expandDatadir(const char* input) {
  if (!input) return NULL;
  const char* marker = "$DATADIR";
  size_t mlen = strlen(marker);
  const char* pos = strstr(input, marker);
  if (!pos) return strdup(input);

  size_t prefix_len = pos - input;
  const char* datadir = DATADIR;
  size_t dlen = strlen(datadir);
  size_t suffix_len = strlen(pos + mlen);
  char* result = malloc(prefix_len + dlen + suffix_len + 1);
  if (!result) return NULL;
  memcpy(result, input, prefix_len);
  memcpy(result + prefix_len, datadir, dlen);
  memcpy(result + prefix_len + dlen, pos + mlen, suffix_len + 1);
  return result;
}

static void test_expand_datadir_no_marker(void) {
  TEST("expandDatadir copies string unchanged when no $DATADIR marker");
  char* out = expandDatadir("/tmp/test");
  ASSERT_STR_EQ(out, "/tmp/test");
  free(out);
}

static void test_expand_datadir_marker_only(void) {
  TEST("expandDatadir replaces $DATADIR with DATADIR macro");
  char* out = expandDatadir("$DATADIR");
  ASSERT_STR_EQ(out, DATADIR);
  free(out);
}

static void test_expand_datadir_prefix(void) {
  TEST("expandDatadir preserves prefix before $DATADIR");
  char* out = expandDatadir("custom$DATADIR");
  ASSERT_STR_EQ(out, "custom" DATADIR);
  free(out);
}

static void test_expand_datadir_suffix(void) {
  TEST("expandDatadir preserves suffix after $DATADIR");
  char* out = expandDatadir("$DATADIR/notes");
  ASSERT_STR_EQ(out, DATADIR "/notes");
  free(out);
}

static void test_expand_datadir_null(void) {
  TEST("expandDatadir returns NULL for NULL input");
  ASSERT_TRUE(expandDatadir(NULL) == NULL);
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
  RUN_TEST(test_fpms_macro,
           "FPMS macro evaluates correctly with parentheses");
  RUN_TEST(test_expand_datadir_no_marker,
           "expandDatadir copies string unchanged when no $DATADIR marker");
  RUN_TEST(test_expand_datadir_marker_only,
           "expandDatadir replaces $DATADIR with DATADIR macro");
  RUN_TEST(test_expand_datadir_prefix,
           "expandDatadir preserves prefix before $DATADIR");
  RUN_TEST(test_expand_datadir_suffix,
           "expandDatadir preserves suffix after $DATADIR");
  RUN_TEST(test_expand_datadir_null,
           "expandDatadir returns NULL for NULL input");
  return test_end();
}
