/**
 * @file test_log.c
 * @brief Unit tests for the logging module.
 *
 * Tests HistDaysInMonth, HistDayOfWeek, HistLevelForCount,
 * and HistSessionsForDay (deduplication, formula-based duration, end times).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "log.h"
#include "test_helpers.h"
#include "tomato.h"

Config g_config;

void RenderCriticalQuitConfirmation(AppData* app) { (void)app; }

/**
 * ---------------------------------------------------------------------------
 * Helpers for binary log fixtures
 * ---------------------------------------------------------------------------
 */

static time_t mkts(int y, int m, int d, int h, int min) {
  struct tm tm = {0};
  tm.tm_year = y - 1900;
  tm.tm_mon = m - 1;
  tm.tm_mday = d;
  tm.tm_hour = h;
  tm.tm_min = min;
  tm.tm_sec = 0;
  return mktime(&tm);
}

static void write_rec(FILE* f, uint16_t idx, uint8_t step, uint8_t cycle,
                      uint8_t tcycles, uint8_t work, uint8_t sp, uint8_t lp,
                      uint32_t elapsed, uint32_t step_time, uint8_t status,
                      uint32_t start) {
  pomodoroLogRecord r = {.session_index = idx,
                         .current_step = step,
                         .current_cycle = cycle,
                         .total_cycles = tcycles,
                         .work_time = work,
                         .short_pause_time = sp,
                         .long_pause_time = lp,
                         .total_elapsed = elapsed,
                         .current_step_time = step_time,
                         .status = status,
                         .session_start_time = start};
  fwrite(&r, sizeof(r), 1, f);
}

/**
 * ---------------------------------------------------------------------------
 * HistSessionsForDay — formula-based duration & end-time edge cases
 * ---------------------------------------------------------------------------
 */

static void test_hist_sessions_empty_file(void) {
  TEST("HistSessionsForDay empty file returns 0");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 0);

  remove(tmp);
}

static void test_hist_sessions_skipped_session(void) {
  TEST("HistSessionsForDay skipped session: endTime == startTime");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 11, 12);
  /* Skipped session: total_elapsed=0, status=0 (completed via skip) */
  write_rec(f, 1, 0, 4, 4, 25, 5, 15, 0, 0, 0, (uint32_t)start);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(durs[0], 7800); /* 4*25*60 + 3*5*60 + 15*60 */
  ASSERT_EQ(ends[0], start);

  remove(tmp);
}

static void test_hist_sessions_completed_formula(void) {
  TEST("HistSessionsForDay completed session uses formula, not elapsed");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 9, 0);
  /* 8 cycles, work=25, short=5, long=15 */
  write_rec(f, 1, 0, 8, 8, 25, 5, 15, 13000, 0, 0, (uint32_t)start);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  /* 8*25*60 + 7*5*60 + 15*60 = 12000 + 2100 + 900 = 15000 */
  ASSERT_EQ(durs[0], 15000);
  ASSERT_EQ(ends[0], start + 13000);

  remove(tmp);
}

static void test_hist_sessions_dedup_many_records(void) {
  TEST("HistSessionsForDay deduplicates many records into one session");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 10, 0);
  /* Write 145 per-minute records for same session */
  for (int i = 0; i < 145; i++) {
    write_rec(f, 1, 0, 4, 4, 25, 5, 15, (uint32_t)(start + i * 60),
              (uint32_t)(i * 60), 0, (uint32_t)start);
  }
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(durs[0], 7800);

  remove(tmp);
}

static void test_hist_sessions_uncompleted(void) {
  TEST("HistSessionsForDay uncompleted session has duration 0");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 14, 0);
  write_rec(f, 1, 0, 1, 4, 25, 5, 15, 300, 300, 1, (uint32_t)start);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(durs[0], 0); /* uncompleted → duration 0 */

  remove(tmp);
}

static void test_hist_sessions_multiple_same_day(void) {
  TEST("HistSessionsForDay multiple sessions same day returns all");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t s1 = mkts(2026, 7, 19, 9, 0);
  time_t s2 = mkts(2026, 7, 19, 11, 0);
  write_rec(f, 1, 0, 4, 4, 25, 5, 15, 7800, 0, 0, (uint32_t)s1);
  write_rec(f, 2, 0, 2, 2, 25, 5, 15, 3300, 0, 0, (uint32_t)s2);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 2);
  ASSERT_EQ(durs[0], 7800);
  ASSERT_EQ(durs[1], 4200); /* 2*25*60 + 1*5*60 + 15*60 = 4200 */

  remove(tmp);
}

static void test_hist_sessions_different_day_filter(void) {
  TEST("HistSessionsForDay only returns sessions on requested day");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t s1 = mkts(2026, 7, 18, 10, 0); /* prev day */
  time_t s2 = mkts(2026, 7, 19, 10, 0); /* target day */
  write_rec(f, 1, 0, 4, 4, 25, 5, 15, 7800, 0, 0, (uint32_t)s1);
  write_rec(f, 2, 0, 4, 4, 25, 5, 15, 7800, 0, 0, (uint32_t)s2);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(indices[0], 2);

  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * HistSessionsForDay — end-times via total_elapsed
 * ---------------------------------------------------------------------------
 */

static void test_hist_sessions_end_time_from_elapsed(void) {
  TEST("HistSessionsForDay end time = start + total_elapsed (real end)");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 11, 12);
  /* Real elapsed = 450s (7.5 min of work), then user skipped */
  write_rec(f, 1, 0, 1, 4, 25, 5, 15, 450, 450, 0, (uint32_t)start);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  /* End should be start + 450 (real recorded end), not start + formula */
  ASSERT_EQ(ends[0], start + 450);
  /* Duration should still be formula */
  ASSERT_EQ(durs[0], 7800);

  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * HistSessionsForDay — end-times from skip (total_elapsed=0)
 * ---------------------------------------------------------------------------
 */

static void test_hist_sessions_skip_instant_end_time(void) {
  TEST("HistSessionsForDay skip-step: end time same as start");

  char tmp[] = "/tmp/t_log_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t start = mkts(2026, 7, 19, 11, 12);
  /* Skip: total_elapsed=0, so start+0 = start */
  write_rec(f, 42, 0, 4, 4, 25, 5, 15, 0, 0, 0, (uint32_t)start);
  fclose(f);

  int indices[10];
  time_t starts[10], ends[10];
  int durs[10], sts[10];
  int n =
    HistSessionsForDay(tmp, 2026, 7, 19, indices, starts, ends, durs, sts, 10);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(ends[0], start);

  remove(tmp);
}

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
  RUN_TEST(test_hist_sessions_empty_file, "HistSessionsForDay empty file");
  RUN_TEST(test_hist_sessions_skipped_session,
           "HistSessionsForDay skipped session endTime==startTime");
  RUN_TEST(test_hist_sessions_completed_formula,
           "HistSessionsForDay completed formula duration");
  RUN_TEST(test_hist_sessions_dedup_many_records,
           "HistSessionsForDay dedup many records");
  RUN_TEST(test_hist_sessions_uncompleted,
           "HistSessionsForDay uncompleted duration 0");
  RUN_TEST(test_hist_sessions_multiple_same_day,
           "HistSessionsForDay multiple same day");
  RUN_TEST(test_hist_sessions_different_day_filter,
           "HistSessionsForDay different day filter");
  RUN_TEST(test_hist_sessions_end_time_from_elapsed,
           "HistSessionsForDay endTime from total_elapsed");
  RUN_TEST(test_hist_sessions_skip_instant_end_time,
           "HistSessionsForDay skip instant endTime");
  return test_end();
}
