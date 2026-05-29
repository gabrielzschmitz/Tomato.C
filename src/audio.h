#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdbool.h>

#include "error.h"

#define NOISE_TRACK_COUNT 4

/**
 * White noise playback state for the noise slide dialog.
 * Tracks which ambient sounds are playing, their volumes,
 * and which track is currently selected.
 */
typedef struct {
  bool playing[NOISE_TRACK_COUNT]; /**< Per-track on/off state */
  int volume[NOISE_TRACK_COUNT];   /**< Per-track volume 0-100 */
  int master_volume;               /**< Master volume 0-100 */
  int selected; /**< Currently selected track index (-1/0..NOISE_TRACK_COUNT) */
} WhiteNoiseData;

/**
 * ---------------------------------------------------------------------------
 * Audio Plyback
 * ---------------------------------------------------------------------------
 */

/**
 * Play audio from path using miniaudio.
 * Spawns a detached thread to handle playback asynchronously.
 * Sound is disabled on WSL and when NOTIFICATIONS_SOUND is 0.
 * @param audio_path Path to the audio file to play
 * @param volume Volume level for playback (0.0 to 1.0)
 * @param loop true to loop playback, false to play once
 * @return NO_ERROR on success, error code on failure
 */
ErrorType PlayAudio(const char* audio_path, const float volume,
                    const bool loop);

/**
 * ---------------------------------------------------------------------------
 * Noise Audio
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize the persistent noise audio engine.
 * Starts the miniaudio engine for white noise playback.
 * Safe to call multiple times (idempotent).
 * @return NO_ERROR on success, AUDIO_ENGINE_INIT_ERROR on failure
 */
ErrorType InitNoiseAudio(void);

/**
 * Shut down the noise audio engine.
 * Stops all active sounds and uninitializes the engine.
 * Safe to call multiple times (idempotent).
 */
void ShutdownNoiseAudio(void);

/**
 * Start playing a noise track on loop.
 * Loads the ambient sound file and begins playback.
 * If the track is already playing, it is restarted.
 * @param track_index Index of the track (0 to NOISE_TRACK_COUNT-1)
 * @param volume Volume level (0.0 to 1.0, includes master scaling)
 * @return NO_ERROR on success, or an error code on failure
 */
ErrorType NoiseStartTrack(int track_index, float volume);

/**
 * Stop playing a noise track.
 * Unloads the sound resource and silences the track.
 * Safe to call on non-playing tracks.
 * @param track_index Index of the track (0 to NOISE_TRACK_COUNT-1)
 */
void NoiseStopTrack(int track_index);

/**
 * Set the volume of an actively playing noise track.
 * Only affects tracks that are currently playing.
 * @param track_index Index of the track (0 to NOISE_TRACK_COUNT-1)
 * @param volume Volume level (0.0 to 1.0, includes master scaling)
 */
void NoiseSetVolume(int track_index, float volume);

#endif /* AUDIO_H */
