#include "pomodoro.h"

#include "input.h"

/**
 * ---------------------------------------------------------------------------
 * Pomodoro State Machine
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize pomodoro timer data with given configuration values.
 * Resets the state machine to the initial WORK_TIME step.
 * @param pd Pointer to the pomodoro data to initialize
 * @param total_cycles Number of work sessions before a long break
 * @param work_time Duration of each work session in minutes
 * @param short_pause_time Duration of a short pause in minutes
 * @param long_pause_time Duration of a long pause in minutes
 */
void InitPomodoroData(PomodoroData* pd, int total_cycles, int work_time,
                      int short_pause_time, int long_pause_time) {
  pd->total_cycles = total_cycles;
  pd->current_cycle = 0;
  pd->work_time = work_time;
  pd->short_pause_time = short_pause_time;
  pd->long_pause_time = long_pause_time;
  pd->current_step = WORK_TIME;
  pd->current_step_time = 0;
  pd->last_step_time = 0;
  pd->total_elapsed = 0;
  pd->delta_time_ms = 0.0;
  pd->session_index = 0;
  pd->step_start_time = 0;
  pd->session_start_time = 0;
  pd->status = 1;
}

/**
 * Advance the pomodoro timer by a number of seconds.
 * Increments both the current step time and the total elapsed time.
 * @param pd Pointer to the pomodoro data
 * @param seconds Number of seconds to add
 */
void TickPomodoroTime(PomodoroData* pd, int seconds) {
  pd->current_step_time += seconds;
  pd->total_elapsed += seconds;
}

/**
 * Transition the pomodoro state machine to the next step.
 *
 * The transition rules are:
 * - WORK_TIME -> SHORT_PAUSE (normal), or LONG_PAUSE on the final cycle
 * - SHORT_PAUSE -> WORK_TIME (cycle counter advances)
 * - LONG_PAUSE -> MAIN_MENU (cycle resets, session marked completed)
 *
 * @param pd Pointer to the pomodoro data
 */
void TransitionPomodoroStep(PomodoroData* pd) {
  pd->current_step_time = 0;
  if (pd->current_step == WORK_TIME) {
    if (pd->current_cycle >= pd->total_cycles - 1) {
      pd->current_step = LONG_PAUSE;
    } else {
      pd->current_step = SHORT_PAUSE;
    }
  } else if (pd->current_step == SHORT_PAUSE) {
    pd->current_step = WORK_TIME;
    pd->current_cycle += 1;
  } else {
    pd->current_step = MAIN_MENU;
    pd->current_cycle = 0;
    pd->total_elapsed = 0;
    pd->status = 0;
  }
}

/**
 * Get the duration of the current pomodoro step in minutes.
 * Returns the configured work, short pause, or long pause duration
 * depending on pd->current_step.
 * @param pd Pointer to the pomodoro data
 * @return int Duration in minutes for the current step
 */
int GetStepDurationMinutes(const PomodoroData* pd) {
  if (pd->current_step == WORK_TIME) return pd->work_time;
  if (pd->current_step == SHORT_PAUSE) return pd->short_pause_time;
  return pd->long_pause_time;
}
