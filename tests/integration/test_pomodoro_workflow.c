/**
 * @file test_pomodoro_workflow.c
 * @brief Integration tests for the pomodoro timer state machine.
 *
 * Tests initial state, tick advancement, work/short-pause/long-pause
 * transitions, cycle counting, and IsStepEnded boundary behaviour.
 * Uses the real PomodoroData type and pomodoro.c functions.
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
 * Initial state
 * ---------------------------------------------------------------------------
 */

/**
 * Verify that InitPomodoroData sets the initial step to WORK_TIME
 * with current_cycle = 0 and total_elapsed = 0.
 */
static void test_initial_state(void) {
  TEST("pomodoro starts in work step");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 0);
  ASSERT_EQ(pd.total_elapsed, 0);
}

/**
 * ---------------------------------------------------------------------------
 * Tick
 * ---------------------------------------------------------------------------
 */

/**
 * Verify that TickPomodoroTime advances both current_step_time and
 * total_elapsed by the given number of seconds.
 */
static void test_tick_advances_time(void) {
  TEST("tick advances step time and total elapsed");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  TickPomodoroTime(&pd, 60);
  ASSERT_EQ(pd.current_step_time, 60);
  ASSERT_EQ(pd.total_elapsed, 60);
}

/**
 * ---------------------------------------------------------------------------
 * Transitions
 * ---------------------------------------------------------------------------
 */

/**
 * Verify that WORK_TIME transitions to SHORT_PAUSE at the end of a work
 * session, and that current_step_time resets to 0.
 */
static void test_work_to_short_pause(void) {
  TEST("work transitions to short pause");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  TickPomodoroTime(&pd, pd.work_time * 60);
  ASSERT_TRUE(IsStepEnded(pd.current_step_time, GetStepDurationMinutes(&pd)));
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  ASSERT_EQ(pd.current_step_time, 0);
}

/**
 * Verify that SHORT_PAUSE transitions back to WORK_TIME and
 * increments the cycle counter.
 */
static void test_short_pause_to_work(void) {
  TEST("short pause transitions back to work");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  pd.current_step = SHORT_PAUSE;
  pd.current_step_time = pd.short_pause_time * 60;
  ASSERT_TRUE(IsStepEnded(pd.current_step_time, GetStepDurationMinutes(&pd)));
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 1);
}

/**
 * Verify that the last work cycle transitions to LONG_PAUSE instead
 * of SHORT_PAUSE.
 */
static void test_long_pause_after_last_cycle(void) {
  TEST("last work cycle goes to long pause");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  pd.current_cycle = pd.total_cycles - 1;
  pd.current_step_time = pd.work_time * 60;
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, LONG_PAUSE);
}

/**
 * Verify that the cycle counter increments after completing a
 * work → short-pause → work sequence.
 */
static void test_cycle_increments_after_pause(void) {
  TEST("cycle increments after short pause");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  pd.current_step_time = pd.work_time * 60;
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  pd.current_step_time = pd.short_pause_time * 60;
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 1);
}

/**
 * Execute a full pomodoro cycle: work → short-pause → work → ...
 * → long pause, verifying every step and cycle count along the way.
 */
static void test_full_cycle_all_steps(void) {
  TEST("all steps in a full pomodoro cycle");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);

  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 0);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);
  ASSERT_EQ(pd.current_cycle, 0);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, WORK_TIME);
  ASSERT_EQ(pd.current_cycle, 1);

  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, SHORT_PAUSE);

  pd.current_cycle = pd.total_cycles - 1;
  pd.current_step = WORK_TIME;
  TransitionPomodoroStep(&pd);
  ASSERT_EQ(pd.current_step, LONG_PAUSE);
}

/**
 * ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------
 */

/**
 * Verify IsStepEnded boundary behaviour used by the timer engine.
 */
static void test_is_step_ended_utility(void) {
  TEST("IsStepEnded used by timer (boundary)");
  ASSERT_TRUE(IsStepEnded(1500, 25));
  ASSERT_FALSE(IsStepEnded(1499, 25));
  ASSERT_TRUE(IsStepEnded(300, 5));
  ASSERT_TRUE(IsStepEnded(1800, 30));
}

/**
 * Verify that total elapsed time accumulates correctly across
 * multiple TickPomodoroTime calls.
 */
static void test_total_elapsed_accumulates(void) {
  TEST("total elapsed accumulates across ticks");
  PomodoroData pd;
  InitPomodoroData(&pd, 4, 25, 5, 30);
  TickPomodoroTime(&pd, 60);
  TickPomodoroTime(&pd, 120);
  TickPomodoroTime(&pd, 30);
  ASSERT_EQ(pd.total_elapsed, 210);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("pomodoro_workflow");
  RUN_TEST(test_initial_state, "pomodoro starts in work step");
  RUN_TEST(test_tick_advances_time,
           "tick advances step time and total elapsed");
  RUN_TEST(test_work_to_short_pause, "work transitions to short pause");
  RUN_TEST(test_short_pause_to_work,
           "short pause transitions back to work");
  RUN_TEST(test_long_pause_after_last_cycle,
           "last work cycle goes to long pause");
  RUN_TEST(test_cycle_increments_after_pause,
           "cycle increments after short pause");
  RUN_TEST(test_full_cycle_all_steps,
           "all steps in a full pomodoro cycle");
  RUN_TEST(test_is_step_ended_utility,
           "IsStepEnded used by timer (boundary)");
  RUN_TEST(test_total_elapsed_accumulates,
           "total elapsed accumulates across ticks");
  return test_end();
}
