#ifndef NOTIFY_H_
#define NOTIFY_H_

#include "error.h"

/**
 * Notification data structure containing notification content.
 * Used to send system notifications to the user.
 */
typedef struct {
  const char* title;       /**< Notification title */
  const char* description; /**< Detailed description text */
  const char* audio_path;  /**< Path to audio file to play (or NULL) */
} Notification;

/**
 * Send a notification with sound using a Notification struct.
 * @param notification Pointer to the notification data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType Notify(const Notification* notification);

#endif /* NOTIFY_H */
