#ifndef POMODORO_H_
#define POMODORO_H_

#include <time.h>

/**
 * Pomodoro timer data structure.
 * Tracks the current state of the pomodoro cycle.
 */
typedef struct PomodoroData {
  int total_cycles;     /**< Total number of pomodoro cycles to complete */
  int current_cycle;    /**< Current cycle number (1 to total_cycles) */
  int work_time;        /**< Work time duration in minutes */
  int short_pause_time; /**< Short pause duration in minutes */
  int long_pause_time;  /**< Long pause duration in minutes */
  int current_step; /**< Current step (0=work, 1=short pause, 2=long pause) */
  int current_step_time;  /**< Elapsed time in current step (seconds) */
  int last_step_time;     /**< Time from previous step for comparison */
  int total_elapsed;      /**< Total elapsed time across all steps (seconds) */
  double delta_time_ms;   /**< Elapsed ms since last frame update */
  int session_index;      /**< Current session index for log entries */
  time_t step_start_time; /**< Timestamp when current step started */
  time_t session_start_time; /**< Timestamp when the session was started */
  int status;                /**< 0 = completed, 1 = uncompleted */
} PomodoroData;

/**
 * ---------------------------------------------------------------------------
 * Pomodoro State Machine
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize pomodoro timer data with given configuration values.
 * Resets the state machine to the initial WORK_TIME step.
 * @param pd Pointer to the pomodoro data to initialize.
 * @param total_cycles Number of work sessions before a long break.
 * @param work_time Duration of each work session in minutes.
 * @param short_pause_time Duration of a short pause in minutes.
 * @param long_pause_time Duration of a long pause in minutes.
 */
void InitPomodoroData(PomodoroData* pd, int total_cycles, int work_time,
                      int short_pause_time, int long_pause_time);

/**
 * Advance the pomodoro timer by a number of seconds.
 * Increments both the current step time and the total elapsed time.
 * @param pd Pointer to the pomodoro data.
 * @param seconds Number of seconds to add.
 */
void TickPomodoroTime(PomodoroData* pd, int seconds);

/**
 * Compute how many whole timer ticks elapsed and advance the reference.
 *
 * The reference is advanced by whole intervals only, so the fractional
 * remainder is kept for the next call instead of being discarded.
 * @param last_ms Pointer to the last reference time in milliseconds.
 * @param now_ms Current time in milliseconds.
 * @param interval_ms Duration of one tick in milliseconds.
 * @return Number of whole ticks elapsed, or 0 if the interval has not passed.
 */
int AdvancePomodoroClock(double* last_ms, double now_ms, double interval_ms);

/**
 * Transition the pomodoro state machine to the next step.
 * WORK_TIME -> SHORT_PAUSE (or LONG_PAUSE on final cycle)
 * SHORT_PAUSE -> WORK_TIME (increments cycle counter)
 * LONG_PAUSE -> MAIN_MENU (resets cycle, marks session completed)
 * @param pd Pointer to the pomodoro data.
 */
void TransitionPomodoroStep(PomodoroData* pd);

/**
 * Get the duration of the current pomodoro step in minutes.
 * Returns the configured work, short pause, or long pause duration
 * depending on pd->current_step.
 * @param pd Pointer to the pomodoro data.
 * @return int Duration in minutes for the current step.
 */
int GetStepDurationMinutes(const PomodoroData* pd);

#endif /* POMODORO_H_ */
