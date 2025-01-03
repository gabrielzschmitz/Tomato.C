#ifndef ERROR_H
#define ERROR_H

/* Defining error handling enum */
typedef enum {
  NO_ERROR, /* No error, default state */

  /* Memory and resource allocation errors */
  MALLOC_ERROR, /* Critical: Memory allocation failure */

  /* Initialization errors */
  INVALID_CONFIG, /* Critical: Invalid configuration (prevents app start) */
  WINDOW_CREATION_ERROR,           /* Severe: Error during window creation */
  ANIMATION_DESERIALIZATION_ERROR, /* Severe: Error during animation deserialization */

  /* Input and user interaction errors */
  INVALID_INPUT,    /* Moderate: Invalid user input */
  TOO_SMALL_SCREEN, /* Moderate: Screen size is too small for the app */
  ERROR_EXECUTING_SELECTED_ACTION, /* Moderate: Error during selected action execution */

  /* Cleanup and deletion errors */
  WINDOW_DELETION_ERROR /* Minor: Error during window deletion */
} ErrorType;

/* Get a descriptive error message */
const char* GetErrorMessage(ErrorType error);

/* Log error with descriptive message */
void LogError(const char* context, ErrorType error);

#endif /* ERROR_H */
