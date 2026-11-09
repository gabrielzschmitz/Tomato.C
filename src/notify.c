#include "notify.h"

#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#if !defined(__GNUC__) && !defined(__clang__)
#define __attribute__(x)
#include <libnotify/notify.h>
#undef __attribute__
#else
#include <libnotify/notify.h>
#endif
#endif
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "audio.h"
#include "config.h"
#include "error.h"

/* PRIVATE NOTIFY FUNCTIONS */
static ErrorType sendNotification(const char* title, const char* description);
static bool shouldDispatchAsync(void);
static ErrorType queueNotification(const char* title, const char* description);
static void* notificationThread(void* arg);

typedef struct {
  char* title;
  char* description;
} NotificationRequest;

extern char** environ;

/**
 * Send a notification with sound using a Notification struct.
 * @param notification Pointer to the notification data
 * @return NO_ERROR when sent or queued, or an error code on validation or
 *         dispatch failure
 */
ErrorType Notify(const Notification* notification) {
  if (!notification) {
    LogError("Notification", NULL_POINTER_ERROR);
    return NULL_POINTER_ERROR;
  }
  /* Attempt to send the notification, but never let a missing notification
   * daemon suppress the sound. Both paths are attempted independently. */
  ErrorType send_status = NO_ERROR;
  if (NOTIFICATIONS && (!notification->title || !notification->description)) {
    send_status = NULL_POINTER_ERROR;
  } else if (NOTIFICATIONS && shouldDispatchAsync()) {
    send_status =
      queueNotification(notification->title, notification->description);
  } else {
    send_status =
      sendNotification(notification->title, notification->description);
  }
  if (send_status != NO_ERROR)
    LogError("Notification", NOTIFICATION_SEND_ERROR);

  ErrorType audio_status =
    PlayAudio(notification->audio_path, NOTIFICATIONS_SOUND_VOLUME, false);
  if (audio_status != NO_ERROR)
    LogError("Audio Playback", AUDIO_PLAYBACK_ERROR);

  if (audio_status != NO_ERROR) return audio_status;
  return send_status;
}

/**
 * Return true when platform notification commands must run off the UI thread.
 * @return true on macOS and WSL, false otherwise.
 */
static bool shouldDispatchAsync(void) {
#ifdef __APPLE__
  return true;
#else
  return WSL != 0;
#endif
}

/**
 * Copy and dispatch a platform notification to a detached worker thread.
 * @param title Notification title.
 * @param description Notification details.
 * @return NO_ERROR when queued, or an allocation/thread error.
 */
static ErrorType queueNotification(const char* title, const char* description) {
  NotificationRequest* request = calloc(1, sizeof(*request));
  if (!request) return MALLOC_ERROR;

  size_t title_size = strlen(title) + 1;
  size_t description_size = strlen(description) + 1;
  request->title = malloc(title_size);
  request->description = malloc(description_size);
  if (!request->title || !request->description) {
    free(request->title);
    free(request->description);
    free(request);
    return MALLOC_ERROR;
  }
  memcpy(request->title, title, title_size);
  memcpy(request->description, description, description_size);

  pthread_t thread;
  if (pthread_create(&thread, NULL, notificationThread, request) != 0) {
    free(request->title);
    free(request->description);
    free(request);
    return PTHREAD_CREATION_ERROR;
  }
  if (pthread_detach(thread) != 0) {
    LogError("queueNotification", PTHREAD_DETACH_ERROR);
    return PTHREAD_DETACH_ERROR;
  }
  return NO_ERROR;
}

/**
 * Send one queued notification and release its copied strings.
 * @param arg NotificationRequest pointer.
 * @return Always NULL.
 */
static void* notificationThread(void* arg) {
  NotificationRequest* request = (NotificationRequest*)arg;
  ErrorType status = sendNotification(request->title, request->description);
  if (status != NO_ERROR) LogError("Notification worker", status);
  free(request->title);
  free(request->description);
  free(request);
  return NULL;
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
      if (description[i] == '\\') esc_desc[j++] = '\\';
      if (description[i] == '"') esc_desc[j++] = '\\';
      esc_desc[j++] = description[i];
    }
    esc_desc[j] = '\0';
  }
  {
    size_t j = 0;
    for (size_t i = 0; i < title_len; i++) {
      if (title[i] == '\\') esc_title[j++] = '\\';
      if (title[i] == '"') esc_title[j++] = '\\';
      esc_title[j++] = title[i];
    }
    esc_title[j] = '\0';
  }

  char script[1024];
  snprintf(script, sizeof(script),
           "display notification \"%s\" with title \"%s\"", esc_desc,
           esc_title);
  char* const args[] = {"osascript", "-e", script, NULL};
  pid_t pid;
  int spawn_status = posix_spawnp(&pid, "osascript", NULL, NULL, args, environ);
  free(esc_desc);
  free(esc_title);
  if (spawn_status != 0) {
    LogError("sendNotification", FORK_ERROR);
    return FORK_ERROR;
  }

  int status;
  pid_t waited;
  do {
    waited = waitpid(pid, &status, 0);
  } while (waited == -1 && errno == EINTR);
  if (waited == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LogError("sendNotification", NOTIFICATION_SEND_ERROR);
    return NOTIFICATION_SEND_ERROR;
  }
#else
  if (WSL) {
    char* const args[] = {
      "notify-send",      "-t", "5000", "-a", "Tomato.C", (char*)title,
      (char*)description, NULL};
    pid_t pid;
    int spawn_status =
      posix_spawnp(&pid, "notify-send", NULL, NULL, args, environ);
    if (spawn_status != 0) {
      LogError("sendNotification", FORK_ERROR);
      return FORK_ERROR;
    }
    int status;
    pid_t waited;
    do {
      waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
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
    if (!notification) {
      LogError("sendNotification", MALLOC_ERROR);
      notify_uninit();
      return MALLOC_ERROR;
    }
    bool shown = notify_notification_show(notification, NULL);
    g_object_unref(notification);
    if (!shown) {
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
