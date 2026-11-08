/**
 * @file test_util.c
 * @brief Unit tests for the utility module.
 *
 * Tests Max, IsStepEnded, IsCurrentStepInList, FormatRemainingTime,
 * UTF16CharCount, CheckConfigIconType, GetConfigIconType, and
 * UTF16CharFitWidth.
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "test_helpers.h"
#include "util.h"

/* Stub — util.c may call LogError on allocation failures */
void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

Config g_config;

#ifndef COLOR_BLACK
#define COLOR_BLACK 0
#endif

/**
 * ---------------------------------------------------------------------------
 * Max
 * ---------------------------------------------------------------------------
 */

static void test_max_a_greater(void) {
  TEST("Max when a > b");
  ASSERT_EQ(Max(10, 5), 10);
}

static void test_max_b_greater(void) {
  TEST("Max when b > a");
  ASSERT_EQ(Max(3, 7), 7);
}

static void test_max_equal(void) {
  TEST("Max when equal");
  ASSERT_EQ(Max(4, 4), 4);
}

static void test_max_negative(void) {
  TEST("Max with negatives");
  ASSERT_EQ(Max(-5, -10), -5);
}

/**
 * ---------------------------------------------------------------------------
 * IsStepEnded
 * ---------------------------------------------------------------------------
 */

static void test_is_step_ended_exact(void) {
  TEST("IsStepEnded at exact boundary");
  ASSERT_TRUE(IsStepEnded(300, 5));
}

static void test_is_step_ended_over(void) {
  TEST("IsStepEnded over boundary");
  ASSERT_TRUE(IsStepEnded(301, 5));
}

static void test_is_step_ended_under(void) {
  TEST("IsStepEnded under boundary");
  ASSERT_FALSE(IsStepEnded(299, 5));
}

static void test_is_step_ended_zero(void) {
  TEST("IsStepEnded with zero time");
  ASSERT_TRUE(IsStepEnded(0, 0));
}

/**
 * ---------------------------------------------------------------------------
 * IsCurrentStepInList
 * ---------------------------------------------------------------------------
 */

static void test_is_current_step_in_list_found(void) {
  TEST("IsCurrentStepInList found");
  int list[] = {0, 1, 2};
  ASSERT_TRUE(IsCurrentStepInList(list, 3, 1));
}

static void test_is_current_step_in_list_not_found(void) {
  TEST("IsCurrentStepInList not found");
  int list[] = {0, 1, 2};
  ASSERT_FALSE(IsCurrentStepInList(list, 3, 5));
}

static void test_is_current_step_in_list_empty(void) {
  TEST("IsCurrentStepInList empty array");
  ASSERT_FALSE(IsCurrentStepInList(NULL, 0, 0));
}

/**
 * ---------------------------------------------------------------------------
 * FormatRemainingTime
 * ---------------------------------------------------------------------------
 */

static void test_format_remaining_time_normal(void) {
  TEST("FormatRemainingTime normal");
  char* s = FormatRemainingTime(0, 25);
  ASSERT_NOT_NULL(s);
  ASSERT_STR_EQ(s, "25:00");
  free(s);
}

static void test_format_remaining_time_partial(void) {
  TEST("FormatRemainingTime partial");
  char* s = FormatRemainingTime(60, 25);
  ASSERT_NOT_NULL(s);
  ASSERT_STR_EQ(s, "24:00");
  free(s);
}

static void test_format_remaining_time_exceeded(void) {
  TEST("FormatRemainingTime exceeded");
  char* s = FormatRemainingTime(1800, 25);
  ASSERT_NOT_NULL(s);
  ASSERT_STR_EQ(s, "00:00");
  free(s);
}

static void test_format_remaining_time_zero(void) {
  TEST("FormatRemainingTime with 0 total");
  char* s = FormatRemainingTime(0, 0);
  ASSERT_NOT_NULL(s);
  ASSERT_STR_EQ(s, "00:00");
  free(s);
}

/**
 * ---------------------------------------------------------------------------
 * UTF16CharCount
 * ---------------------------------------------------------------------------
 */

static void test_utf16_char_count_ascii(void) {
  TEST("UTF16CharCount ASCII");
  ASSERT_EQ(UTF16CharCount("hello"), 5);
}

static void test_utf16_char_count_empty(void) {
  TEST("UTF16CharCount empty");
  ASSERT_EQ(UTF16CharCount(""), 0);
}

static void test_utf16_char_count_truncated(void) {
  TEST("UTF16CharCount stops at truncated UTF-8 sequences");
  ASSERT_EQ(UTF16CharCount("\xE2"), 1);
  ASSERT_EQ(UTF16CharCount("a\xE2"), 2);
  ASSERT_EQ(UTF16CharCount("\xE2\x82"), 2);
  ASSERT_EQ(UTF16CharCount("\xF0\x9F\x92"), 3);
}

