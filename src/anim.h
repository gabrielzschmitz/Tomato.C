#ifndef ANIM_H_
#define ANIM_H_

#include <ncurses.h>

#include "tomato.h"

/* Increase animations frames in real life seconds */
void FrameTimer(double* milliseconds, int* frame_seconds);

/* Handle UTF-8 character in line */
int HandleUnicodeCharacter(const char* src, char** dest);

/* Handle color character in line */
int HandleCharacterColor(const char* src, int** dest);

/* Escape special characters in a line */
void EscapeLine(const char* line, char* sprite_line, int* color_line);

/* Copy escaped line to sprite frame */
void CopyLineToFrame(Sprite* sprite, const char* sprite_line,
                     const int* color_line, int* lines_read);

/* Deserialize a single line of sprite */
void DeserializeSpriteLine(const char* line, Sprite* sprite, int* lines_read);

/* Deserialize sprites from file to Sprite struct */
void DeserializeSprite(const char* filename, Sprite* sprite);

/* Break a string using color_map as tokens */
void BreakStringByColorMap(const char* input_string, const int* color_map,
                           size_t length, char*** output_strings,
                           int* num_strings);

/* Returns the length of a array of strings */
int LengthStringArray(char** string, int from, int to);

#endif /* ANIM_H */
