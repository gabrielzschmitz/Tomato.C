#include "error.h"
#include <stdio.h>

/* Descriptive error messages */
const char* GetErrorMessage(ErrorType error) {
  switch (error) {
    case NO_ERROR:
      return "No error";

      /* Memory and resource allocation errors */
    case MALLOC_ERROR: return "Memory allocation error";
    case FORK_ERROR:
      return "Fork creation error";

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
    default: return "Unknown error";
  }
}

/* Log error with a descriptive message */
void LogError(const char* context, ErrorType error) {
  fprintf(stderr, "[ERROR] %s: %s (Code: %d)\n", context,
          GetErrorMessage(error), error);
}