static void test_utf16_char_count_invalid_continuation(void) {
  TEST("UTF16CharCount treats invalid continuation bytes as one byte");
  ASSERT_EQ(UTF16CharCount("\xE2\x28\xA1"), 3);
}

/**
 * ---------------------------------------------------------------------------
 * CheckConfigIconType
 * ---------------------------------------------------------------------------
 */

static void test_check_config_icon_type(void) {
  TEST("CheckConfigIconType");
  g_config.visual.icons_index = 0;
  g_config.visual.icons = "nerd-icons";
  ASSERT_TRUE(CheckConfigIconType());
}

/**
 * ---------------------------------------------------------------------------
 * GetConfigIconType
 * ---------------------------------------------------------------------------
 */

static void test_get_config_icon_type_nerd(void) {
  TEST("GetConfigIconType returns NERD_ICONS for index 0");
  g_config.visual.icons_index = 0;
  ASSERT_EQ(GetConfigIconType(), NERD_ICONS);
}

static void test_get_config_icon_type_emoji(void) {
  TEST("GetConfigIconType returns EMOJIS for index 1");
  g_config.visual.icons_index = 1;
  ASSERT_EQ(GetConfigIconType(), EMOJIS);
}

static void test_get_config_icon_type_ascii(void) {
  TEST("GetConfigIconType returns ASCII for index 2");
  g_config.visual.icons_index = 2;
  ASSERT_EQ(GetConfigIconType(), ASCII);
}

static void test_get_config_icon_type_unknown(void) {
  TEST("GetConfigIconType returns ASCII for unknown index");
  g_config.visual.icons_index = 99;
  ASSERT_EQ(GetConfigIconType(), ASCII);
}

/**
 * ---------------------------------------------------------------------------
 * UTF16CharFitWidth
 * ---------------------------------------------------------------------------
 */

static void test_utf16_char_fit_width_ascii(void) {
  TEST("UTF16CharFitWidth fits ASCII chars");
  int bytes = 0;
  int count = UTF16CharFitWidth("hello", 3, &bytes);
  ASSERT_EQ(count, 3);
  ASSERT_EQ(bytes, 3);
}

static void test_utf16_char_fit_width_exact(void) {
  TEST("UTF16CharFitWidth fits exact width");
  int bytes = 0;
  int count = UTF16CharFitWidth("abc", 3, &bytes);
  ASSERT_EQ(count, 3);
  ASSERT_EQ(bytes, 3);
}

static void test_utf16_char_fit_width_empty(void) {
  TEST("UTF16CharFitWidth handles empty string");
  int bytes = 99;
  int count = UTF16CharFitWidth("", 10, &bytes);
  ASSERT_EQ(count, 0);
  ASSERT_EQ(bytes, 0);
}

static void test_utf16_char_fit_width_zero_max(void) {
  TEST("UTF16CharFitWidth returns 0 for 0 max_width");
  int bytes = 99;
  int count = UTF16CharFitWidth("hello", 0, &bytes);
  ASSERT_EQ(count, 0);
  ASSERT_EQ(bytes, 0);
}

static void test_utf16_char_fit_width_truncated(void) {
  TEST("UTF16CharFitWidth stops at truncated UTF-8 sequences");
  int bytes = 0;
  int count = UTF16CharFitWidth("\xE2\x82", 10, &bytes);
  ASSERT_EQ(count, 2);
  ASSERT_EQ(bytes, 2);

  count = UTF16CharFitWidth("\xF0\x9F\x92", 10, &bytes);
  ASSERT_EQ(count, 3);
  ASSERT_EQ(bytes, 3);

  count = UTF16CharFitWidth("\xE2\x82\xAC", 1, &bytes);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(bytes, 3);
}

/**
 * ---------------------------------------------------------------------------
 * Timer clock
 * ---------------------------------------------------------------------------
 */

static void test_timer_clock_excludes_suspend(void) {
  TEST("Timer clock defaults to CLOCK_MONOTONIC");
  ASSERT_EQ((int)GetTimerClock(), (int)CLOCK_MONOTONIC);
}

static void test_current_time_monotonic(void) {
  TEST("GetCurrentTimeMS is monotonic");
  double before = GetCurrentTimeMS();
  double after = GetCurrentTimeMS();
  ASSERT_TRUE(after >= before);
}

/**
 * ---------------------------------------------------------------------------
 * UTF-8 cursor helpers
 * ---------------------------------------------------------------------------
 */

static const char* utf8_sample(void) { return "a\xC3\xA9\xF0\x9F\x8D\x85"; }

