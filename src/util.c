#include "util.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "tomato.h"

/**
 * ---------------------------------------------------------------------------
 * Colors & Rendering
 * ---------------------------------------------------------------------------
 */

/**
 * Set text foreground and background colors.
 * Configures ncurses color pair and attributes.
 * @param fg Foreground color index (COLOR_*)
 * @param bg Background color index (COLOR_*)
 * @param attr Additional ncurses attributes (A_BOLD, etc.)
 */
void SetColor(short int fg, short int bg, chtype attr) {
  chtype color;

  /* Handle background color when BGTRANSPARENCY is disabled */
  if (!BG_TRANSPARENCY && bg == NO_COLOR) bg = COLOR_BLACK;

  if (bg == NO_COLOR && BG_TRANSPARENCY)
    color = COLOR_PAIR((fg + 1) + (PALETTE_SIZE * PALETTE_SIZE));
  else
    color = COLOR_PAIR((bg * PALETTE_SIZE) + fg + 1);

  color |= attr;
  attrset(color);
}

/**
 * ---------------------------------------------------------------------------
 * Time
 * ---------------------------------------------------------------------------
 */

/**
 * Get the current time as a formatted string.
 * @param buffer Output buffer for the time string
 * @param buffer_size Size of the buffer
 */
void GetCurrentTime(char* buffer, size_t buffer_size) {
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  strftime(buffer, buffer_size, "%H:%M", tm_info);
}

/**
 * Get the current time in milliseconds.
 * Used for frame timing and animation updates.
 * @return Current time in milliseconds since epoch
 */
double GetCurrentTimeMS(void) {
  const double SECONDS_TO_MILLISECONDS = 1000.0;
  const double NANOSECONDS_TO_MILLISECONDS = 1.0 / 1000000.0;

  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);

  double ms = (current_time.tv_sec * SECONDS_TO_MILLISECONDS) +
              (current_time.tv_nsec * NANOSECONDS_TO_MILLISECONDS);
  return ms;
}

/**
 * Convert elapsed seconds and total time to formatted string.
 * Format: "MM:SS" remaining or "00:00" if time exceeded.
 * @param elapsed_seconds Seconds elapsed in current step
 * @param total_minutes Total time for the step in minutes
 * @return Newly allocated string (caller must free), or NULL on failure
 */
char* FormatRemainingTime(int elapsed_seconds, int total_minutes) {
  int total_seconds = total_minutes * 60;
  int remaining_seconds = total_seconds - elapsed_seconds;

  if (remaining_seconds < 0) remaining_seconds = 0;

  int minutes = remaining_seconds / 60;
  int seconds = remaining_seconds % 60;

  char* time_string = (char*)malloc(16);
  if (!time_string) return NULL;

  snprintf(time_string, 16, "%02d:%02d", minutes, seconds);
  return time_string;
}

/**
 * ---------------------------------------------------------------------------
 * Config
 * ---------------------------------------------------------------------------
 */

/**
 * Check if the ICON_TYPE from config.h is valid.
 * @return true if valid, false otherwise
 */
bool CheckConfigIconType(void) {
  if (strcmp(ICONS, "nerd-icons") == 0)
    return true;
  else if (strcmp(ICONS, "emojis") == 0)
    return true;
  else if (strcmp(ICONS, "ascii") == 0)
    return true;
  else
    return false;
}

/**
 * Get the ICON_TYPE from config.h as an enum value.
 * @return IconType value based on config
 */
IconType GetConfigIconType(void) {
  IconType type = ASCII;
  if (strcmp(ICONS, "nerd-icons") == 0)
    type = NERD_ICONS;
  else if (strcmp(ICONS, "emojis") == 0)
    type = EMOJIS;
  else if (strcmp(ICONS, "ascii") == 0)
    type = ASCII;
  return type;
}

/**
 * ---------------------------------------------------------------------------
 * UTF-8 String Utilities
 * ---------------------------------------------------------------------------
 */

/**
 * Count UTF-16 characters in a string.
 * @param str UTF-8 string to count
 * @return Number of UTF-16 code units
 */
