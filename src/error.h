#ifndef ERROR_H
#define ERROR_H

/* Defining error handling enum */
typedef enum {
  NO_ERROR,
  ANIMATION_DESERIALIZATION_ERROR,
  WINDOW_CREATION_ERROR,
  WINDOW_DELETION_ERROR,
  MALLOC_ERROR,
  INVALID_INPUT,
  INVALID_CONFIG,
  TOO_SMALL_SCREEN,
} ErrorType;

/* Get a descriptive error message */
const char* GetErrorMessage(ErrorType error);

/* Log error with descriptive message */
void LogError(const char* context, ErrorType error);

#endif /* ERROR_H */
