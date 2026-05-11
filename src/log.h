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

#endif /* LOG_H_ */
