/**
 * @file test_audio.c
 * @brief Unit tests for the audio notification module.
 *
 * Tests PlayAudio guards against the production audio implementation.
 */

#include <stdbool.h>
#include <string.h>

#include "core/config.h"
#include "core/error.h"
#include "features/audio/audio.h"
#include "test_helpers.h"

Config g_config;

void LogError(const char* context, ErrorType type) {
  (void)context;
  (void)type;
}

/**
 * ---------------------------------------------------------------------------
 * PlayAudio
 * ---------------------------------------------------------------------------
 */

/** @brief Disabled sound returns without initializing the audio engine. */
static void test_play_audio_disabled(void) {
  TEST("PlayAudio disabled sound returns NO_ERROR");
  memset(&g_config, 0, sizeof(g_config));
  ASSERT_EQ(PlayAudio("/path/to/sound.wav", 0.5f, false), NO_ERROR);
}

/** @brief NULL path returns NULL_POINTER_ERROR before engine initialization. */
static void test_play_audio_null_path(void) {
  TEST("PlayAudio NULL path returns NULL_POINTER_ERROR");
  memset(&g_config, 0, sizeof(g_config));
  g_config.notifications.sound = 1;
  ASSERT_EQ(PlayAudio(NULL, 0.5f, false), NULL_POINTER_ERROR);
}

/** @brief WSL skips playback without opening an audio device. */
static void test_play_audio_wsl_disabled(void) {
  TEST("PlayAudio skips sound in WSL");
  memset(&g_config, 0, sizeof(g_config));
  g_config.notifications.sound = 1;
  g_config.misc.wsl = 1;
  ASSERT_EQ(PlayAudio("/path/to/sound.wav", 0.5f, false), NO_ERROR);
}

/**
 * ---------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------------
 */

int main(void) {
  test_begin("audio");
  RUN_TEST(test_play_audio_disabled, "PlayAudio disabled sound");
  RUN_TEST(test_play_audio_null_path, "PlayAudio NULL path");
  RUN_TEST(test_play_audio_wsl_disabled, "PlayAudio skips WSL");
  return test_end();
}
