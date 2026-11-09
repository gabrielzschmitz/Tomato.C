/**
 * @file test_notify.c
 * @brief Unit tests for the notification module.
 *
 * Tests Notify NULL pointer guard, disabled-notifications short-circuit,
 * and NULL title/description error paths.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "audio.h"
#include "config.h"
#include "error.h"
#include "notify.h"
#include "test_helpers.h"

Config g_config;

static int play_audio_calls = 0;

static double elapsedMilliseconds(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
         (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

/* stubs */
void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

ErrorType PlayAudio(const char* audio_path, const float volume,
                    const bool loop) {
  (void)audio_path;
  (void)volume;
  (void)loop;
  play_audio_calls++;
  return NO_ERROR;
}

/**
 * ---------------------------------------------------------------------------
 * Notify — NULL and disabled paths
 * ---------------------------------------------------------------------------
 */

/** @brief Notify(NULL) returns NULL_POINTER_ERROR. */
static void test_notify_null(void) {
  TEST("Notify with NULL returns NULL_POINTER_ERROR");
  ErrorType result = Notify(NULL);
  ASSERT_EQ(result, NULL_POINTER_ERROR);
}

/** @brief Notify returns NO_ERROR when notifications are disabled. */
static void test_notify_disabled(void) {
  TEST("Notify with notifications disabled returns NO_ERROR");
  Notification n;
  memset(&n, 0, sizeof(n));
  n.title = "test";
  n.description = "test desc";
  g_config.notifications.enabled = 0;
  ErrorType result = Notify(&n);
  ASSERT_EQ(result, NO_ERROR);
}

/** @brief Notify with NULL title returns NULL_POINTER_ERROR. */
static void test_notify_null_title(void) {
  TEST("Notify with NULL title returns NULL_POINTER_ERROR");
  Notification n;
  memset(&n, 0, sizeof(n));
  n.title = NULL;
  n.description = "desc";
  g_config.notifications.enabled = 1;
  ErrorType result = Notify(&n);
  ASSERT_EQ(result, NULL_POINTER_ERROR);
}

/** @brief Notify with NULL description returns NULL_POINTER_ERROR. */
static void test_notify_null_description(void) {
  TEST("Notify with NULL description returns NULL_POINTER_ERROR");
  Notification n;
  memset(&n, 0, sizeof(n));
  n.title = "title";
  n.description = NULL;
  g_config.notifications.enabled = 1;
  ErrorType result = Notify(&n);
  ASSERT_EQ(result, NULL_POINTER_ERROR);
}

/** @brief Sound still plays when the desktop notification fails. */
static void test_notify_plays_sound_when_send_fails(void) {
  TEST("Notify plays sound even when sendNotification fails");
  Notification n;
  memset(&n, 0, sizeof(n));
  n.title = NULL; /* forces a send failure */
  n.description = "desc";
  g_config.notifications.enabled = 1;
  play_audio_calls = 0;

  ErrorType result = Notify(&n);

  ASSERT_EQ(result, NULL_POINTER_ERROR);
  ASSERT_EQ(play_audio_calls, 1);
}

/** @brief WSL notification helper wait does not block Notify. */
static void test_notify_platform_dispatch_is_async(void) {
  TEST("Notify returns before slow platform helper exits");
  char dir[] = "/tmp/t_notify_async_XXXXXX";
  ASSERT_NOT_NULL(mkdtemp(dir));

  const char* helper_name = "notify-send";
#ifdef __APPLE__
  helper_name = "osascript";
#endif
  char helper_path[256];
  char marker_path[256];
  snprintf(helper_path, sizeof(helper_path), "%s/%s", dir, helper_name);
  snprintf(marker_path, sizeof(marker_path), "%s/started", dir);
  FILE* helper = fopen(helper_path, "w");
  ASSERT_NOT_NULL(helper);
  fprintf(helper, "#!/bin/sh\n: > '%s'\n/bin/sleep 0.3\n", marker_path);
  fclose(helper);
  ASSERT_EQ(chmod(helper_path, 0700), 0);

  const char* old_path = getenv("PATH");
  char* saved_path = NULL;
  if (old_path) {
    saved_path = malloc(strlen(old_path) + 1);
    ASSERT_NOT_NULL(saved_path);
    strcpy(saved_path, old_path);
  }
  char path[4096];
  snprintf(path, sizeof(path), "%s:%s", dir, old_path ? old_path : "");
  ASSERT_EQ(setenv("PATH", path, 1), 0);

  g_config.notifications.enabled = 1;
#ifndef __APPLE__
  g_config.misc.wsl = 1;
#endif
  Notification notification = {"title", "description", NULL};
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  ASSERT_EQ(Notify(&notification), NO_ERROR);
  clock_gettime(CLOCK_MONOTONIC, &end);
  ASSERT_TRUE(elapsedMilliseconds(start, end) < 200.0);

  for (int i = 0; i < 100 && access(marker_path, F_OK) != 0; i++) usleep(10000);
  ASSERT_EQ(access(marker_path, F_OK), 0);
  usleep(350000);

  if (saved_path) {
    setenv("PATH", saved_path, 1);
    free(saved_path);
  } else {
    unsetenv("PATH");
  }
  unlink(marker_path);
  unlink(helper_path);
  rmdir(dir);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("notify");
  RUN_TEST(test_notify_null, "Notify(NULL) returns NULL_POINTER_ERROR");
  RUN_TEST(test_notify_disabled,
           "Notify with notifications disabled returns NO_ERROR");
  RUN_TEST(test_notify_null_title, "Notify with NULL title returns error");
  RUN_TEST(test_notify_null_description,
           "Notify with NULL description returns error");
  RUN_TEST(test_notify_plays_sound_when_send_fails,
           "Notify plays sound even when sendNotification fails");
  RUN_TEST(test_notify_platform_dispatch_is_async,
           "Notify returns before platform helper exits");
  return test_end();
}
