#include "error.h"
#include "config.h"

#include <ncurses.h>
#include <stdio.h>

/* Get a descriptive error message with its level */
const char* GetErrorLevelMessage(ErrorLevel level) {
  switch (level) {
    case ERROR_LEVEL_INFO: return "INFO";
    case ERROR_LEVEL_WARNING: return "WARNING";
    case ERROR_LEVEL_ERROR: return "ERROR";
    case ERROR_LEVEL_CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

/* Get error level from error type */
ErrorLevel GetErrorLevel(ErrorType error) {
  switch (error) {
    case MALLOC_ERROR:
    case NULL_POINTER_ERROR:
    case FORK_ERROR:
    case PTHREAD_CREATION_ERROR:
    case PTHREAD_DETACH_ERROR:
    case INVALID_CONFIG:
    case INIT_ERROR: return ERROR_LEVEL_CRITICAL;

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
    case AUDIO_PLAYBACK_ERROR: return ERROR_LEVEL_ERROR;

    case INVALID_INPUT:
    case INVALID_ARGUMENT_ERROR:
    case NOTIFICATION_SEND_ERROR:
    case ERROR_EXECUTING_SELECTED_ACTION:
    case SOCKET_ACCEPT_ERROR:
    case SOCKET_READ_ERROR:
    case SOCKET_WRITE_ERROR: return ERROR_LEVEL_WARNING;

    case TOO_SMALL_SCREEN:
    case WINDOW_DELETION_ERROR:
    case END_SCREEN_ERROR:
    case END_APP_ERROR:
    case SOCKET_CLOSE_ERROR:
    case UNLINK_ERROR: return ERROR_LEVEL_INFO;

    default:
      return ERROR_LEVEL_INFO; /* Default to INFO if error type is unknown */
  }
}

/* Descriptive error messages */
const char* GetErrorMessage(ErrorType error) {
  switch (error) {
    case NO_ERROR:
      return "No error";

      /* Memory and resource allocation errors */
    case MALLOC_ERROR: return "Memory allocation error";
    case NULL_POINTER_ERROR: return "NULL pointer error";
    case FORK_ERROR: return "Fork creation error";
    case PTHREAD_CREATION_ERROR: return "Pthread creation error";
    case PTHREAD_DETACH_ERROR:
      return "Pthread detach error";

      /* Initialization errors */
    case INVALID_CONFIG: return "Invalid configuration";
    case INIT_ERROR: return "Initialization failure";
    case WINDOW_CREATION_ERROR: return "Window creation error";
    case ANIMATION_DESERIALIZATION_ERROR:
      return "Animation deserialization error";
    case ANIMATION_EQUAL_NULL:
      return "Animation rollfilm is NULL after deserialization";

      /* Input and user interaction errors */
    case INVALID_INPUT: return "Invalid input";
    case INVALID_ARGUMENT_ERROR: return "Invalid argument";
    case TOO_SMALL_SCREEN: return "Screen size is too small";
    case ERROR_EXECUTING_SELECTED_ACTION:
      return "Error during execution of selected action";

      /* Update errors */
    case UPDATE_ERROR: return "Error during application update";
    case DRAW_ERROR: return "Error while drawing the screen";
    case INPUT_ERROR:
      return "Error during input handling";

      /* Cleanup and deletion errors */
    case WINDOW_DELETION_ERROR: return "Window deletion error";
    case END_SCREEN_ERROR: return "Error during screen cleanup";
    case END_APP_ERROR:
      return "Error during app cleanup";

      /* Log and socket errors */
    case TIMER_LOG_ERROR: return "Timer log creation or handling failure";
    case SOCKET_CREATION_ERROR: return "Socket creation failed";
    case SOCKET_CONNECTION_ERROR: return "Socket connection failed";
    case SOCKET_BIND_ERROR: return "Socket bind failed";
    case SOCKET_LISTEN_ERROR: return "Socket listen failed";
    case SOCKET_ACCEPT_ERROR: return "Socket accept failed";
    case SOCKET_READ_ERROR: return "Socket read failed";
    case SOCKET_WRITE_ERROR: return "Socket write failed";
    case SOCKET_CLOSE_ERROR: return "Socket close failed";
    case UNLINK_ERROR: return "Unlinking socket path failed";

    /* Notification-related errors */
    case NOTIFICATION_SEND_ERROR: return "Failed to send notification";
    case AUDIO_PLAYBACK_ERROR: return "Failed to play audio";

    default: return "Unknown error";
  }
}

/* Log error with descriptive message and level */
void LogError(const char* context, ErrorType error) {
  const char* error_message = GetErrorMessage(error);
  const char* error_level = GetErrorLevelMessage(GetErrorLevel(error));

  char message[256];
  snprintf(message, sizeof(message), "[%s] %s: %s (Code: %d)", error_level,
           context, error_message, error);

  FILE* logFile = fopen(ERROR_LOG, "a");
  if (logFile != NULL) {
    fprintf(logFile, "%s\n", message);
    fclose(logFile);
  } else {
    fprintf(stderr, "[CRITICAL] Failed to open log file\n");
  }
}
