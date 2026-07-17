#include "notify.h"

#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <libnotify/notify.h>
#endif
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "audio.h"
#include "config.h"
#include "error.h"

/* PRIVATE NOTIFY FUNCTIONS */
static ErrorType sendNotification(const char* title, const char* description);

/**
 * Send a notification with sound using a Notification struct.
 * @param notification Pointer to the notification data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType Notify(const Notification* notification) {
  if (!notification) {
    LogError("Notification", NULL_POINTER_ERROR);
    return NULL_POINTER_ERROR;
  }
  ErrorType status = NO_ERROR;

  /* Attempt to send the notification */
  status = sendNotification(notification->title, notification->description);
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

/**
 * Send a notification to the user using the system notification daemon.
 * @param title Notification title string
 * @param description Detailed description text
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static ErrorType sendNotification(const char* title, const char* description) {
  if (NOTIFICATIONS == 0) return NO_ERROR;
  if (title == NULL || description == NULL) return NULL_POINTER_ERROR;

#ifdef __APPLE__
  /* Escape double-quotes for AppleScript string literals */
  char* esc_desc = NULL;
  char* esc_title = NULL;
  size_t desc_len = strlen(description);
  size_t title_len = strlen(title);
  esc_desc = malloc(desc_len * 2 + 1);
  esc_title = malloc(title_len * 2 + 1);
  if (!esc_desc || !esc_title) {
    free(esc_desc);
    free(esc_title);
    LogError("sendNotification", MALLOC_ERROR);
    return MALLOC_ERROR;
  }
  {
    size_t j = 0;
    for (size_t i = 0; i < desc_len; i++) {
      if (description[i] == '"') esc_desc[j++] = '\\';
      esc_desc[j++] = description[i];
    }
    esc_desc[j] = '\0';
  }
  {
    size_t j = 0;
    for (size_t i = 0; i < title_len; i++) {
      if (title[i] == '"') esc_title[j++] = '\\';
      esc_title[j++] = title[i];
    }
    esc_title[j] = '\0';
  }

  pid_t pid = fork();
  if (pid == -1) {
    free(esc_desc);
    free(esc_title);
    LogError("sendNotification", FORK_ERROR);
    return FORK_ERROR;
  }
  if (pid == 0) {
    char script[1024];
    snprintf(script, sizeof(script),
             "display notification \"%s\" with title \"%s\"", esc_desc,
             esc_title);
    execlp("osascript", "osascript", "-e", script, NULL);
    _exit(1);
  }
  free(esc_desc);
  free(esc_title);

  int status;
  if (waitpid(pid, &status, 0) == -1 || !WIFEXITED(status) ||
      WEXITSTATUS(status) != 0) {
    LogError("sendNotification", NOTIFICATION_SEND_ERROR);
    return NOTIFICATION_SEND_ERROR;
  }
#else
  if (WSL) {
    pid_t pid = fork();
    if (pid == -1) {
      LogError("sendNotification", FORK_ERROR);
      return FORK_ERROR;
    }
    if (pid == 0) {
      execlp("notify-send", "notify-send", "-t", "5000", "-a", "Tomato.C",
             title, description, NULL);
      _exit(1);
    }
    int status;
    if (waitpid(pid, &status, 0) == -1 || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {
      LogError("sendNotification", NOTIFICATION_SEND_ERROR);
      return NOTIFICATION_SEND_ERROR;
    }
  } else {
    /* Initialize libnotify */
    if (!notify_init("Tomato.C")) {
      LogError("sendNotification", NOTIFICATION_SEND_ERROR);
      return NOTIFICATION_SEND_ERROR;
    }

    /* Create and show the notification */
    NotifyNotification* notification =
      notify_notification_new(title, description, NULL);
    if (!notify_notification_show(notification, NULL)) {
      LogError("sendNotification", NOTIFICATION_SEND_ERROR);
      notify_uninit();
      return NOTIFICATION_SEND_ERROR;
    }

    /* Cleanup */
    notify_uninit();
  }
#endif

  return NO_ERROR;
}
