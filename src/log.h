#ifndef LOG_H_
#define LOG_H_

#include <ncurses.h>
#include <stdint.h>

#include "error.h"
#include "tomato.h"

/**
 * Binary log record structure (packed, 28 bytes).
 */
typedef struct __attribute__((packed)) {
  uint16_t session_index; /**< Unique session identifier */
  uint8_t
    current_step; /**< Current step (0=work, 1=short pause, 2=long pause) */
  uint8_t current_cycle;       /**< Current cycle number (1-based) */
  uint8_t total_cycles;        /**< Total pomodoro cycles configured */
  uint8_t work_time;           /**< Work time duration in minutes */
  uint8_t short_pause_time;    /**< Short pause duration in minutes */
  uint8_t long_pause_time;     /**< Long pause duration in minutes */
  uint32_t total_elapsed;      /**< Total elapsed time across steps (seconds) */
  uint32_t current_step_time;  /**< Elapsed time in current step (seconds) */
  uint8_t status;              /**< 0 = completed, 1 = uncompleted */
  uint32_t session_start_time; /**< Unix timestamp when session started */
} pomodoroLogRecord;

/**
 * ---------------------------------------------------------------------------
 * Timer
 * ---------------------------------------------------------------------------
 */

/**
 * Create a timer log server using Unix domain sockets.
 * Server listens for connections and serves timer data.
 * @param path File path for the Unix socket
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType CreateTimerLog(const char* path);

/**
 * Get the timer log from the socket (client-side).
 * Connects to the timer log server and reads current status.
 * @param path File path for the Unix socket
 * @param loop Whether to continuously poll the socket
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType GetTimerLog(const char* path, bool loop);

/**
 * Set a timer log by writing to the socket.
 * Sends timer data to the Unix socket server.
 * @param path File path for the Unix socket
 * @param log Formatted log string to write
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType SetTimerLog(const char* path, const char* log);

/**
 * Format pomodoro data into a timer log string.
 * Creates a human-readable string for timer status display.
 * @param data Pomodoro timer data to format
 * @param is_paused Whether the timer is currently paused
 * @return Newly allocated formatted string (caller must free), or NULL on failure
 */
char* FormatTimerLog(PomodoroData data, bool is_paused);

/**
 * ---------------------------------------------------------------------------
 * Notes
 * ---------------------------------------------------------------------------
 */

/**
 * Save notes to a file for persistence.
 * Writes the current notes data structure to disk.
 * @param path File path for the notes log
 * @param notes Pointer to the notes data to save
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType SaveNotes(const char* path, const NotesData* notes);

/**
 * Load notes from a file for persistence.
 * Reads notes data from disk and populates the notes structure.
 * @param path File path for the notes log
 * @param notes Pointer to the notes data to populate
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType LoadNotes(const char* path, NotesData* notes);

/**
 * ---------------------------------------------------------------------------
 * Pomodoro
 * ---------------------------------------------------------------------------
 */

/**
 * Save current pomodoro state to a log file.
 * Writes timestamp and all pomodoro data fields.
 * @param path File path for the pomodoro log
 * @param data Pointer to the pomodoro data to save
 * @param append If true, append new line; if false, update last line
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType SavePomodoro(const char* path, const PomodoroData* data, bool append);

/**
 * Get the last used index from the pomodoro log.
 * @param path File path for the pomodoro log
 * @return Last used index (1 if file is empty)
 */
int GetLastLogIndexOnly(const char* path);

/**
 * Remove uncompleted entries for a given index.
 * @param path File path for the pomodoro log
 * @param index Index to remove uncompleted entries for
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType RemoveUncompletedEntries(const char* path, int index);

/**
 * Load pomodoro state from a log file.
 * Reads the last line and restores pomodoro data.
 * @param path File path for the pomodoro log
 * @param data Pointer to the pomodoro data to populate
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType LoadPomodoro(const char* path, PomodoroData* data);

/**
 * Print pomodoro history statistics from the log file.
 * @param path File path for the pomodoro log
 */
void GetPomodoroHistory(const char* path);

/**
 * ---------------------------------------------------------------------------
 * History
 * ---------------------------------------------------------------------------
 */

/**
 * Returns the number of days in a given month.
 * @param year  Gregorian year (e.g. 2026)
 * @param month Month (1-12)
 * @return Days in month (28-31)
 */
int HistDaysInMonth(int year, int month);

/**
 * Returns the day-of-week for a date.
 * @param year  Gregorian year
 * @param month Month (1-12)
 * @param day   Day (1-31)
 * @return 0=Sunday .. 6=Saturday
 */
int HistDayOfWeek(int year, int month, int day);

/**
 * Fills daily session-count array for a given month from the binary log.
 * @param path   Binary log path (POMODORO_LOG)
 * @param year   Year
 * @param month  Month (1-12)
 * @param counts Output array[31] — count per day (0 for days outside month)
 * @return Days in month (same as length of valid entries in counts)
 */
int HistDailyCounts(const char* path, int year, int month, int* counts);

/**
 * Returns session records for a specific day from the binary log.
 * @param path      Binary log path (POMODORO_LOG)
 * @param year      Year
 * @param month     Month (1-12)
 * @param day       Day (1-31)
 * @param indices   Output array of session indices
 * @param startTimes Output array of unix timestamps
 * @param durations Output array of durations in seconds
 * @param statuses  Output array (0=completed, 1=uncompleted)
 * @param maxCount  Capacity of output arrays
 * @return Number of sessions found (capped at maxCount)
 */
int HistSessionsForDay(const char* path, int year, int month, int day,
                       int* indices, time_t* startTimes, int* durations,
                       int* statuses, int maxCount);

/**
 * Computes current and longest streak ending at the given date.
 * A streak is consecutive calendar days (past to present) with at
 * least one completed session.
 * @param path    Binary log path (POMODORO_LOG)
 * @param year    Year of streak endpoint
 * @param month   Month of streak endpoint
 * @param day     Day of streak endpoint
 * @param current Output — current streak length
 * @param longest Output — longest streak ever
 */
void HistStreak(const char* path, int year, int month, int day, int* current,
                int* longest);

/**
 * Maps session count to contribution-icon index.
 *
 * Boundaries:
 *   0   → 0 ("░░")
 *   1-2 → 1 ("▒▒")
 *   3-5 → 2 ("▓▓")
 *   6+  → 3 ("██")
 *
 * @param count Number of sessions
 * @return Icon index 0-3
 */
int HistLevelForCount(int count);

#endif /* LOG_H_ */
