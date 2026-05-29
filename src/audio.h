#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdbool.h>

#include "error.h"

/**
 * Definition of a single white noise track registered at startup.
 * Contains all per-track configuration: display name, icon set,
 * audio file path, and default volume.
 */
typedef struct {
  const char* name;   /**< Display name (e.g. "Rain") */
  const char** icons; /**< Icon array indexed by IconType (nerd/emoji/ascii) */
  const char* sound_path; /**< Path to the audio file */
  int default_volume;     /**< Default volume 0-100 */
  int sel_color; /**< Ncurses color (0-15) when this track is selected */
} WhiteNoiseTrackDef;

/**
 * White noise playback state for the noise slide dialog.
 * Tracks which ambient sounds are registered, their playback
 * state, and which track is currently selected.
 * Fields are allocated dynamically by RegisterWhiteNoiseTracks.
 */
typedef struct {
  int track_count;            /**< Number of registered tracks */
  WhiteNoiseTrackDef* tracks; /**< Array of track definitions */
  bool* playing;              /**< Per-track on/off state (track_count) */
  int* volume;                /**< Per-track volume 0-100 (track_count) */
  int master_volume;          /**< Master volume 0-100 */
  int master_sel_color; /**< Ncurses color (0-15) when master is selected */
  int selected;         /**< Currently selected track index (0..track_count) */
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
 * Initialise a WhiteNoiseData struct to empty (zeroed) state.
 * Tracks must be registered via RegisterWhiteNoiseTracks after
 * calling this function.
 * @param data Pointer to the WhiteNoiseData struct to initialise
 */
void InitWhiteNoiseData(WhiteNoiseData* data);

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
 * Register white noise tracks for the noise slide dialog.
 * Frees any previously registered tracks and copies the given
 * definitions.  All tracks start stopped with their default volume.
 * @param data   Pointer to WhiteNoiseData to populate
 * @param tracks Array of WhiteNoiseTrackDef to register
 * @param count  Number of tracks in the array
 * @return NO_ERROR on success, or MALLOC_ERROR on failure
 */
ErrorType RegisterWhiteNoiseTracks(WhiteNoiseData* data,
                                   const WhiteNoiseTrackDef* tracks, int count);

/**
 * Start playing a noise track on loop.
 * Loads the ambient sound file and begins playback.
 * If the track is already playing, it is restarted.
 * @param track_index Index of the track (0 to track_count-1)
 * @param sound_path  Path to the audio file to load
 * @param volume Volume level (0.0 to 1.0, includes master scaling)
 * @return NO_ERROR on success, or an error code on failure
 */
ErrorType NoiseStartTrack(int track_index, const char* sound_path,
                          float volume);

/**
 * Stop playing a noise track.
 * Unloads the sound resource and silences the track.
 * Safe to call on non-playing tracks.
 * @param track_index Index of the track (0 to track_count-1)
 */
void NoiseStopTrack(int track_index);

/**
 * Set the volume of an actively playing noise track.
 * Only affects tracks that are currently playing.
 * @param track_index Index of the track (0 to track_count-1)
 * @param volume Volume level (0.0 to 1.0, includes master scaling)
 */
void NoiseSetVolume(int track_index, float volume);

#endif /* AUDIO_H */
