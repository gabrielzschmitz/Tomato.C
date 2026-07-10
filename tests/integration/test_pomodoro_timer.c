/**
 * @file test_pomodoro_timer.c
 * @brief Integration tests for pomodoro timer logic.
 *
 * Tests IsStepEnded boundary conditions, FormatRemainingTime output,
 * full 4-cycle pomodoro state machine transitions, and total elapsed
 * time accumulation across steps.
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "test_helpers.h"
#include "util.h"

Config g_config;

#define STEP_WORK 0
#define STEP_SHORT_PAUSE 1
#define STEP_LONG_PAUSE 2

typedef struct {
  int current_step;
  int current_step_time;
  int current_cycle;
  int total_cycles;
  int work_time;
  int short_pause_time;
  int long_pause_time;
  int total_elapsed;
  int status;
} TimerState;

static void reset_timer(TimerState* ts) {
  ts->current_step = STEP_WORK;
  ts->current_step_time = 0;
  ts->current_cycle = 1;
  ts->total_cycles = 4;
  ts->work_time = 25;
  ts->short_pause_time = 5;
  ts->long_pause_time = 30;
  ts->total_elapsed = 0;
  ts->status = 1;
}

static int step_duration_minutes(const TimerState* ts) {
  if (ts->current_step == STEP_WORK) return ts->work_time;
  if (ts->current_step == STEP_SHORT_PAUSE) return ts->short_pause_time;
  return ts->long_pause_time;
}

static void tick(TimerState* ts, int seconds) {
  ts->current_step_time += seconds;
  ts->total_elapsed += seconds;
}

static void advance_step(TimerState* ts) {
  ts->current_step_time = 0;
  if (ts->current_step == STEP_WORK) {
    if (ts->current_cycle >= ts->total_cycles) {
      ts->current_step = STEP_LONG_PAUSE;
    } else {
      ts->current_step = STEP_SHORT_PAUSE;
    }
  } else {
    ts->current_step = STEP_WORK;
    if (ts->current_cycle < ts->total_cycles) {
      ts->current_cycle++;
    }
  }
}

/**
 * ---------------------------------------------------------------------------
 * IsStepEnded boundary tests
 * ---------------------------------------------------------------------------
 */

static void test_is_step_ended_at_zero(void) {
  TEST("IsStepEnded returns false at time 0");
  ASSERT_FALSE(IsStepEnded(0, 25));
}

static void test_is_step_ended_just_before(void) {
  TEST("IsStepEnded returns false one second before end");
  ASSERT_FALSE(IsStepEnded(1499, 25));
}

static void test_is_step_ended_exactly_at(void) {
  TEST("IsStepEnded returns true exactly at boundary");
  ASSERT_TRUE(IsStepEnded(1500, 25));
}

static void test_is_step_ended_past(void) {
  TEST("IsStepEnded returns true past boundary");
  ASSERT_TRUE(IsStepEnded(1800, 25));
}

static void test_is_step_ended_zero_duration(void) {
  TEST("IsStepEnded with 0 duration ends immediately");
  ASSERT_TRUE(IsStepEnded(0, 0));
  ASSERT_TRUE(IsStepEnded(1, 0));
}

static void test_is_step_ended_negative_time(void) {
  TEST("IsStepEnded with negative time is not ended");
  ASSERT_FALSE(IsStepEnded(-1, 10));
}

/**
 * ---------------------------------------------------------------------------
 * FormatRemainingTime tests
 * ---------------------------------------------------------------------------
 */

static void test_format_remaining_zero(void) {
  TEST("FormatRemainingTime with time 0");
  char* result = FormatRemainingTime(0, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "25:00");
  free(result);
}

static void test_format_remaining_partial(void) {
  TEST("FormatRemainingTime with partial time");
  char* result = FormatRemainingTime(60, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "24:00");
  free(result);
}

static void test_format_remaining_past(void) {
  TEST("FormatRemainingTime past duration");
  char* result = FormatRemainingTime(1800, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "00:00");
  free(result);
}

static void test_format_remaining_exact(void) {
  TEST("FormatRemainingTime at exact duration");
  char* result = FormatRemainingTime(1500, 25);
  ASSERT_STR_EQ(result, "00:00");
  free(result);
}

static void test_format_remaining_short_pause(void) {
  TEST("FormatRemainingTime short pause");
  char* result = FormatRemainingTime(120, 5);
  ASSERT_STR_EQ(result, "03:00");
  free(result);
}

/**
 * ---------------------------------------------------------------------------
 * Full cycle validation
 * ---------------------------------------------------------------------------
 */

static void test_four_cycle_pomodoro(void) {
  TEST("4-cycle pomodoro ends on long pause after 4th work");
  TimerState ts;
  reset_timer(&ts);

  /* Cycle 1: Work -> ShortPause -> Work */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 2);

  /* Cycle 2: Work -> ShortPause -> Work */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 3);

  /* Cycle 3: Work -> ShortPause -> Work */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 4);

  /* Cycle 4: Work -> LongPause (last cycle) */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_LONG_PAUSE);
  ASSERT_EQ(ts.current_cycle, 4);
}

static void test_timer_total_elapsed_exact(void) {
  TEST("timer total elapsed matches exact minutes");
  TimerState ts;
  reset_timer(&ts);

  tick(&ts, ts.work_time * 60);
  ASSERT_EQ(ts.current_step_time, 1500);
  ASSERT_EQ(ts.total_elapsed, 1500);
  advance_step(&ts);

  tick(&ts, ts.short_pause_time * 60);
  ASSERT_EQ(ts.current_step_time, 300);
  ASSERT_EQ(ts.total_elapsed, 1800);
  advance_step(&ts);

  tick(&ts, ts.work_time * 60);
  ASSERT_EQ(ts.current_step_time, 1500);
  ASSERT_EQ(ts.total_elapsed, 3300);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("pomodoro_timer");
  RUN_TEST(test_is_step_ended_at_zero, "IsStepEnded false at time 0");
  RUN_TEST(test_is_step_ended_just_before,
           "IsStepEnded false 1s before boundary");
  RUN_TEST(test_is_step_ended_exactly_at, "IsStepEnded true at boundary");
  RUN_TEST(test_is_step_ended_past, "IsStepEnded true past boundary");
  RUN_TEST(test_is_step_ended_zero_duration,
           "IsStepEnded 0 duration ends immediately");
  RUN_TEST(test_is_step_ended_negative_time, "IsStepEnded with negative time");
  RUN_TEST(test_format_remaining_zero, "FormatRemainingTime at 0");
  RUN_TEST(test_format_remaining_partial, "FormatRemainingTime partial");
  RUN_TEST(test_format_remaining_past, "FormatRemainingTime past duration");
  RUN_TEST(test_format_remaining_exact, "FormatRemainingTime exact duration");
  RUN_TEST(test_format_remaining_short_pause,
           "FormatRemainingTime short pause");
  RUN_TEST(test_four_cycle_pomodoro, "4-cycle pomodoro ends on long pause");
  RUN_TEST(test_timer_total_elapsed_exact, "timer total elapsed match");
  return test_end();
}
