#ifndef LOG_H_
#define LOG_H_

#include <ncurses.h>

#include "error.h"
#include "tomato.h"

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

#endif /* LOG_H_ */
