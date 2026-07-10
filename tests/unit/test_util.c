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
#include "test_helpers.h"
#include "util.h"

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
  TEST("GetConfigIconType returns NERD_ICONS for nerd-icons");
  g_config.visual.icons = "nerd-icons";
  ASSERT_EQ(GetConfigIconType(), NERD_ICONS);
}

static void test_get_config_icon_type_emoji(void) {
  TEST("GetConfigIconType returns EMOJIS for emojis");
  g_config.visual.icons = "emojis";
  ASSERT_EQ(GetConfigIconType(), EMOJIS);
}

static void test_get_config_icon_type_ascii(void) {
  TEST("GetConfigIconType returns ASCII for ascii");
  g_config.visual.icons = "ascii";
  ASSERT_EQ(GetConfigIconType(), ASCII);
}

static void test_get_config_icon_type_unknown(void) {
  TEST("GetConfigIconType returns ASCII for unknown");
  g_config.visual.icons = "unknown";
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
  return test_end();
}
