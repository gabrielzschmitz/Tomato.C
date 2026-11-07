/**
 * @file test_pomodoro.c
 * @brief Unit tests for the pomodoro timer helpers.
 *
 * Tests AdvancePomodoroClock fractional carry and TickPomodoroTime.
 */

#include <stdbool.h>
#include <string.h>

#include "pomodoro.h"
#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * AdvancePomodoroClock
 * ---------------------------------------------------------------------------
 */

/** @brief Nothing happens before a full interval has elapsed. */
static void test_clock_no_tick_before_interval(void) {
  TEST("AdvancePomodoroClock waits for a full interval");
  double last = 1000.0;
  ASSERT_EQ(AdvancePomodoroClock(&last, 1999.0, 1000.0), 0);
  ASSERT_EQ((int)last, 1000);
}

/** @brief The fractional remainder is kept instead of being discarded. */
static void test_clock_keeps_fraction(void) {
  TEST("AdvancePomodoroClock keeps the fractional remainder");
  double last = 0.0;

  ASSERT_EQ(AdvancePomodoroClock(&last, 1008.0, 1000.0), 1);
  ASSERT_EQ((int)last, 1000);

  ASSERT_EQ(AdvancePomodoroClock(&last, 2007.0, 1000.0), 1);
  ASSERT_EQ((int)last, 2000);

  ASSERT_EQ(AdvancePomodoroClock(&last, 3005.0, 1000.0), 1);
  ASSERT_EQ((int)last, 3000);
}

/** @brief A long frame gap catches up with every whole tick. */
static void test_clock_multiple_ticks(void) {
  TEST("AdvancePomodoroClock counts multiple ticks");
  double last = 0.0;
  ASSERT_EQ(AdvancePomodoroClock(&last, 2500.0, 1000.0), 2);
  ASSERT_EQ((int)last, 2000);
}

/** @brief Debug speed uses a shorter interval without drifting. */
static void test_clock_debug_interval(void) {
  TEST("AdvancePomodoroClock supports the debug interval");
  double last = 0.0;
  ASSERT_EQ(AdvancePomodoroClock(&last, 12.0, 10.0), 1);
  ASSERT_EQ((int)last, 10);
  ASSERT_EQ(AdvancePomodoroClock(&last, 19.0, 10.0), 0);
  ASSERT_EQ(AdvancePomodoroClock(&last, 21.0, 10.0), 1);
  ASSERT_EQ((int)last, 20);
}

/** @brief Invalid arguments are ignored. */
static void test_clock_invalid_args(void) {
  TEST("AdvancePomodoroClock rejects invalid arguments");
  double last = 42.0;
  ASSERT_EQ(AdvancePomodoroClock(NULL, 2000.0, 1000.0), 0);
  ASSERT_EQ(AdvancePomodoroClock(&last, 2000.0, 0.0), 0);
  ASSERT_EQ(AdvancePomodoroClock(&last, 2000.0, -1.0), 0);
  ASSERT_EQ((int)last, 42);
}

/**
 * ---------------------------------------------------------------------------
 * TickPomodoroTime
 * ---------------------------------------------------------------------------
 */

/** @brief Ticking advances both counters by the same amount. */
static void test_tick_updates_counters(void) {
  TEST("TickPomodoroTime updates step and total time");
  PomodoroData pd;
  memset(&pd, 0, sizeof(pd));
  TickPomodoroTime(&pd, 3);
  ASSERT_EQ(pd.current_step_time, 3);
  ASSERT_EQ(pd.total_elapsed, 3);
  TickPomodoroTime(&pd, 2);
  ASSERT_EQ(pd.current_step_time, 5);
  ASSERT_EQ(pd.total_elapsed, 5);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("pomodoro");
  RUN_TEST(test_clock_no_tick_before_interval,
           "no tick before a full interval");
  RUN_TEST(test_clock_keeps_fraction, "fractional remainder is kept");
  RUN_TEST(test_clock_multiple_ticks, "multiple ticks are counted");
  RUN_TEST(test_clock_debug_interval, "debug interval does not drift");
  RUN_TEST(test_clock_invalid_args, "invalid arguments are ignored");
  RUN_TEST(test_tick_updates_counters, "tick updates both counters");
  return test_end();
}
