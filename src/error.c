#include "error.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* PRIVATE VARIABLES */
static ErrorEntry error_stack[MAX_ERROR_ENTRIES] = {0};
static int error_count = 0;
static bool app_frozen = false;

/* PRIVATE ERROR FUNCTIONS */
static const char* getErrorMessage(ErrorType error);
static ErrorLevel getErrorLevel(ErrorType error);
static const char* getErrorLevelMessage(ErrorLevel level);
static int getErrorColor(ErrorLevel level);
static int getErrorTimeout(ErrorLevel level);

/**
 * Log an error with context information.
 * Writes to the error log file with timestamp and context.
 * @param context String describing where the error occurred
 * @param error The error type
 */
void LogError(const char* context, ErrorType error) {
  const char* error_message = getErrorMessage(error);
  const char* error_level = getErrorLevelMessage(getErrorLevel(error));

  char message[256];
  snprintf(message, sizeof(message), "[%s] %s: %s (Code: %d)", error_level,
           context, error_message, error);

  FILE* logFile = fopen(ERROR_LOG, "a");
  if (logFile != NULL) {
    fprintf(logFile, "%s\n", message);
    fclose(logFile);
  } else
    fprintf(stderr, "[CRITICAL] Failed to open log file\n");
}

/**
 * Set and display an error at the bottom of the screen.
 * Adds error to the display stack and logs to file.
 * If error level is CRITICAL, freezes the app and shows quit confirmation.
 * @param app Pointer to application data (used for CRITICAL freeze)
 * @param context String describing where the error occurred
 * @param type The error type
 */
void SetError(AppData* app, const char* context, ErrorType type) {
  ErrorLevel level = getErrorLevel(type);
  const char* error_message = getErrorMessage(type);
  const char* error_level = getErrorLevelMessage(level);

  char message[256];
  snprintf(message, sizeof(message), "[%s] %s: %s", error_level, context,
           error_message);

  for (int i = MAX_ERROR_ENTRIES - 1; i > 0; i--) {
    error_stack[i] = error_stack[i - 1];
  }

  error_stack[0].timestamp = time(NULL);
  error_stack[0].level = level;
  memset(error_stack[0].message, 0, sizeof(error_stack[0].message));
  snprintf(error_stack[0].message, sizeof(error_stack[0].message), "%s",
           message);

  if (error_count < MAX_ERROR_ENTRIES) error_count++;
  if (error_count > MAX_ERROR_ENTRIES) error_count = MAX_ERROR_ENTRIES;

  LogError(context, type);

  if (level == ERROR_LEVEL_CRITICAL) {
    app_frozen = true;
    if (app != NULL) {
      app->frozen = true;
      RenderCriticalQuitConfirmation(app);
    }
  }
}

/**
 * Clear all errors from the error stack.
 * Resets the error display and allows normal app operation.
 */
void ClearErrors(void) {
  error_count = 0;
  memset(error_stack, 0, sizeof(error_stack));
}

/**
 * Render all errors in the stack at the bottom line of the screen.
 * Displays only the first error with timeout countdown.
 * Format: [N][LEVEL] message (Xs) where N is count of remaining errors.
 * When timeout reaches 0 and there are more errors, shifts next to front.
 * Auto-clears based on timeout: INFO=5s, WARNING=15s, ERROR/DEBUG=30s,
 * CRITICAL=never.
 */
