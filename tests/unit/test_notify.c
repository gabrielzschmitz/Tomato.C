/**
 * @file test_notify.c
 * @brief Unit tests for the notification module.
 *
 * Tests Notify NULL pointer guard, disabled-notifications short-circuit,
 * and NULL title/description error paths.
 */

#include <string.h>

#include "audio.h"
#include "config.h"
#include "error.h"
#include "notify.h"
#include "test_helpers.h"

Config g_config;

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
  return test_end();
}
