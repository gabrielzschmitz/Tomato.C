#include "audio.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"

#ifdef __APPLE__
#define MA_NO_RUNTIME_LINKING
#endif
#define MINIAUDIO_IMPLEMENTATION
/**
 * The 'keys' macro in config.h conflicts with parameter names in Apple's
 * CoreFoundation headers (included transitively by miniaudio on macOS).
 * Undefine it here and restore it after the include.
 */
#undef keys
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#include "external/miniaudio.h"
#pragma GCC diagnostic pop
#define keys (g_config.key_bindings)

/**
 * Private helper functions and state for audio playback.
 */
typedef struct LoopingPlayback {
  ma_sound sound;
  struct LoopingPlayback* next;
} LoopingPlayback;

static ErrorType playLoopingAudio(const char* audio_path, float volume);
/* Noise Audio */
static ma_engine noise_engine;
static ma_sound_group notification_group;
static ma_sound* noise_sounds = NULL;
static LoopingPlayback* looping_playbacks = NULL;
static int noise_sound_count = 0;
static bool noise_engine_initialized = false;

/**
 * ---------------------------------------------------------------------------
 * Audio Playback
 * ---------------------------------------------------------------------------
 */

/**
 * Play audio from path using miniaudio.
 * Uses persistent audio engine; one-shot sounds self-clean after playback.
 * Sound is disabled on WSL and when NOTIFICATIONS_SOUND is 0.
 * @param audio_path Path to the audio file to play
 * @param volume Volume level for playback (0.0 to 1.0)
 * @param loop true to loop playback, false to play once
 * @return NO_ERROR on success, error code on failure
 */
ErrorType PlayAudio(const char* audio_path, const float volume,
                    const bool loop) {
  if (NOTIFICATIONS_SOUND == 0 || WSL != 0) return NO_ERROR;
  if (audio_path == NULL) {
    LogError("PlayAudio", NULL_POINTER_ERROR);
    return NULL_POINTER_ERROR;
  }

  if (!noise_engine_initialized) {
    ErrorType error = InitNoiseAudio();
    if (error != NO_ERROR) return error;
  }

  if (loop) return playLoopingAudio(audio_path, volume);

  ma_sound_group_set_volume(&notification_group, volume);
  if (ma_engine_play_sound(&noise_engine, audio_path, &notification_group) !=
      MA_SUCCESS) {
    LogError("PlayAudio", AUDIO_PLAYBACK_ERROR);
    return AUDIO_PLAYBACK_ERROR;
  }
  return NO_ERROR;
}

/**
 * Start a looping effect on the shared engine.
 * @param audio_path Path to audio file.
 * @param volume Playback volume from 0.0 to 1.0.
 * @return NO_ERROR on success or an audio error on failure.
 */
static ErrorType playLoopingAudio(const char* audio_path, float volume) {
  LoopingPlayback* playback = calloc(1, sizeof(*playback));
  if (!playback) return MALLOC_ERROR;

  ma_result result = ma_sound_init_from_file(&noise_engine, audio_path, 0, NULL,
                                             NULL, &playback->sound);
  if (result != MA_SUCCESS) {
    free(playback);
    LogError("playLoopingAudio", AUDIO_PLAYBACK_ERROR);
    return AUDIO_PLAYBACK_ERROR;
  }

  ma_sound_set_looping(&playback->sound, true);
  ma_sound_set_volume(&playback->sound, volume);
  result = ma_sound_start(&playback->sound);
  if (result != MA_SUCCESS) {
    ma_sound_uninit(&playback->sound);
    free(playback);
    LogError("playLoopingAudio", AUDIO_PLAYBACK_ERROR);
    return AUDIO_PLAYBACK_ERROR;
  }

  playback->next = looping_playbacks;
  looping_playbacks = playback;
  return NO_ERROR;
}

/* ---------------------------------------------------------------------------
 * Noise Audio
 * --------------------------------------------------------------------------- */

/**
 * Register white noise tracks and initialize the internal audio state.
 * Frees any previous track data and reallocates arrays to match the
 * given track count.  All tracks start stopped with their default
 * volume.
 * @param data   Pointer to WhiteNoiseData to populate
 * @param tracks Array of WhiteNoiseTrackDef to register
 * @param count  Number of tracks in the array
 * @return NO_ERROR on success, or MALLOC_ERROR on failure
 */
ErrorType RegisterWhiteNoiseTracks(WhiteNoiseData* data,
                                   const WhiteNoiseTrackDef* tracks,
                                   int count) {
  if (data->tracks) {
    free(data->tracks);
    data->tracks = NULL;
  }
  free(data->playing);
  free(data->volume);

  data->tracks =
    (WhiteNoiseTrackDef*)malloc(count * sizeof(WhiteNoiseTrackDef));
  data->playing = (bool*)calloc(count, sizeof(bool));
  data->volume = (int*)malloc(count * sizeof(int));
  if (!data->tracks || !data->playing || !data->volume) {
    free(data->tracks);
    free(data->playing);
    free(data->volume);
    data->tracks = NULL;
    data->playing = NULL;
    data->volume = NULL;
    data->track_count = 0;
    return MALLOC_ERROR;
  }

  for (int i = 0; i < count; i++) {
    data->tracks[i] = tracks[i];
    data->volume[i] = tracks[i].default_volume;
    data->playing[i] = false;
  }
  data->track_count = count;
  data->master_volume = NOISE_MASTER_VOLUME;
  data->master_sel_color = 15;
  data->selected = 0;

  /* Resize the internal miniaudio sound array */
  free(noise_sounds);
  noise_sounds = (ma_sound*)calloc(count, sizeof(ma_sound));
  noise_sound_count = count;
  for (int i = 0; i < count; i++) memset(&noise_sounds[i], 0, sizeof(ma_sound));

  return NO_ERROR;
}