void RenderErrorLine(void) {
  if (error_count == 0) return;

  int screen_width = COLS;
  int y = LINES - 1;
  time_t now = time(NULL);

  ErrorLevel level = error_stack[0].level;
  int timeout = getErrorTimeout(level);
  int elapsed = (int)(now - error_stack[0].timestamp);

  if (timeout > 0 && elapsed >= timeout) {
    for (int j = 0; j < error_count - 1; j++) {
      error_stack[j] = error_stack[j + 1];
      error_stack[j].timestamp = time(NULL);
    }
    error_count--;
    return;
  }

  int remaining_count = error_count;

  char level_str[16];
  snprintf(level_str, sizeof(level_str), "[%s]", getErrorLevelMessage(level));

  int timeout_remaining = 0;
  char timeout_str[16];

  if (level == ERROR_LEVEL_CRITICAL) {
    snprintf(timeout_str, sizeof(timeout_str), "(%ds)", elapsed);
  } else if (timeout > 0) {
    timeout_remaining = timeout - elapsed;
    if (timeout_remaining < 0) timeout_remaining = 0;
    snprintf(timeout_str, sizeof(timeout_str), "(%ds)", timeout_remaining);
  } else {
    snprintf(timeout_str, sizeof(timeout_str), "(--)");
  }

  char count_str[16] = "";
  int count_len = 0;
  if (remaining_count > 1) {
    snprintf(count_str, sizeof(count_str), "[%d]", remaining_count);
    count_len = strlen(count_str);
  }

  int count_str_len = count_len;
  int level_len = strlen(level_str);
  int msg_len = strlen(error_stack[0].message);
  int timeout_len = strlen(timeout_str);
  int total_len = count_str_len + 1 + level_len + 1 + msg_len + 1 + timeout_len;

  if (total_len > screen_width) {
    int max_msg_len =
      screen_width - count_str_len - 1 - level_len - 1 - 1 - timeout_len;
    if (max_msg_len < 0) max_msg_len = 0;
    msg_len = max_msg_len;
  }

  int x_pos = 0;

  int fg = getErrorColor(level);
  int bg = (level == ERROR_LEVEL_CRITICAL) ? COLOR_RED : NO_COLOR;
  if (remaining_count > 1) {
    SetColor(fg, bg, A_BOLD);
    mvprintw(y, x_pos, "%s", count_str);
    x_pos += count_str_len;
  }

  SetColor(fg, bg, A_BOLD);
  mvprintw(y, x_pos, "%s", level_str);
  x_pos += level_len;

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y, x_pos, " %.*s ", msg_len, error_stack[0].message);
  x_pos += 1 + msg_len + 1;

  SetColor(fg, bg, A_BOLD);
  mvprintw(y, x_pos, "%s", timeout_str);

  if (DEBUG) {
    for (int i = 0; i < 5; i++) {
      if (error_stack[i].message[0] != '\0') {
        int e = (int)(now - error_stack[i].timestamp);
        mvprintw(2 + i, 2, "ERR[%d]: lvl=%d t=%ds msg=%s", i,
                 error_stack[i].level, e, error_stack[i].message);
      }
    }
    mvprintw(7, 2, "ERR_CNT: %d", error_count);
  }
}

/**
 * Check if a critical error has occurred and app is frozen.
 * @return true if app is in frozen state due to critical error, false otherwise
 */
bool IsCriticalError(void) { return app_frozen; }

/**
 * Check if there are any errors currently in the stack.
 * @return true if there are errors to display, false otherwise
 */
bool HasErrors(void) { return error_count > 0; }

/**
 * Test function to add an error to the error line display.
 * Adds error to the display stack at the current timestamp.
 * If level is CRITICAL, also freezes the app.
 * If full_flow is true, also logs to file and (for CRITICAL) renders quit dialog.
 * Only adds up to 10 unique messages per app execution.
 * @param app Pointer to application data (used for CRITICAL freeze when full_flow)
 * @param message The error message to display
 * @param level The error level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * @param full_flow If true, also logs to file and (for CRITICAL) freezes app + shows quit dialog
 */
