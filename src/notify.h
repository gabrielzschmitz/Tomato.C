#ifndef NOTIFY_H_
#define NOTIFY_H_

#include "error.h"

/* Define a struct to hold notification data */
typedef struct {
  const char* title;
  const char* description;
  const char* audio_path;
} Notification;

/* Send a notification to the user */
ErrorType SendNotification(const char* title, const char* description);

/* Notify with sound using a Notification struct */
ErrorType Notify(const Notification* notification);

#endif /* NOTIFY_H */
