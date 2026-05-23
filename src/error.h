#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <time.h>

/**
 * Forward declaration of AppData.
 * Required for SetError function that interacts with app state.
 */
typedef struct AppData AppData;

/**
 * Error level enum defining severity hierarchy.
 * DEBUG < INFO < WARNING < ERROR < CRITICAL
 */
typedef enum {
  ERROR_LEVEL_DEBUG,   /* Debug: green, internal debugging messages */
  ERROR_LEVEL_INFO,    /* Informational: white, non-critical info */
  ERROR_LEVEL_WARNING, /* Warning: yellow, needs attention but not critical */
  ERROR_LEVEL_ERROR,   /* Error: red, critical issue requiring attention */
  ERROR_LEVEL_CRITICAL /* Critical: white on red, failure requiring immediate
                         attention and app freeze */
} ErrorLevel;

/**
 * Structure representing a single error entry in the error stack.
 * Contains timestamp, severity level, and error message.
 */
typedef struct {
  time_t timestamp;    /* Timestamp when error was recorded */
  ErrorLevel level;    /* Severity level of the error */
  char message[256];   /* Error message content */
} ErrorEntry;

/**
 * Enum for defining error types throughout the application.
 * Categorized by subsystem and severity.
 */
typedef enum {
  NO_ERROR, /* No error, default state */

  /* Memory and resource allocation errors */
  MALLOC_ERROR,           /* Critical: Memory allocation failure */
  NULL_POINTER_ERROR,     /* Critical: Null pointer at function */
  FORK_ERROR,             /* Critical: Fork creation failure */
  PTHREAD_CREATION_ERROR, /* Critical: Pthread creation failure */
  PTHREAD_DETACH_ERROR,   /* Critical: Pthread detach failure */

  /* Initialization errors */
  INVALID_CONFIG, /* Critical: Invalid configuration (prevents app start) */
  INIT_ERROR,     /* Critical: General initialization failure */
  WINDOW_CREATION_ERROR,           /* Error: Error during window creation */
  ANIMATION_DESERIALIZATION_ERROR, /* Error: Error during animation deserialization */
  ANIMATION_EQUAL_NULL, /* Error: Animation rollfilm is NULL after deserialization */

  /* Input and user interaction errors */
  INVALID_INPUT,          /* Warning: Invalid user input */
  INVALID_ARGUMENT_ERROR, /* Warning: Invalid argument error */
  TOO_SMALL_SCREEN,       /* Info: Screen size is too small for the app */
  ERROR_EXECUTING_SELECTED_ACTION, /* Warning: Error during selected action execution */

  /* Update errors */
  UPDATE_ERROR, /* Error: Error during application update */
  DRAW_ERROR,   /* Error: Error while drawing the screen */
  INPUT_ERROR,  /* Error: Error during input handling */

  /* Cleanup and deletion errors */
  WINDOW_DELETION_ERROR, /* Info: Error during window deletion */
  END_SCREEN_ERROR,      /* Info: Error during screen cleanup */
  END_APP_ERROR,         /* Info: Error during app cleanup */

  /* Log and socket errors */
  TIMER_LOG_ERROR,         /* Error: Timer log creation or handling failure */
  SOCKET_CREATION_ERROR,   /* Error: Socket creation failed */
  SOCKET_CONNECTION_ERROR, /* Error: Failed to connect to socket */
  SOCKET_BIND_ERROR,       /* Error: Binding the socket failed */
  SOCKET_LISTEN_ERROR,     /* Error: Listening on the socket failed */
  SOCKET_ACCEPT_ERROR,     /* Warning: Accepting a client connection failed */
  SOCKET_READ_ERROR,       /* Warning: Failed to read from socket */
  SOCKET_WRITE_ERROR,      /* Warning: Failed to write to socket */
  SOCKET_CLOSE_ERROR,      /* Info: Socket closing error */
  UNLINK_ERROR,            /* Info: Unlinking the socket path failed */

  /* Notification-related errors */
  NOTIFICATION_SEND_ERROR, /* Error: Failed to send notification */
  AUDIO_PLAYBACK_ERROR     /* Error: Failed to play audio */
} ErrorType;

/**
 * Log an error with context information.
 * Writes to the error log file with timestamp and context.
 * @param context String describing where the error occurred
 * @param error The error type
 */
void LogError(const char* context, ErrorType error);

/**
 * Set and display an error at the bottom of the screen.
 * Adds error to the display stack and logs to file.
 * If error level is CRITICAL, freezes the app and shows quit confirmation.
 * @param app Pointer to application data (used for CRITICAL freeze)
 * @param context String describing where the error occurred
 * @param type The error type
 */
void SetError(AppData* app, const char* context, ErrorType type);

/**
 * Clear all errors from the error stack.
 * Resets the error display and allows normal app operation.
 */
void ClearErrors(void);

/**
 * Render all errors in the stack at the bottom line of the screen.
 * Displays up to ERROR_STACK_SIZE errors with appropriate colors.
 * Auto-clears based on timeout: INFO=5s, WARNING=15s, ERROR/DEBUG=30s,
 * CRITICAL=never.
 */
void RenderErrorLine(void);

/**
 * Check if a critical error has occurred and app is frozen.
 * @return true if app is in frozen state due to critical error, false otherwise
 */
bool IsCriticalError(void);

/**
 * Check if there are any errors currently in the stack.
 * @return true if there are errors to display, false otherwise
 */
bool HasErrors(void);

/**
 * Test function to add an error to the error line display.
 * Useful for testing error rendering - call from tomato.c and comment/uncomment.
 * @param message The error message to display
 * @param level The error level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 */
void TestErrorLine(const char* message, ErrorLevel level);

#endif /* ERROR_H */
