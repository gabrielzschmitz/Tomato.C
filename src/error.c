#include "error.h"
#include <stdio.h>

/* Descriptive error messages */
const char* GetErrorMessage(ErrorType error) {
  switch (error) {
    case NO_ERROR: return "No error";
    case ANIMATION_DESERIALIZATION_ERROR:
      return "Animation deserialization error";
    case WINDOW_CREATION_ERROR: return "Window creation error";
    case WINDOW_DELETION_ERROR: return "Window deletion error";
    case MALLOC_ERROR: return "Memory allocation error";
    case INVALID_INPUT: return "Invalid input";
    case INVALID_CONFIG: return "Invalid configuration";
    case ERROR_EXECUTING_SELECTED_ACTION: return "Error during execution of selected action";
    case TOO_SMALL_SCREEN: return "Screen size is too small";
    default: return "Unknown error";
  }
}

/* Log error with a descriptive message */
void LogError(const char* context, ErrorType error) {
  fprintf(stderr, "[ERROR] %s: %s (Code: %d)\n", context,
          GetErrorMessage(error), error);
}
