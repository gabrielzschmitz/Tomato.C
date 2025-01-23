#include "audio.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#ifdef __APPLE__
#define MA_NO_RUNTIME_LINKING
#endif
#define MINIAUDIO_IMPLEMENTATION
#include "external/miniaudio.h"

/* Structure to pass data to the playback thread */
static struct playbackData {
  char audio_path[512];
  float volume;
  bool loop;
} playbackData;

/* Playback thread function */
static void* playbackThread(void* arg) {
  struct playbackData* data = (struct playbackData*)arg;
  ma_result result;
  ma_engine engine;
  ma_sound sound;

  /* Initialize the engine */
  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    free(data);
    return NULL;
  }

  /* Set volume for the engine */
  ma_engine_set_volume(&engine, data->volume);

  /* Initialize the sound */
  result =
    ma_sound_init_from_file(&engine, data->audio_path, 0, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    ma_engine_uninit(&engine);
    free(data);
    return NULL;
  }

  /* Enable looping */
  ma_sound_set_looping(&sound, data->loop);

  /* Play the sound */
  result = ma_sound_start(&sound);
  if (result != MA_SUCCESS) {
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
    free(data);
    return NULL;
  }

  /* Wait for playback to finish */
  while (ma_sound_is_playing(&sound))
    ma_sleep(1);

  /* Cleanup */
  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
  free(data);

  pthread_exit(NULL);
  return NULL;
}

/* Play audio from path using miniaudio */
ErrorType PlayAudio(const char* audio_path, const float volume,
                    const bool loop) {
  if (NOTIFICATIONS_SOUND == 0 || WSL != 0) return NO_ERROR;
  if (audio_path == NULL) return NULL_POINTER_ERROR;

  /* Prepare playback data */
  struct playbackData* data =
    (struct playbackData*)malloc(sizeof(struct playbackData));
  if (data == NULL) return MALLOC_ERROR;

  strncpy(data->audio_path, audio_path, sizeof(data->audio_path) - 1);
  data->audio_path[sizeof(data->audio_path) - 1] = '\0';
  data->volume = volume;
  data->loop = loop;

  /* Create playback thread */
  pthread_t thread;
  if (pthread_create(&thread, NULL, playbackThread, data) != 0) {
    free(data);
    return PTHREAD_CREATION_ERROR;
  }

  /* Detach the thread to allow self-cleanup */
  if (pthread_detach(thread) != 0) {
    free(data);
    return PTHREAD_DETACH_ERROR;
  }

  return NO_ERROR;
}
