#ifndef ERROR_H
#define ERROR_H

/* Defining error handling enum */
typedef enum {
  NO_ERROR, /* No error, default state */

  /* Memory and resource allocation errors */
  MALLOC_ERROR, /* Critical: Memory allocation failure */
  FORK_ERROR,   /* Critical: Fork creation failure */

  /* Initialization errors */
  INVALID_CONFIG, /* Critical: Invalid configuration (prevents app start) */
  INIT_ERROR,     /* Critical: General initialization failure */
  WINDOW_CREATION_ERROR,           /* Severe: Error during window creation */
  ANIMATION_DESERIALIZATION_ERROR, /* Severe: Error during animation deserialization */
  ANIMATION_EQUAL_NULL, /* Severe: Animation rollfilm is NULL after deserialization */

  /* Input and user interaction errors */
  INVALID_INPUT,    /* Moderate: Invalid user input */
  TOO_SMALL_SCREEN, /* Moderate: Screen size is too small for the app */
  ERROR_EXECUTING_SELECTED_ACTION, /* Moderate: Error during selected action execution */

  /* Update errors */
  UPDATE_ERROR, /* Severe: Error during application update */
  DRAW_ERROR,   /* Severe: Error while drawing the screen */
  INPUT_ERROR,  /* Severe: Error during input handling */

  /* Cleanup and deletion errors */
  WINDOW_DELETION_ERROR, /* Minor: Error during window deletion */
  END_SCREEN_ERROR,      /* Minor: Error during screen cleanup */
  END_APP_ERROR,         /* Minor: Error during app cleanup */

  /* Log and socket errors */
  TIMER_LOG_ERROR,         /* Severe: Timer log creation or handling failure */
  SOCKET_CREATION_ERROR,   /* Severe: Socket creation failed */
  SOCKET_CONNECTION_ERROR, /* Severe: Failed to connect to socket */
  SOCKET_BIND_ERROR,       /* Severe: Binding the socket failed */
  SOCKET_LISTEN_ERROR,     /* Severe: Listening on the socket failed */
  SOCKET_ACCEPT_ERROR,     /* Moderate: Accepting a client connection failed */
  SOCKET_READ_ERROR,       /* Moderate: Failed to read from socket */
  SOCKET_WRITE_ERROR,      /* Moderate: Failed to write to socket */
  SOCKET_CLOSE_ERROR,      /* Minor: Socket closing error */
  UNLINK_ERROR             /* Minor: Unlinking the socket path failed */
} ErrorType;

/* Get a descriptive error message */
const char* GetErrorMessage(ErrorType error);

/* Log error with descriptive message */
void LogError(const char* context, ErrorType error);

#endif /* ERROR_H */
