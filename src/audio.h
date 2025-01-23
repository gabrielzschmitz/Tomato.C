#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdbool.h>
#include "error.h"

/* Play audio from path using miniaudio */
ErrorType PlayAudio(const char* audio_path, const float volume,
                    const bool loop);

#endif /* AUDIO_H */
