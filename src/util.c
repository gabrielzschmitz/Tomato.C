#include "util.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tomato.h"

/* Set text foreground and background colors */
void SetColor(short int fg, short int bg, chtype attr) {
  chtype color;

  // Handle background color when BGTRANSPARENCY is disabled
  if (!BG_TRANSPARENCY && bg == NO_COLOR) bg = COLOR_BLACK;

  if (bg == NO_COLOR && BG_TRANSPARENCY)
    color = COLOR_PAIR((fg + 1) + (PALETTE_SIZE * PALETTE_SIZE));
  else color = COLOR_PAIR((bg * PALETTE_SIZE) + fg + 1);

  color |= attr;
  attrset(color);
}

/* Get the widest and tallest animation */
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

/* Flush input */
void ResetInput(AppData* app) {
  app->last_input = -1;
  app->user_input = -1;
}

/* Check if the IconType from the config.h is valid */
bool CheckConfigIconType() {
  if (strcmp(ICONS, "nerd-icons") == 0) return true;
  else if (strcmp(ICONS, "emojis") == 0) return true;
  else if (strcmp(ICONS, "ascii") == 0) return true;
  else return false;
}

/* Get the IconType from the config.h */
IconType GetConfigIconType() {
  IconType type;
  if (strcmp(ICONS, "nerd-icons") == 0) type = NERD_ICONS;
  else if (strcmp(ICONS, "emojis") == 0) type = EMOJIS;
  else if (strcmp(ICONS, "ascii") == 0) type = ASCII;
  return type;
}

/* Helper function to count UTF-16 characters */
int UTF16CharCount(const char* str) {
  int count = 0;
  const unsigned char* s = (const unsigned char*)str;

  while (*s) {
    if ((*s & 0x80) == 0) {
      s++; /* 1-byte sequence (ASCII) */
    } else if ((*s & 0xE0) == 0xC0) {
      s += 2; /* 2-byte sequence */
    } else if ((*s & 0xF0) == 0xE0) {
      s += 3; /* 3-byte sequence */
    } else if ((*s & 0xF8) == 0xF0) {
      if (GetConfigIconType() == EMOJIS) count++;
      s += 4; /* 4-byte sequence */
    } else {
      s++; /* Invalid UTF-8 byte */
    }
    count++;
  }

  return count;
}

/* Helper function to calculate the number of UTF-16 characters that fit within
 * the maximum width */
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

/* Helper function to get the current time as a string */
void GetCurrentTime(char* buffer, size_t buffer_size) {
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  strftime(buffer, buffer_size, "%H:%M", tm_info);
}

/* Returns which int is larger */
int Max(int a, int b){
  return a > b ? a : b;
}

/* Function to get the current time in milliseconds */
double GetCurrentTimeMS(void) {
  const double SECONDS_TO_MILLISECONDS = 1000.0;
  const double NANOSECONDS_TO_MILLISECONDS = 1.0 / 1000000.0;

  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);

  double ms = (current_time.tv_sec * SECONDS_TO_MILLISECONDS) +
              (current_time.tv_nsec * NANOSECONDS_TO_MILLISECONDS);
  return ms;
}

/* Convert elapsed seconds and total time in minutes to a time string */
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

/* Check if a pomodoro step has ended */
bool StepEnded(int elapsed_seconds, int total_minutes){
  int total_seconds = total_minutes * 60;
  int remaining_seconds = total_seconds - elapsed_seconds;

  if (remaining_seconds <= 0) return true;
  return false;
}
