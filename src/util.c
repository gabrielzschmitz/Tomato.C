#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

/* O_NOFOLLOW is not exposed by all macOS SDK configurations. */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0x0100
#endif

#include "config.h"
#include "error.h"
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

  if (!BG_TRANSPARENCY && bg == NO_COLOR) bg = COLOR_BLACK;

  if (bg == NO_COLOR && BG_TRANSPARENCY) {
    int pair = (fg + 1) + (PALETTE_SIZE * PALETTE_SIZE);
    if (pair < COLOR_PAIRS)
      color = COLOR_PAIR(pair);
    else
      color = COLOR_PAIR((COLOR_BLACK * PALETTE_SIZE) + fg + 1);
  } else
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
 * Clock used for timer and animation timestamps.
 *
 * CLOCK_MONOTONIC deliberately stops while the machine is suspended: a
 * pomodoro session should not expire while the user's laptop is asleep.
 * Define TOMATO_TIMER_CLOCK as CLOCK_BOOTTIME at build time to count
 * suspend time instead (Linux only).
 */
#ifndef TOMATO_TIMER_CLOCK
#define TOMATO_TIMER_CLOCK CLOCK_MONOTONIC
#endif

/**
 * Return the clock used by GetCurrentTimeMS.
 * @return clockid_t passed to clock_gettime
 */
clockid_t GetTimerClock(void) { return TOMATO_TIMER_CLOCK; }

/**
 * Get the current time in milliseconds.
 * Used for frame timing and animation updates.
 * @return Current time in milliseconds since epoch
 */
double GetCurrentTimeMS(void) {
  const double SECONDS_TO_MILLISECONDS = 1000.0;
  const double NANOSECONDS_TO_MILLISECONDS = 1.0 / 1000000.0;

  struct timespec current_time;
  if (clock_gettime(GetTimerClock(), &current_time) != 0) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * SECONDS_TO_MILLISECONDS) +
           (tv.tv_usec * NANOSECONDS_TO_MILLISECONDS * 1000.0);
  }

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
  if (!time_string) {
    LogError("FormatRemainingTime", MALLOC_ERROR);
    return NULL;
  }

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
  switch (g_config.visual.icons_index) {
    case 0:
      return NERD_ICONS;
    case 1:
      return EMOJIS;
    case 2:
      return ASCII;
    default:
      return ASCII;
  }
}

/**
 * ---------------------------------------------------------------------------
 * UTF-8 String Utilities
 * ---------------------------------------------------------------------------
 */

/**
 * Return the length of the UTF-8 character at s.
 *
 * Truncated sequences and invalid continuation bytes are treated as a
 * single byte so callers never read past the terminating NUL.
 * @param s Start of the sequence
 * @param remaining Bytes left in the string, excluding the NUL
 * @return Sequence length in bytes, always at least 1 when remaining > 0
 */
static size_t utf8CharLen(const unsigned char* s, size_t remaining) {
  if (remaining == 0) return 0;

  size_t len;
  if ((s[0] & 0x80) == 0)
    return 1;
  else if ((s[0] & 0xE0) == 0xC0)
    len = 2;
  else if ((s[0] & 0xF0) == 0xE0)
    len = 3;
  else if ((s[0] & 0xF8) == 0xF0)
    len = 4;
  else
    return 1;

  if (len > remaining) return 1;
  for (size_t i = 1; i < len; i++)
    if ((s[i] & 0xC0) != 0x80) return 1;
  return len;
}

/**
 * Count UTF-16 characters in a string.
 * @param str UTF-8 string to count
 * @return Number of UTF-16 code units
 */