void TestErrorLine(AppData* app, const char* message, ErrorLevel level,
                   bool full_flow) {
  static int total_added = 0;
  if (total_added >= 10) return;

  for (int i = 0; i < error_count; i++) {
    if (error_stack[i].level == level &&
        strcmp(error_stack[i].message, message) == 0) {
      return;
    }
  }
  if (error_count >= MAX_ERROR_ENTRIES) return;

  time_t now = time(NULL);

  error_stack[error_count].timestamp = now;
  error_stack[error_count].level = level;
  snprintf(error_stack[error_count].message,
           sizeof(error_stack[error_count].message), "%s", message);
  error_count++;
  total_added++;

  if (level == ERROR_LEVEL_CRITICAL) {
    app_frozen = true;
    if (full_flow && app != NULL) {
      app->frozen = true;
      RenderCriticalQuitConfirmation(app);
    }
  }

  if (full_flow) {
    const char* level_str = getErrorLevelMessage(level);
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "[%s] Test: %s", level_str, message);
    FILE* logFile = fopen(ERROR_LOG, "a");
    if (logFile != NULL) {
      fprintf(logFile, "%s\n", log_msg);
      fclose(logFile);
    }
  }
}

/**
 * Get error message for a given error type.
 * @param error The error type
 * @return Human-readable string describing the error
 */
static const char* getErrorMessage(ErrorType error) {
  switch (error) {
    case NO_ERROR:
      return "No error";

      /* Memory and resource allocation errors */
    case MALLOC_ERROR:
      return "Memory allocation error";
    case NULL_POINTER_ERROR:
      return "NULL pointer error";
    case FORK_ERROR:
      return "Fork creation error";
    case PTHREAD_CREATION_ERROR:
      return "Pthread creation error";
    case PTHREAD_DETACH_ERROR:
      return "Pthread detach error";

      /* Initialization errors */
    case INVALID_CONFIG:
      return "Invalid configuration";
    case FILE_ERROR:
      return "Error while managing file";
    case INIT_ERROR:
      return "Initialization failure";
    case WINDOW_CREATION_ERROR:
      return "Window creation error";
    case ANIMATION_DESERIALIZATION_ERROR:
      return "Animation deserialization error";
    case ANIMATION_EQUAL_NULL:
      return "Animation rollfilm is NULL after deserialization";

      /* Input and user interaction errors */
    case INVALID_INPUT:
      return "Invalid input";
    case INVALID_ARGUMENT_ERROR:
      return "Invalid argument";
    case TOO_SMALL_SCREEN:
      return "Screen size is too small";
    case ERROR_EXECUTING_SELECTED_ACTION:
      return "Error during execution of selected action";

      /* Update errors */
    case UPDATE_ERROR:
      return "Error during application update";
    case DRAW_ERROR:
      return "Error while drawing the screen";
    case INPUT_ERROR:
      return "Error during input handling";

      /* Cleanup and deletion errors */
    case WINDOW_DELETION_ERROR:
      return "Window deletion error";
    case END_SCREEN_ERROR:
      return "Error during screen cleanup";
    case END_APP_ERROR:
      return "Error during app cleanup";

      /* Log and socket errors */
    case TIMER_LOG_ERROR:
      return "Timer log creation or handling failure";
    case SOCKET_CREATION_ERROR:
      return "Socket creation failed";
    case SOCKET_CONNECTION_ERROR:
      return "Socket connection failed";
    case SOCKET_BIND_ERROR:
      return "Socket bind failed";
    case SOCKET_LISTEN_ERROR:
      return "Socket listen failed";
    case SOCKET_ACCEPT_ERROR:
      return "Socket accept failed";
    case SOCKET_READ_ERROR:
      return "Socket read failed";
    case SOCKET_WRITE_ERROR:
      return "Socket write failed";
    case SOCKET_CLOSE_ERROR:
      return "Socket close failed";
    case UNLINK_ERROR:
      return "Unlinking socket path failed";

    /* Notification-related errors */
    case NOTIFICATION_SEND_ERROR:
      return "Failed to send notification";
    case AUDIO_PLAYBACK_ERROR:
      return "Failed to play audio";

    case TEST_ERROR:
      return "Test error";

    default:
      return "Unknown error";
  }
}

