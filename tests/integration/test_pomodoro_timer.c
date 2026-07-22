/**
 * @file test_pomodoro_timer.c
 * @brief Integration tests for pomodoro timer utility functions.
 *
 * Tests boundary conditions of IsStepEnded, FormatRemainingTime output
 * formatting, full 4-cycle pomodoro state machine transitions, and
 * total elapsed time accumulation across steps.  Uses the real
 * PomodoroData type and pomodoro.c functions (no mirroring).
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "pomodoro.h"
#include "test_helpers.h"
#include "util.h"

/* Stub — util.c may call LogError on allocation failures */
Config g_config;
void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

/**
 * ---------------------------------------------------------------------------
 * IsStepEnded boundary tests
 * ---------------------------------------------------------------------------
 */

/**
 * Test that IsStepEnded returns false when current_step_time is 0.
 */
static void test_is_step_ended_at_zero(void) {
  TEST("IsStepEnded returns false at time 0");
  ASSERT_FALSE(IsStepEnded(0, 25));
}

/**
 * Test that IsStepEnded returns false one second before the boundary.
 */
static void test_is_step_ended_just_before(void) {
  TEST("IsStepEnded returns false one second before end");
  ASSERT_FALSE(IsStepEnded(1499, 25));
}

/**
 * Test that IsStepEnded returns true exactly at the boundary.
 */
static void test_is_step_ended_exactly_at(void) {
  TEST("IsStepEnded returns true exactly at boundary");
  ASSERT_TRUE(IsStepEnded(1500, 25));
}

/**
 * Test that IsStepEnded returns true past the boundary.
 */
static void test_is_step_ended_past(void) {
  TEST("IsStepEnded returns true past boundary");
  ASSERT_TRUE(IsStepEnded(1800, 25));
}

/**
 * Test that IsStepEnded returns true for any time when duration is 0.
 */
static void test_is_step_ended_zero_duration(void) {
  TEST("IsStepEnded with 0 duration ends immediately");
  ASSERT_TRUE(IsStepEnded(0, 0));
  ASSERT_TRUE(IsStepEnded(1, 0));
}

/**
 * Test that IsStepEnded returns false for negative time values.
 */
static void test_is_step_ended_negative_time(void) {
  TEST("IsStepEnded with negative time is not ended");
  ASSERT_FALSE(IsStepEnded(-1, 10));
}

/**
 * ---------------------------------------------------------------------------
 * FormatRemainingTime tests
 * ---------------------------------------------------------------------------
 */

/**
 * Test FormatRemainingTime output when no time has elapsed.
 */
static void test_format_remaining_zero(void) {
  TEST("FormatRemainingTime with time 0");
  char* result = FormatRemainingTime(0, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "25:00");
  free(result);
}

/**
 * Test FormatRemainingTime output with partial elapsed time.
 */
static void test_format_remaining_partial(void) {
  TEST("FormatRemainingTime with partial time");
  char* result = FormatRemainingTime(60, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "24:00");
  free(result);
}

/**
 * Test FormatRemainingTime output when elapsed exceeds duration.
 */
static void test_format_remaining_past(void) {
  TEST("FormatRemainingTime past duration");
  char* result = FormatRemainingTime(1800, 25);
  ASSERT_NOT_NULL(result);
  ASSERT_STR_EQ(result, "00:00");
  free(result);
}

/**
 * Test FormatRemainingTime output at exact duration boundary.
 */
static void test_format_remaining_exact(void) {
  TEST("FormatRemainingTime at exact duration");
  char* result = FormatRemainingTime(1500, 25);
  ASSERT_STR_EQ(result, "00:00");
  free(result);
}

/**
 * Test FormatRemainingTime output for a short pause duration.
 */
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

/**
 * Execute a full 4-cycle pomodoro via TransitionPomodoroStep.
 * Verifies the correct step sequence: work → short pause → work → ...
 * → long pause after the 4th work session.
 */
static void test_four_cycle_pomodoro(void) {
  TEST("4-cycle pomodoro ends on long pause after 4th work");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 1);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 2);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 3);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, LONG_PAUSE);
  ASSERT_EQ(pd.current_cycle, 3);
}

/**
 * Verify that TickPomodoroTime accumulates elapsed time correctly
 * across a work → short-pause → work sequence.
 */
static void test_timer_total_elapsed_exact(void) {
  TEST("timer total elapsed matches exact minutes");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);

  TickPomodoroTime(&pd, pd.work_time * 60);
  ASSERT_EQ(pd.current_step_time, 1500);
  ASSERT_EQ(pd.total_elapsed, 1500);
  TransitionPomodoroStep(&pd);

  TickPomodoroTime(&pd, pd.short_pause_time * 60);
  ASSERT_EQ(pd.current_step_time, 300);
  ASSERT_EQ(pd.total_elapsed, 1800);
  TransitionPomodoroStep(&pd);

  TickPomodoroTime(&pd, pd.work_time * 60);
  ASSERT_EQ(pd.current_step_time, 1500);
  ASSERT_EQ(pd.total_elapsed, 3300);
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
