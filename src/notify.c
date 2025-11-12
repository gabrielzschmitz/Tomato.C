#include "notify.h"

#include <libnotify/notify.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"
#include "config.h"
#include "error.h"

/* Send a notification to the user */
ErrorType SendNotification(const char* title, const char* description) {
  if (NOTIFICATIONS == 0) return NO_ERROR;
  if (title == NULL || description == NULL) return NULL_POINTER_ERROR;

#ifdef __APPLE__
  char command[512];
  snprintf(command, sizeof(command),
           "osascript -e 'display notification \"%s\" with title \"%s\"'",
           description, title);
  int result = system(command);
  if (result != 0) {
    LogError("SendNotification", NOTIFICATION_SEND_ERROR);
    return NOTIFICATION_SEND_ERROR;
  }
#else
  if (WSL) {
    char command[512];
    snprintf(command, sizeof(command),
             "notify-send -t 5000 -a Tomato.C \"%s\" \"%s\"", title,
             description);
    int result = system(command);
    if (result != 0) {
      LogError("SendNotification", NOTIFICATION_SEND_ERROR);
      return NOTIFICATION_SEND_ERROR;
    }
  } else {
    /* Initialize libnotify */
    if (!notify_init("Tomato.C")) {
      LogError("SendNotification", NOTIFICATION_SEND_ERROR);
      return NOTIFICATION_SEND_ERROR;
    }

    /* Create and show the notification */
    NotifyNotification* notification =
      notify_notification_new(title, description, NULL);
    if (!notify_notification_show(notification, NULL)) {
      LogError("SendNotification", NOTIFICATION_SEND_ERROR);
      notify_uninit();
      return NOTIFICATION_SEND_ERROR;
    }

    /* Cleanup */
    notify_uninit();
  }
#endif

  return NO_ERROR;
}

/* Notify with sound using a Notification struct */
ErrorType Notify(const Notification* notification) {
  if (!notification) {
    LogError("Notification", NULL_POINTER_ERROR);
    return NULL_POINTER_ERROR;
  }
  ErrorType status = NO_ERROR;

  /* Attempt to send the notification */
  status = SendNotification(notification->title, notification->description);
  if (status != NO_ERROR) {
    LogError("Notification", NOTIFICATION_SEND_ERROR);
    return status;
  }

  /* Attempt to play the notification audio */
  status =
    PlayAudio(notification->audio_path, NOTIFICATIONS_SOUND_VOLUME, false);
  if (status != NO_ERROR) LogError("Audio Playback", AUDIO_PLAYBACK_ERROR);

  return status;
}
