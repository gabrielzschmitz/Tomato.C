/**
 * @file test_log.c
 * @brief Unit tests for the logging module.
 *
 * Tests HistDaysInMonth, HistDayOfWeek, and HistLevelForCount.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "error.h"
#include "log.h"
#include "test_helpers.h"
#include "tomato.h"

Config g_config;

void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }

/**
 * ---------------------------------------------------------------------------
 * HistDaysInMonth
 * ---------------------------------------------------------------------------
 */

static void test_hist_days_in_month_jan(void) {
  TEST("HistDaysInMonth January");
  ASSERT_EQ(HistDaysInMonth(2024, 1), 31);
}

static void test_hist_days_in_month_feb_non_leap(void) {
  TEST("HistDaysInMonth February non-leap");
  ASSERT_EQ(HistDaysInMonth(2023, 2), 28);
}

static void test_hist_days_in_month_feb_leap(void) {
  TEST("HistDaysInMonth February leap");
  ASSERT_EQ(HistDaysInMonth(2024, 2), 29);
}

static void test_hist_days_in_month_feb_century(void) {
  TEST("HistDaysInMonth February century non-leap");
  ASSERT_EQ(HistDaysInMonth(2100, 2), 28);
}

/**
 * ---------------------------------------------------------------------------
 * HistDayOfWeek
 * ---------------------------------------------------------------------------
 */

static void test_hist_day_of_week(void) {
  TEST("HistDayOfWeek known date");
  ASSERT_EQ(HistDayOfWeek(2024, 1, 1), 1); /* 2024-01-01 = Monday */
}

static void test_hist_day_of_week_sunday(void) {
  TEST("HistDayOfWeek Sunday");
  ASSERT_EQ(HistDayOfWeek(2024, 1, 7), 0); /* 2024-01-07 = Sunday */
}

/**
 * ---------------------------------------------------------------------------
 * HistLevelForCount
 * ---------------------------------------------------------------------------
 */

static void test_hist_level_for_count_zero(void) {
  TEST("HistLevelForCount 0 returns 0");
  ASSERT_EQ(HistLevelForCount(0), 0);
}

static void test_hist_level_for_count_one(void) {
  TEST("HistLevelForCount 1");
  ASSERT_EQ(HistLevelForCount(1), 1);
}

static void test_hist_level_for_count_five(void) {
  TEST("HistLevelForCount 5");
  ASSERT_EQ(HistLevelForCount(5), 2);
}

static void test_hist_level_for_count_ten(void) {
  TEST("HistLevelForCount 10");
  ASSERT_EQ(HistLevelForCount(10), 3);
}

static void test_hist_level_for_count_twenty(void) {
  TEST("HistLevelForCount 20 caps at 3");
  ASSERT_EQ(HistLevelForCount(20), 3);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("log");
  RUN_TEST(test_hist_days_in_month_jan, "HistDaysInMonth January");
  RUN_TEST(test_hist_days_in_month_feb_non_leap,
           "HistDaysInMonth February non-leap");
  RUN_TEST(test_hist_days_in_month_feb_leap, "HistDaysInMonth February leap");
  RUN_TEST(test_hist_days_in_month_feb_century,
           "HistDaysInMonth February century non-leap");
  RUN_TEST(test_hist_day_of_week, "HistDayOfWeek known date");
  RUN_TEST(test_hist_day_of_week_sunday, "HistDayOfWeek Sunday");
  RUN_TEST(test_hist_level_for_count_zero, "HistLevelForCount 0");
  RUN_TEST(test_hist_level_for_count_one, "HistLevelForCount 1");
  RUN_TEST(test_hist_level_for_count_five, "HistLevelForCount 5");
  RUN_TEST(test_hist_level_for_count_ten, "HistLevelForCount 10");
  RUN_TEST(test_hist_level_for_count_twenty, "HistLevelForCount 20");
  return test_end();
}