static void test_utf8_prev_char_start(void) {
  TEST("UTF8PrevCharStart steps over multibyte sequences");
  const char* s = utf8_sample();
  ASSERT_EQ(UTF8PrevCharStart(s, 7), 3); /* before the emoji */
  ASSERT_EQ(UTF8PrevCharStart(s, 3), 1); /* before é */
  ASSERT_EQ(UTF8PrevCharStart(s, 1), 0); /* before a */
  ASSERT_EQ(UTF8PrevCharStart(s, 0), 0);
}

static void test_utf8_next_char_start(void) {
  TEST("UTF8NextCharStart steps over multibyte sequences");
  const char* s = utf8_sample();
  ASSERT_EQ(UTF8NextCharStart(s, 7, 0), 1);
  ASSERT_EQ(UTF8NextCharStart(s, 7, 1), 3);
  ASSERT_EQ(UTF8NextCharStart(s, 7, 3), 7);
  ASSERT_EQ(UTF8NextCharStart(s, 7, 7), 7);
}

static void test_utf8_align_cursor(void) {
  TEST("UTF8AlignCursor snaps into the middle of a character");
  const char* s = utf8_sample();
  ASSERT_EQ(UTF8AlignCursor(s, 7, 2), 1);
  ASSERT_EQ(UTF8AlignCursor(s, 7, 5), 3);
  ASSERT_EQ(UTF8AlignCursor(s, 7, 99), 7);
  ASSERT_EQ(UTF8AlignCursor(s, 7, -4), 0);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("util");
  RUN_TEST(test_max_a_greater, "Max when a > b");
  RUN_TEST(test_max_b_greater, "Max when b > a");
  RUN_TEST(test_max_equal, "Max when equal");
  RUN_TEST(test_max_negative, "Max with negatives");
  RUN_TEST(test_is_step_ended_exact, "IsStepEnded at exact boundary");
  RUN_TEST(test_is_step_ended_over, "IsStepEnded over boundary");
  RUN_TEST(test_is_step_ended_under, "IsStepEnded under boundary");
  RUN_TEST(test_is_step_ended_zero, "IsStepEnded with zero time");
  RUN_TEST(test_is_current_step_in_list_found, "IsCurrentStepInList found");
  RUN_TEST(test_is_current_step_in_list_not_found,
           "IsCurrentStepInList not found");
  RUN_TEST(test_is_current_step_in_list_empty,
           "IsCurrentStepInList empty array");
  RUN_TEST(test_format_remaining_time_normal, "FormatRemainingTime normal");
  RUN_TEST(test_format_remaining_time_partial, "FormatRemainingTime partial");
  RUN_TEST(test_format_remaining_time_exceeded, "FormatRemainingTime exceeded");
  RUN_TEST(test_format_remaining_time_zero, "FormatRemainingTime with 0 total");
  RUN_TEST(test_utf16_char_count_ascii, "UTF16CharCount ASCII");
  RUN_TEST(test_utf16_char_count_empty, "UTF16CharCount empty");
  RUN_TEST(test_utf16_char_count_truncated,
           "UTF16CharCount handles truncated sequences");
  RUN_TEST(test_utf16_char_count_invalid_continuation,
           "UTF16CharCount handles invalid continuations");
  RUN_TEST(test_check_config_icon_type, "CheckConfigIconType");
  RUN_TEST(test_get_config_icon_type_nerd,
           "GetConfigIconType returns NERD_ICONS for nerd-icons");
  RUN_TEST(test_get_config_icon_type_emoji,
           "GetConfigIconType returns EMOJIS for emojis");
  RUN_TEST(test_get_config_icon_type_ascii,
           "GetConfigIconType returns ASCII for ascii");
  RUN_TEST(test_get_config_icon_type_unknown,
           "GetConfigIconType returns ASCII for unknown");
  RUN_TEST(test_utf16_char_fit_width_ascii,
           "UTF16CharFitWidth fits ASCII chars");
  RUN_TEST(test_utf16_char_fit_width_exact,
           "UTF16CharFitWidth fits exact width");
  RUN_TEST(test_utf16_char_fit_width_empty,
           "UTF16CharFitWidth handles empty string");
  RUN_TEST(test_utf16_char_fit_width_zero_max,
           "UTF16CharFitWidth returns 0 for 0 max_width");
  RUN_TEST(test_utf16_char_fit_width_truncated,
           "UTF16CharFitWidth handles truncated sequences");
  RUN_TEST(test_timer_clock_excludes_suspend,
           "timer clock defaults to CLOCK_MONOTONIC");
  RUN_TEST(test_current_time_monotonic, "GetCurrentTimeMS is monotonic");
  RUN_TEST(test_utf8_prev_char_start,
           "UTF8PrevCharStart steps over multibyte sequences");
  RUN_TEST(test_utf8_next_char_start,
           "UTF8NextCharStart steps over multibyte sequences");
  RUN_TEST(test_utf8_align_cursor,
           "UTF8AlignCursor snaps into the middle of a character");
  return test_end();
}
