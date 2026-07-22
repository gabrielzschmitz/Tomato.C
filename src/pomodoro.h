#ifndef POMODORO_H_
#define POMODORO_H_

#include "tomato.h"

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
                      int short_pause_time, int long_pause_time);

/**
 * Advance the pomodoro timer by a number of seconds.
 * Increments both the current step time and the total elapsed time.
 * @param pd Pointer to the pomodoro data
 * @param seconds Number of seconds to add
 */
void TickPomodoroTime(PomodoroData* pd, int seconds);

/**
 * Transition the pomodoro state machine to the next step.
 * WORK_TIME -> SHORT_PAUSE (or LONG_PAUSE on final cycle)
 * SHORT_PAUSE -> WORK_TIME (increments cycle counter)
 * LONG_PAUSE -> MAIN_MENU (resets cycle, marks session completed)
 * @param pd Pointer to the pomodoro data
 */
void TransitionPomodoroStep(PomodoroData* pd);

/**
 * Get the duration of the current pomodoro step in minutes.
 * Returns the configured work, short pause, or long pause duration
 * depending on pd->current_step.
 * @param pd Pointer to the pomodoro data
 * @return int Duration in minutes for the current step
 */
int GetStepDurationMinutes(const PomodoroData* pd);

#endif /* POMODORO_H_ */
