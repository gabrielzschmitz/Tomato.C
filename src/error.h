#ifndef ERROR_H
#define ERROR_H

/* Defining error levels */
typedef enum {
  ERROR_LEVEL_INFO,    /* Informational: non-critical */
  ERROR_LEVEL_WARNING, /* Warning: needs attention but not critical */
  ERROR_LEVEL_ERROR,   /* Error: critical issue */
  ERROR_LEVEL_CRITICAL /* Critical: failure requiring immediate attention */
} ErrorLevel;

/* Defining error handling enum */
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

/* Get a descriptive error message with its level */
const char* GetErrorLevelMessage(ErrorLevel level);

/* Get error level from error type */
ErrorLevel GetErrorLevel(ErrorType error);

/* Log error with descriptive message and level */
const char* GetErrorMessage(ErrorType error);

/* Log error with descriptive message */
void LogError(const char* context, ErrorType error);

#endif /* ERROR_H */