int UTF16CharCount(const char* str) {
  int count = 0;
  const unsigned char* s = (const unsigned char*)str;

  while (*s) {
    if ((*s & 0x80) == 0)
      s++; /* 1-byte sequence (ASCII) */
    else if ((*s & 0xE0) == 0xC0)
      s += 2; /* 2-byte sequence */
    else if ((*s & 0xF0) == 0xE0)
      s += 3; /* 3-byte sequence */
    else if ((*s & 0xF8) == 0xF0) {
      if (GetConfigIconType() == EMOJIS) count++;
      s += 4; /* 4-byte sequence */
    } else
      s++; /* Invalid UTF-8 byte */
    count++;
  }

  return count;
}

/**
 * Calculate how many UTF-16 characters fit within max width.
 * Used for text wrapping with multi-byte characters.
 * @param str UTF-8 string to measure
 * @param max_width Maximum width in columns
 * @param byte_count Output: number of bytes that fit
 * @return Number of UTF-16 characters that fit
 */
int UTF16CharFitWidth(const char* str, int max_width, int* byte_count) {
  int char_count = 0;
  const char* s = str;
  *byte_count = 0;

  while (*s && char_count < max_width) {
    if ((*s & 0x80) == 0) {
      s++; /* 1-byte sequence (ASCII) */
      (*byte_count)++;
    } else if ((*s & 0xE0) == 0xC0) {
      s += 2; /* 2-byte sequence */
      (*byte_count) += 2;
    } else if ((*s & 0xF0) == 0xE0) {
      s += 3; /* 3-byte sequence */
      (*byte_count) += 3;
    } else if ((*s & 0xF8) == 0xF0) {
      s += 4; /* 4-byte sequence */
      (*byte_count) += 4;
    } else {
      s++; /* Invalid UTF-8 byte */
      (*byte_count)++;
    }
    char_count++;
  }

  return char_count;
}

/**
 * ---------------------------------------------------------------------------
 * Math
 * ---------------------------------------------------------------------------
 */

/**
 * Return the larger of two integers.
 * @param a First integer
 * @param b Second integer
 * @return Maximum of a and b
 */
int Max(int a, int b) { return a > b ? a : b; }

/**
 * ---------------------------------------------------------------------------
 * Pomodoro Logic
 * ---------------------------------------------------------------------------
 */

/**
 * Check if a pomodoro step has ended based on elapsed time.
 * @param elapsed_seconds Seconds elapsed in current step
 * @param total_minutes Total time for the step in minutes
 * @return true if elapsed >= total, false otherwise
 */
bool IsStepEnded(int elapsed_seconds, int total_minutes) {
  int total_seconds = total_minutes * 60;
  int remaining_seconds = total_seconds - elapsed_seconds;

  if (remaining_seconds <= 0) return true;
  return false;
}

/**
 * Check if current_step is in the array of steps.
 * @param array Array of step values to check
 * @param array_size Number of elements in array
 * @param current_step Step value to find
 * @return true if current_step found in array, false otherwise
 */
bool IsCurrentStepInList(const int* array, size_t array_size,
                         int current_step) {
  for (size_t i = 0; i < array_size; i++)
    if (array[i] == current_step) return true;
  return false;
}

/**
 * ---------------------------------------------------------------------------
 * Input
 * ---------------------------------------------------------------------------
 */

/**
 * Flush input buffer and reset input state.
 * Discards any pending keypresses.
 * @param app Pointer to the application data
 */
void ResetInput(AppData* app) {
  app->last_input = -1;
  app->user_input = -1;
}

/**
 * ---------------------------------------------------------------------------
 * Animation
 * ---------------------------------------------------------------------------
 */

/**
 * Get the widest and tallest animation from loaded animations.
 * Used to calculate panel sizing.
 * @param app Pointer to the application data
 * @return Dimensions struct with max width and height
 */
Dimensions GetWidestAndTallestAnimation(AppData* app) {
  int widest = 0;
  int tallest = 0;

  for (int i = 0; i < MAX_ANIMATIONS; i++) {
    if (app->animations[i]->frame_width > widest)
      widest = app->animations[i]->frame_width;
    if (app->animations[i]->frame_height > tallest)
      tallest = app->animations[i]->frame_height;
  }

  return (Dimensions){widest, tallest};
}