int UTF16CharCount(const char* str) {
  int count = 0;
  const unsigned char* s = (const unsigned char*)str;
  size_t remaining = strlen(str);

  while (remaining > 0) {
    size_t len = utf8CharLen(s, remaining);
    if (len == 4 && GetConfigIconType() == EMOJIS) count++;
    s += len;
    remaining -= len;
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
  size_t remaining = strlen(str);
  *byte_count = 0;

  while (remaining > 0 && char_count < max_width) {
    size_t len = utf8CharLen((const unsigned char*)s, remaining);
    s += len;
    remaining -= len;
    (*byte_count) += (int)len;
    char_count++;
  }

  return char_count;
}

/**
 * Return the start offset of the UTF-8 character before pos.
 * @param str UTF-8 string
 * @param pos Current byte offset
 * @return Byte offset of the previous character start (0 at the beginning)
 */
int UTF8PrevCharStart(const char* str, int pos) {
  if (!str || pos <= 0) return 0;
  int i = pos - 1;
  while (i > 0 && ((unsigned char)str[i] & 0xC0) == 0x80) i--;
  return i;
}

/**
 * Return the start offset of the UTF-8 character after pos.
 * @param str UTF-8 string
 * @param len Length of str in bytes
 * @param pos Current byte offset
 * @return Byte offset of the next character start (len at the end)
 */
int UTF8NextCharStart(const char* str, int len, int pos) {
  if (!str || pos < 0) return 0;
  if (pos >= len) return len;

  unsigned char c = (unsigned char)str[pos];
  int step = 1;
  if ((c & 0xE0) == 0xC0)
    step = 2;
  else if ((c & 0xF0) == 0xE0)
    step = 3;
  else if ((c & 0xF8) == 0xF0)
    step = 4;

  int next = pos + step;
  return next < len ? next : len;
}

/**
 * Move pos to the nearest preceding UTF-8 character boundary.
 * @param str UTF-8 string
 * @param len Length of str in bytes
 * @param pos Byte offset to align
 * @return Aligned byte offset within [0, len]
 */
int UTF8AlignCursor(const char* str, int len, int pos) {
  if (!str) return 0;
  if (pos < 0) return 0;
  if (pos > len) pos = len;
  while (pos > 0 && pos < len && ((unsigned char)str[pos] & 0xC0) == 0x80)
    pos--;
  return pos;
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
 * Get the minimum terminal size needed to display all UI elements.
 * Accounts for the largest sprite, page indicator, status bar, and error line.
 * @param app Pointer to the application data
 * @return Dimensions struct with min required width and height
 */
Dimensions GetWidestAndTallestAnimation(AppData* app) {
  int widest = 0;
  int tallest = 0;

  for (int i = 0; i < MAX_ANIMATIONS; i++) {
    if (app->animations[i] == NULL) continue;
    if (app->animations[i]->frame_width > widest)
      widest = app->animations[i]->frame_width;
    if (app->animations[i]->frame_height > tallest)
      tallest = app->animations[i]->frame_height;
  }

  /* Add overhead for page indicator line, status bar, and error line */
  tallest += UI_OVERHEAD_LINES;

  return (Dimensions){widest, tallest};
}

/**
 * ---------------------------------------------------------------------------
 * File I/O
 * ---------------------------------------------------------------------------
 */

/**
 * Open a file for writing, refusing to follow symlinks.
 * Internally uses open() with O_NOFOLLOW, then wraps the fd with fdopen().
 * @param path File path to open
 * @param mode File mode ("r", "w", "a", "wb", "ab", etc.)
 * @return FILE pointer on success, NULL on error (sets errno)
 */
FILE* FOpenNoFollow(const char* path, const char* mode) {
  if (path == NULL || mode == NULL) return NULL;

  int flags = O_NOFOLLOW;
  if (mode[0] == 'a' || (mode[0] == 'a' && mode[1] == 'b'))
    flags |= O_WRONLY | O_APPEND | O_CREAT;
  else if (mode[0] == 'w')
    flags |= O_WRONLY | O_TRUNC | O_CREAT;
  else /* "rb", "r" */
    flags |= O_RDONLY;

  int fd = open(path, flags, 0644);
  if (fd == -1) return NULL;
  return fdopen(fd, mode);
}

/**
 * Create a directory (and parents) if it does not exist.
 * @param dir Path to the directory to create
 * @return 0 on success, -1 on failure (sets errno)
 */
int EnsureDir(const char* dir) {
  char tmp[4096];
  size_t len = strlen(dir);
  if (len >= sizeof(tmp) || len == 0) {
    errno = len == 0 ? EINVAL : ENAMETOOLONG;
    return -1;
  }
  memcpy(tmp, dir, len + 1);

  /* Skip leading '/' so we don't call mkdir("") */
  size_t start = (tmp[0] == '/') ? 1 : 0;
  for (size_t i = start; i < len; i++) {
    if (tmp[i] == '/') {
      tmp[i] = '\0';
      if (mkdir(tmp, 0755) == -1 && errno != EEXIST) {
        LogError("EnsureDir", FILE_ERROR);
        return -1;
      }
      tmp[i] = '/';
    }
  }
  if (mkdir(tmp, 0755) == -1 && errno != EEXIST) {
    LogError("EnsureDir", FILE_ERROR);
    return -1;
  }
  return 0;
}