/**
 * Initialise a WhiteNoiseData struct to empty state (zeroed).
 * Tracks should be registered via RegisterWhiteNoiseTracks after
 * calling this function.
 * @param data Pointer to the WhiteNoiseData struct to initialise
 */
void InitWhiteNoiseData(WhiteNoiseData* data) {
  memset(data, 0, sizeof(WhiteNoiseData));
}

/**
 * Initialize the persistent noise audio engine.
 * Starts the miniaudio engine for white noise playback.
 * Safe to call multiple times (idempotent).
 * @return NO_ERROR on success, AUDIO_ENGINE_INIT_ERROR on failure
 */
ErrorType InitNoiseAudio(void) {
  if (noise_engine_initialized) return NO_ERROR;

  ma_result result = ma_engine_init(NULL, &noise_engine);
  if (result != MA_SUCCESS) {
    LogError("InitNoiseAudio", AUDIO_ENGINE_INIT_ERROR);
    return AUDIO_ENGINE_INIT_ERROR;
  }

  result = ma_sound_group_init(&noise_engine, 0, NULL, &notification_group);
  if (result != MA_SUCCESS) {
    ma_engine_uninit(&noise_engine);
    LogError("InitNoiseAudio", AUDIO_ENGINE_INIT_ERROR);
    return AUDIO_ENGINE_INIT_ERROR;
  }

  noise_engine_initialized = true;
  return NO_ERROR;
}

/**
 * Shut down the noise audio engine.
 * Stops all active sounds and uninitializes the engine.
 * Safe to call multiple times (idempotent).
 */
void ShutdownNoiseAudio(void) {
  if (!noise_engine_initialized) return;

  for (int i = 0; i < noise_sound_count; i++) {
    if (ma_sound_is_playing(&noise_sounds[i])) ma_sound_stop(&noise_sounds[i]);
    ma_sound_uninit(&noise_sounds[i]);
  }

  while (looping_playbacks) {
    LoopingPlayback* playback = looping_playbacks;
    looping_playbacks = playback->next;
    ma_sound_stop(&playback->sound);
    ma_sound_uninit(&playback->sound);
    free(playback);
  }

  ma_sound_group_stop(&notification_group);
  ma_sound_group_uninit(&notification_group);
  ma_engine_uninit(&noise_engine);
  noise_engine_initialized = false;
}

/**
 * Start playing a noise track on loop.
 * Loads the ambient sound file and begins playback.
 * If the track is already active it is stopped first.
 * @param track_index Index of the track (0 .. track_count-1)
 * @param sound_path  Path to the audio file to load
 * @param volume      Volume level (0.0 .. 1.0, includes master scaling)
 * @return NO_ERROR on success, or an error code on failure
 */
ErrorType NoiseStartTrack(int track_index, const char* sound_path,
                          float volume) {
  if (track_index < 0 || track_index >= noise_sound_count)
    return INVALID_TRACK_INDEX;

  if (NOTIFICATIONS_SOUND == 0 || WSL != 0) return NO_ERROR;

  if (!noise_engine_initialized) {
    ErrorType err = InitNoiseAudio();
    if (err != NO_ERROR) return err;
  }

  if (ma_sound_is_playing(&noise_sounds[track_index])) {
    ma_sound_stop(&noise_sounds[track_index]);
    ma_sound_uninit(&noise_sounds[track_index]);
  }

  ma_result result = ma_sound_init_from_file(&noise_engine, sound_path, 0, NULL,
                                             NULL, &noise_sounds[track_index]);
  if (result != MA_SUCCESS) {
    LogError("NoiseStartTrack", AUDIO_PLAYBACK_ERROR);
    return AUDIO_INIT_ERROR;
  }

  ma_sound_set_looping(&noise_sounds[track_index], true);
  ma_sound_set_volume(&noise_sounds[track_index], volume);

  result = ma_sound_start(&noise_sounds[track_index]);
  if (result != MA_SUCCESS) {
    LogError("NoiseStartTrack", AUDIO_PLAYBACK_ERROR);
    ma_sound_uninit(&noise_sounds[track_index]);
    return AUDIO_START_ERROR;
  }

  return NO_ERROR;
}

/**
 * Stop playing a noise track.
 * Unloads the sound resource and silences the track.
 * Safe to call on non-playing tracks.
 * @param track_index Index of the track (0 .. track_count-1)
 */
void NoiseStopTrack(int track_index) {
  if (!noise_engine_initialized) return;
  if (track_index < 0 || track_index >= noise_sound_count) {
    LogError("NoiseStopTrack", AUDIO_PLAYBACK_ERROR);
    return;
  }

  if (ma_sound_is_playing(&noise_sounds[track_index])) {
    ma_sound_stop(&noise_sounds[track_index]);
    ma_sound_uninit(&noise_sounds[track_index]);
    memset(&noise_sounds[track_index], 0, sizeof(ma_sound));
  }
}

/**
 * Set the volume of an actively playing noise track.
 * Only affects tracks that are currently playing.
 * @param track_index Index of the track (0 .. track_count-1)
 * @param volume      Volume level (0.0 .. 1.0, includes master scaling)
 */
void NoiseSetVolume(int track_index, float volume) {
  if (!noise_engine_initialized) return;
  if (track_index < 0 || track_index >= noise_sound_count) {
    LogError("NoiseSetVolume", INVALID_TRACK_INDEX);
    return;
  }

  if (ma_sound_is_playing(&noise_sounds[track_index]))
    ma_sound_set_volume(&noise_sounds[track_index], volume);
}
