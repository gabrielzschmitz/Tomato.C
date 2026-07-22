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
#include <sys/stat.h>
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
 * GetPomodoroHistoryDay — dynamic allocation test
 * ---------------------------------------------------------------------------
 */

static void test_get_history_day_many_sessions(void) {
  TEST("GetPomodoroHistoryDay handles >100 sessions without crash");
  char tmp[] = "/tmp/t_log_day_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  FILE* f = fopen(tmp, "wb");
  ASSERT_NOT_NULL(f);
  time_t today = time(NULL);
  /* Write 150 unique session records for today */
  for (int i = 1; i <= 150; i++) {
    write_rec(f, (uint16_t)i, 0, 4, 4, 25, 5, 15, 0, 0, 0,
              (uint32_t)today + (i * 60));
  }
  fclose(f);

  /* Call GetPomodoroHistoryDay — must not crash */
  GetPomodoroHistoryDay(tmp);
  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * SavePomodoro / LoadPomodoro — round-trip
 * ---------------------------------------------------------------------------
 */

static void test_save_and_load_pomodoro(void) {
  TEST("SavePomodoro then LoadPomodoro fields match");
  char tmp[] = "/tmp/t_log_sl_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  PomodoroData in;
  memset(&in, 0, sizeof(in));
  in.total_cycles = 4;
  in.current_cycle = 2;
  in.work_time = 25;
  in.short_pause_time = 5;
  in.long_pause_time = 30;
  in.current_step = 1;
  in.current_step_time = 300;
  in.total_elapsed = 1800;
  in.status = 1;
  in.session_index = 7;
  in.session_start_time = 1700000000;

  ErrorType err = SavePomodoro(tmp, &in, true);
  ASSERT_EQ(err, NO_ERROR);

  PomodoroData out;
  memset(&out, 0, sizeof(out));
  err = LoadPomodoro(tmp, &out);
  ASSERT_EQ(err, NO_ERROR);
  ASSERT_EQ(out.total_cycles, in.total_cycles);
  ASSERT_EQ(out.current_cycle, in.current_cycle);
  ASSERT_EQ(out.work_time, in.work_time);
  ASSERT_EQ(out.short_pause_time, in.short_pause_time);
  ASSERT_EQ(out.long_pause_time, in.long_pause_time);
  ASSERT_EQ(out.current_step, in.current_step);
  ASSERT_EQ(out.current_step_time, in.current_step_time);
  ASSERT_EQ(out.total_elapsed, in.total_elapsed);
  ASSERT_EQ(out.status, in.status);

  remove(tmp);
}

static void test_save_pomodoro_append(void) {
  TEST("SavePomodoro append creates multiple records");
  char tmp[] = "/tmp/t_log_sa_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  PomodoroData d1, d2;
  memset(&d1, 0, sizeof(d1));
  d1.session_index = 1;
  d1.total_cycles = 4;
  d1.work_time = 25;
  d1.current_step = 1;
  d1.current_step_time = 100;

  memset(&d2, 0, sizeof(d2));
  d2.session_index = 2;
  d2.total_cycles = 4;
  d2.work_time = 25;
  d2.current_step = 3;
  d2.current_step_time = 200;

  ASSERT_EQ(SavePomodoro(tmp, &d1, true), NO_ERROR);
  ASSERT_EQ(SavePomodoro(tmp, &d2, true), NO_ERROR);

  PomodoroData out;
  memset(&out, 0, sizeof(out));
  ASSERT_EQ(LoadPomodoro(tmp, &out), NO_ERROR);
  ASSERT_EQ(out.session_index, 2);
  ASSERT_EQ(out.current_step_time, 200);

  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * GetLastLogIndexOnly
 * ---------------------------------------------------------------------------
 */

static void test_get_last_log_index_empty(void) {
  TEST("GetLastLogIndexOnly empty file returns 0");
  char tmp[] = "/tmp/t_log_li_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);
  ASSERT_EQ(GetLastLogIndexOnly(tmp), 0);
  remove(tmp);
}

static void test_get_last_log_index_after_save(void) {
  TEST("GetLastLogIndexOnly returns last session_index");
  char tmp[] = "/tmp/t_log_ls_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  PomodoroData d;
  memset(&d, 0, sizeof(d));
  d.session_index = 42;
  d.total_cycles = 4;
  d.work_time = 25;
  ASSERT_EQ(SavePomodoro(tmp, &d, true), NO_ERROR);

  ASSERT_EQ(GetLastLogIndexOnly(tmp), 42);
  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * RemoveUncompletedEntries — binary log cleanup
 * ---------------------------------------------------------------------------
 */

static void test_remove_uncompleted_entries(void) {
  TEST("RemoveUncompletedEntries removes uncompleted entry for given index");
  char tmp[] = "/tmp/t_log_ru_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  /* Write 3 completed (status=0) then 2 uncompleted (status=1) */
  for (int i = 1; i <= 5; i++) {
    PomodoroData d;
    memset(&d, 0, sizeof(d));
    d.session_index = i;
    d.total_cycles = 4;
    d.work_time = 25;
    d.current_step = 1;
    d.status = (i <= 3) ? 0 : 1;
    ASSERT_EQ(SavePomodoro(tmp, &d, true), NO_ERROR);
  }

  /* Remove uncompleted for session_index 5 only */
  ASSERT_EQ(RemoveUncompletedEntries(tmp, 5), NO_ERROR);

  /* Session 5 removed, session 4 (uncompleted) is now last */
  PomodoroData out;
  memset(&out, 0, sizeof(out));
  ASSERT_EQ(LoadPomodoro(tmp, &out), NO_ERROR);
  ASSERT_EQ(out.session_index, 4);

  /* Remove session 4 as well */
  ASSERT_EQ(RemoveUncompletedEntries(tmp, 4), NO_ERROR);

  /* Now session 3 (completed) is last */
  memset(&out, 0, sizeof(out));
  ASSERT_EQ(LoadPomodoro(tmp, &out), NO_ERROR);
  ASSERT_EQ(out.session_index, 3);
  ASSERT_EQ(out.status, 0);

  remove(tmp);
}

static void test_remove_uncompleted_all_completed(void) {
  TEST("RemoveUncompletedEntries with all completed keeps all");
  char tmp[] = "/tmp/t_log_ra_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  for (int i = 1; i <= 3; i++) {
    PomodoroData d;
    memset(&d, 0, sizeof(d));
    d.session_index = i;
    d.total_cycles = 4;
    d.work_time = 25;
    d.current_step = 1;
    d.status = 0;
    ASSERT_EQ(SavePomodoro(tmp, &d, true), NO_ERROR);
  }

  ASSERT_EQ(RemoveUncompletedEntries(tmp, 3), NO_ERROR);

  PomodoroData out;
  ASSERT_EQ(LoadPomodoro(tmp, &out), NO_ERROR);
  ASSERT_EQ(out.session_index, 3);

  remove(tmp);
}

/**
 * ---------------------------------------------------------------------------
 * E2 — File I/O error paths
 * ---------------------------------------------------------------------------
 */

static void test_load_empty_file(void) {
  TEST("LoadPomodoro empty file returns NO_ERROR, data unchanged");
  char tmp[] = "/tmp/t_log_em_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);
  close(fd);

  PomodoroData out;
  memset(&out, 0xFF, sizeof(out));
  ErrorType err = LoadPomodoro(tmp, &out);
  ASSERT_EQ(err, NO_ERROR);
  /* Data is left as-is when no valid record exists */
  remove(tmp);
}

static void test_load_corrupted_file_partial_record(void) {
  TEST("LoadPomodoro partial (truncated) record does not crash");
  char tmp[] = "/tmp/t_log_co_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);

  pomodoroLogRecord rec;
  memset(&rec, 0, sizeof(rec));
  rec.session_index = 1;
  rec.total_cycles = 4;
  rec.work_time = 25;
  size_t half = sizeof(rec) / 2;
  ASSERT_EQ((int)write(fd, &rec, half), (int)half);
  close(fd);

  PomodoroData out;
  memset(&out, 0xFF, sizeof(out));
  ErrorType err = LoadPomodoro(tmp, &out);
  ASSERT_EQ(err, NO_ERROR);

  remove(tmp);
}

static void test_load_garbage_file(void) {
  TEST("LoadPomodoro garbage binary does not crash");
  char tmp[] = "/tmp/t_log_ga_XXXXXX";
  int fd = mkstemp(tmp);
  ASSERT_GT(fd, -1);

  unsigned char garbage[] = {
    0xFF, 0xFE, 0x00, 0x01, 0xDE, 0xAD, 0xBE, 0xEF,
    0xBA, 0xDD, 0xCA, 0xFE, 0x00, 0x11, 0x22, 0x33
  };
  write(fd, garbage, sizeof(garbage));
  close(fd);

  PomodoroData out;
  memset(&out, 0xFF, sizeof(out));
  ErrorType err = LoadPomodoro(tmp, &out);
  ASSERT_EQ(err, NO_ERROR);

  remove(tmp);
}

static void test_save_pomodoro_nonexistent_dir(void) {
  TEST("SavePomodoro to nonexistent directory returns TIMER_LOG_ERROR");
  PomodoroData d;
  memset(&d, 0, sizeof(d));
  d.session_index = 1;
  d.total_cycles = 4;
  d.work_time = 25;

  ErrorType err = SavePomodoro("/nonexistent_dir_xyz/log.bin", &d, true);
  ASSERT_EQ(err, TIMER_LOG_ERROR);
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
  RUN_TEST(test_get_history_day_many_sessions,
           "GetPomodoroHistoryDay >100 sessions no crash");
  RUN_TEST(test_save_and_load_pomodoro,
           "SavePomodoro then LoadPomodoro fields match");
  RUN_TEST(test_save_pomodoro_append,
           "SavePomodoro append creates multiple records");
  RUN_TEST(test_get_last_log_index_empty,
           "GetLastLogIndexOnly empty file returns 1");
  RUN_TEST(test_get_last_log_index_after_save,
           "GetLastLogIndexOnly returns last session_index");
  RUN_TEST(test_remove_uncompleted_entries,
           "RemoveUncompletedEntries removes uncompleted entries");
  RUN_TEST(test_remove_uncompleted_all_completed,
           "RemoveUncompletedEntries all completed keeps all");
  RUN_TEST(test_load_empty_file,
           "LoadPomodoro empty file returns empty data");
  RUN_TEST(test_load_corrupted_file_partial_record,
           "LoadPomodoro partial record does not crash");
  RUN_TEST(test_load_garbage_file,
           "LoadPomodoro garbage binary does not crash");
  RUN_TEST(test_save_pomodoro_nonexistent_dir,
           "SavePomodoro to nonexistent dir returns TIMER_LOG_ERROR");
  return test_end();
}
