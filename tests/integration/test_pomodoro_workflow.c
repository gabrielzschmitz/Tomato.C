/**
 * @file test_pomodoro_workflow.c
 * @brief Integration tests for the pomodoro timer state machine.
 *
 * Mirrors the step/time/transition logic from update.c's timer engine.
 * Tests initial state, tick advancement, work/short-pause/long-pause
 * transitions, cycle counting, and IsStepEnded boundary behaviour.
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

/**
 * ---------------------------------------------------------------------------
 * State machine helpers (mirroring update.c)
 * ---------------------------------------------------------------------------
 */
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
  ts->status = 1; /* uncompleted */
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
 * Tests
 * ---------------------------------------------------------------------------
 */
static void test_initial_state(void) {
  TEST("pomodoro starts in work step");
  TimerState ts;
  reset_timer(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 1);
  ASSERT_EQ(ts.total_elapsed, 0);
}

static void test_tick_advances_time(void) {
  TEST("tick advances step time and total elapsed");
  TimerState ts;
  reset_timer(&ts);
  tick(&ts, 60);
  ASSERT_EQ(ts.current_step_time, 60);
  ASSERT_EQ(ts.total_elapsed, 60);
}

static void test_work_to_short_pause(void) {
  TEST("work transitions to short pause");
  TimerState ts;
  reset_timer(&ts);
  tick(&ts, ts.work_time * 60); /* exactly at boundary */
  ASSERT_TRUE(IsStepEnded(ts.current_step_time, step_duration_minutes(&ts)));
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);
  ASSERT_EQ(ts.current_step_time, 0);
}

static void test_short_pause_to_work(void) {
  TEST("short pause transitions back to work");
  TimerState ts;
  reset_timer(&ts);
  ts.current_step = STEP_SHORT_PAUSE;
  ts.current_step_time = ts.short_pause_time * 60;
  ASSERT_TRUE(IsStepEnded(ts.current_step_time, step_duration_minutes(&ts)));
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 2);
}

static void test_long_pause_after_last_cycle(void) {
  TEST("last work cycle goes to long pause");
  TimerState ts;
  reset_timer(&ts);
  ts.current_cycle = ts.total_cycles;
  ts.current_step = STEP_WORK;
  ts.current_step_time = ts.work_time * 60;
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_LONG_PAUSE);
}

static void test_cycle_increments_after_pause(void) {
  TEST("cycle increments after short pause");
  TimerState ts;
  reset_timer(&ts);
  ts.current_cycle = 1;

  /* Work -> Short Pause */
  ts.current_step_time = ts.work_time * 60;
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);

  /* Short Pause -> Work (cycle increments) */
  ts.current_step_time = ts.short_pause_time * 60;
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 2);
}

static void test_full_cycle_all_steps(void) {
  TEST("all steps in a full pomodoro cycle");
  TimerState ts;
  reset_timer(&ts);

  /* Cycle 1: Work */
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 1);

  /* Cycle 1: Short Pause */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_SHORT_PAUSE);

  /* Cycle 2: Work */
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_WORK);
  ASSERT_EQ(ts.current_cycle, 2);

  /* Cycle 2: Short Pause */
  advance_step(&ts);

  /* After 4 cycles, last work goes to long pause */
  ts.current_cycle = 4;
  ts.current_step = STEP_WORK;
  advance_step(&ts);
  ASSERT_EQ(ts.current_step, STEP_LONG_PAUSE);
}

static void test_is_step_ended_utility(void) {
  TEST("IsStepEnded used by timer (boundary)");
  ASSERT_TRUE(IsStepEnded(1500, 25)); /* 1500s = 25min */
  ASSERT_FALSE(IsStepEnded(1499, 25));
  ASSERT_TRUE(IsStepEnded(300, 5));
  ASSERT_TRUE(IsStepEnded(1800, 30));
}

static void test_total_elapsed_accumulates(void) {
  TEST("total elapsed accumulates across ticks");
  TimerState ts;
  reset_timer(&ts);
  tick(&ts, 60);
  tick(&ts, 120);
  tick(&ts, 30);
  ASSERT_EQ(ts.total_elapsed, 210);
}

int main(void) {
  test_begin("pomodoro_workflow");
  RUN_TEST(test_initial_state, "pomodoro starts in work step");
  RUN_TEST(test_tick_advances_time,
           "tick advances step time and total elapsed");
  RUN_TEST(test_work_to_short_pause, "work transitions to short pause");
  RUN_TEST(test_short_pause_to_work, "short pause transitions back to work");
  RUN_TEST(test_long_pause_after_last_cycle,
           "last work cycle goes to long pause");
  RUN_TEST(test_cycle_increments_after_pause,
           "cycle increments after short pause");
  RUN_TEST(test_full_cycle_all_steps, "all steps in a full pomodoro cycle");
  RUN_TEST(test_is_step_ended_utility, "IsStepEnded used by timer (boundary)");
  RUN_TEST(test_total_elapsed_accumulates,
           "total elapsed accumulates across ticks");
  return test_end();
}
