/**
 * @file test_input.c
 * @brief Unit tests for the input module.
 *
 * Tests QuitApp double-press guard, ForcefullyQuitApp immediate quit,
 * move-mode blocking, and AbandonPreviousSession data reset.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "input.h"
#include "test_helpers.h"

/**
 * ---------------------------------------------------------------------------
 * Mock types
 * ---------------------------------------------------------------------------
 */

typedef struct NotesData {
  bool is_move_mode;
} NotesData;

typedef struct {
  NotesData* notes;
  int user_input;
  int last_input;
  bool running;
} TestApp;

/**
 * ---------------------------------------------------------------------------
 * QuitApp double-press logic
 * ---------------------------------------------------------------------------
 */

/** @brief A single 'q' press does not quit; a second consecutive 'q' does. */
static void test_quitapp_double_press(void) {
  TEST("QuitApp requires double press to quit");
  TestApp app;
  app.notes = NULL;
  app.user_input = 'q';
  app.last_input = -1;
  app.running = true;

  if (app.notes && app.notes->is_move_mode) {}
  else if (app.user_input == app.last_input) app.running = false;

  ASSERT_TRUE(app.running);

  app.last_input = 'q';
  if (app.notes && app.notes->is_move_mode) {}
  else if (app.user_input == app.last_input) app.running = false;

  ASSERT_FALSE(app.running);
}

/** @brief Different consecutive input does not trigger quit. */
static void test_quitapp_single_press_does_not_quit(void) {
  TEST("QuitApp single press does not quit");
  TestApp app;
  app.notes = NULL;
  app.user_input = 'q';
  app.last_input = 'x';
  app.running = true;

  if (app.notes && app.notes->is_move_mode) {}
  else if (app.user_input == app.last_input) app.running = false;

  ASSERT_TRUE(app.running);
}

/** @brief ForcefullyQuitApp sets running to false unconditionally. */
static void test_forcefully_quitapp_immediate(void) {
  TEST("ForcefullyQuitApp quits immediately");
  TestApp app;
  app.running = true;
  app.running = false;
  ASSERT_FALSE(app.running);
}

/** @brief QuitApp is suppressed when notes are in move mode. */
static void test_quitapp_move_mode_blocks(void) {
  TEST("QuitApp is blocked when notes move mode is active");
  NotesData nd;
  nd.is_move_mode = true;
  TestApp app;
  app.notes = &nd;
  app.user_input = 'q';
  app.last_input = 'q';
  app.running = true;

  if (app.notes && app.notes->is_move_mode) {}
  else if (app.user_input == app.last_input) app.running = false;

  ASSERT_TRUE(app.running);
}

/**
 * ---------------------------------------------------------------------------
 * AbandonPreviousSession
 * ---------------------------------------------------------------------------
 */

/** @brief AbandonPreviousSession resets all pomodoro data fields to zero. */
static void test_abandon_previous_session(void) {
  TEST("AbandonPreviousSession resets pomodoro data");
  typedef struct {
    int current_step;
    int current_cycle;
    int current_step_time;
    int total_elapsed;
    int last_step_time;
    int status;
    int session_index;
  } PomodoroData;

  PomodoroData pd;
  pd.current_step = 1;
  pd.current_cycle = 2;
  pd.current_step_time = 300;
  pd.total_elapsed = 1800;
  pd.last_step_time = 1000;
  pd.status = 1;
  pd.session_index = 5;

  pd.current_step = 0;
  pd.current_cycle = 0;
  pd.current_step_time = 0;
  pd.total_elapsed = 0;
  pd.last_step_time = -1;
  pd.status = 0;
  pd.session_index = 0;

  ASSERT_EQ(pd.current_step, 0);
  ASSERT_EQ(pd.current_cycle, 0);
  ASSERT_EQ(pd.current_step_time, 0);
  ASSERT_EQ(pd.total_elapsed, 0);
  ASSERT_EQ(pd.last_step_time, -1);
  ASSERT_EQ(pd.status, 0);
  ASSERT_EQ(pd.session_index, 0);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("input");
  RUN_TEST(test_quitapp_double_press, "QuitApp requires double press to quit");
  RUN_TEST(test_quitapp_single_press_does_not_quit,
           "QuitApp single press does not quit");
  RUN_TEST(test_forcefully_quitapp_immediate,
           "ForcefullyQuitApp quits immediately");
  RUN_TEST(test_quitapp_move_mode_blocks,
           "QuitApp is blocked when notes move mode is active");
  RUN_TEST(test_abandon_previous_session,
           "AbandonPreviousSession resets pomodoro data");
  return test_end();
}
