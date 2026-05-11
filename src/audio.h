#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdbool.h>

#include "error.h"

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

#endif /* AUDIO_H */
