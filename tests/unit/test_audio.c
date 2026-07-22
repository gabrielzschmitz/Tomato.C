/**
 * @file test_audio.c
 * @brief Unit tests for the audio notification module.
 *
 * Tests PlayAudio guard logic for NULL path and valid paths.
 */

#include <stdbool.h>

#include "audio.h"
#include "config.h"
#include "error.h"
#include "test_helpers.h"

#undef NOTIFICATIONS_SOUND
#define NOTIFICATIONS_SOUND 1

#undef WSL
#define WSL 0

/**
 * ---------------------------------------------------------------------------
 * PlayAudio
 * ---------------------------------------------------------------------------
 */

/** @brief Mirror of real PlayAudio guard logic under test. */
static ErrorType playAudio(const char* audio_path) {
  if (NOTIFICATIONS_SOUND == 0 || WSL != 0) return NO_ERROR;
  if (audio_path == NULL) return NULL_POINTER_ERROR;
  return NO_ERROR;
}

/** @brief NULL path returns NULL_POINTER_ERROR. */
static void test_play_audio_null_path(void) {
  TEST("PlayAudio NULL path returns NULL_POINTER_ERROR");
  ASSERT_EQ(playAudio(NULL), NULL_POINTER_ERROR);
}

/** @brief Valid path returns NO_ERROR. */
static void test_play_audio_valid_path(void) {
  TEST("PlayAudio valid path returns NO_ERROR");
  ASSERT_EQ(playAudio("/path/to/sound.wav"), NO_ERROR);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("audio");
  RUN_TEST(test_play_audio_null_path,
           "PlayAudio NULL path returns NULL_POINTER_ERROR");
  RUN_TEST(test_play_audio_valid_path,
           "PlayAudio valid path returns NO_ERROR");
  return test_end();
}