/**
 * Get error level from error type.
 * @param error The error type
 * @return The corresponding error level
 */
static ErrorLevel getErrorLevel(ErrorType error) {
  switch (error) {
    case MALLOC_ERROR:
    case NULL_POINTER_ERROR:
    case FORK_ERROR:
    case PTHREAD_CREATION_ERROR:
    case PTHREAD_DETACH_ERROR:
    case INVALID_CONFIG:
    case FILE_ERROR:
    case INIT_ERROR:
      return ERROR_LEVEL_CRITICAL;

    case WINDOW_CREATION_ERROR:
    case ANIMATION_DESERIALIZATION_ERROR:
    case ANIMATION_EQUAL_NULL:
    case UPDATE_ERROR:
    case DRAW_ERROR:
    case INPUT_ERROR:
    case TIMER_LOG_ERROR:
    case SOCKET_CREATION_ERROR:
    case SOCKET_CONNECTION_ERROR:
    case SOCKET_BIND_ERROR:
    case SOCKET_LISTEN_ERROR:
    case AUDIO_PLAYBACK_ERROR:
      return ERROR_LEVEL_ERROR;

    case INVALID_INPUT:
    case INVALID_ARGUMENT_ERROR:
    case NOTIFICATION_SEND_ERROR:
    case ERROR_EXECUTING_SELECTED_ACTION:
    case SOCKET_ACCEPT_ERROR:
    case SOCKET_READ_ERROR:
    case SOCKET_WRITE_ERROR:
      return ERROR_LEVEL_WARNING;

    case TOO_SMALL_SCREEN:
    case WINDOW_DELETION_ERROR:
    case END_SCREEN_ERROR:
    case END_APP_ERROR:
    case SOCKET_CLOSE_ERROR:
    case UNLINK_ERROR:
      return ERROR_LEVEL_DEBUG;

    case TEST_ERROR:
      return ERROR_LEVEL_INFO;

    default:
      return ERROR_LEVEL_INFO; /* Default to INFO if error type is unknown */
  }
}

/**
 * Get a descriptive error level message.
 * @param level The error level
 * @return Human-readable string describing the level (e.g., "INFO", "WARNING")
 */
static const char* getErrorLevelMessage(ErrorLevel level) {
  switch (level) {
    case ERROR_LEVEL_DEBUG:
      return "DEBUG";
    case ERROR_LEVEL_INFO:
      return "INFO";
    case ERROR_LEVEL_WARNING:
      return "WARNING";
    case ERROR_LEVEL_ERROR:
      return "ERROR";
    case ERROR_LEVEL_CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

/**
 * Get the ncurses color for a given error level.
 * @param level The error level
 * @return ncurses color index for the error level
 */
static int getErrorColor(ErrorLevel level) {
  switch (level) {
    case ERROR_LEVEL_DEBUG:
      return COLOR_GREEN;
    case ERROR_LEVEL_INFO:
      return COLOR_WHITE;
    case ERROR_LEVEL_WARNING:
      return COLOR_YELLOW;
    case ERROR_LEVEL_ERROR:
      return COLOR_RED;
    case ERROR_LEVEL_CRITICAL:
      return COLOR_BLACK;
    default:
      return COLOR_WHITE;
  }
}

/**
 * Get the display timeout in seconds for a given error level.
 * @param level The error level
 * @return Timeout in seconds (0 means never auto-clear)
 */
static int getErrorTimeout(ErrorLevel level) {
  switch (level) {
    case ERROR_LEVEL_DEBUG:
      return 30;
    case ERROR_LEVEL_INFO:
      return 5;
    case ERROR_LEVEL_WARNING:
      return 15;
    case ERROR_LEVEL_ERROR:
      return 30;
    case ERROR_LEVEL_CRITICAL:
      return 0;
    default:
      return 0;
  }
}
