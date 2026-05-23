#ifndef UTIL_H_
#define UTIL_H_

#include <ncurses.h>

typedef struct AppData AppData;

#define NO_COLOR -1 /* Defining a blank color type */

/**
 * Enum for different types of icons used in the UI.
 * Determines which icon set to use (Nerd Fonts, emojis, or ASCII).
 */
typedef enum {
  NERD_ICONS, /**< Icons using Nerd Font symbols */
  EMOJIS,     /**< Icons represented by emojis */
  ASCII,      /**< Icons using ASCII characters */
} IconType;

/**
 * Structure for storing a 2D vector (position or offset).
 * Used for screen coordinates and offsets.
 */
typedef struct {
  int x; /**< Horizontal coordinate */
  int y; /**< Vertical coordinate */
} Vector2D;

/**
 * Structure for storing dimensions (width and height).
 * Used for sizing panels, dialogs, and other elements.
 */
typedef struct {
  int width;  /**< Horizontal dimension */
  int height; /**< Vertical dimension */
} Dimensions;

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
void SetColor(short int fg, short int bg, chtype attr);

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
void GetCurrentTime(char* buffer, size_t buffer_size);

/**
 * Get the current time in milliseconds.
 * Used for frame timing and animation updates.
 * @return Current time in milliseconds since epoch
 */
double GetCurrentTimeMS(void);

/**
 * Convert elapsed seconds and total time to formatted string.
 * Format: "MM:SS" remaining or "00:00" if time exceeded.
 * @param elapsed_seconds Seconds elapsed in current step
 * @param total_minutes Total time for the step in minutes
 * @return Newly allocated string (caller must free), or NULL on failure
 */
char* FormatRemainingTime(int elapsed_seconds, int total_minutes);

/**
 * ---------------------------------------------------------------------------
 * Config
 * ---------------------------------------------------------------------------
 */

/**
 * Check if the ICON_TYPE from config.h is valid.
 * @return true if valid, false otherwise
 */
bool CheckConfigIconType(void);

/**
 * Get the ICON_TYPE from config.h as an enum value.
 * @return IconType value based on config
 */
IconType GetConfigIconType(void);

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
int UTF16CharCount(const char* str);

/**
 * Calculate how many UTF-16 characters fit within max width.
 * Used for text wrapping with multi-byte characters.
 * @param str UTF-8 string to measure
 * @param max_width Maximum width in columns
 * @param byte_count Output: number of bytes that fit
 * @return Number of UTF-16 characters that fit
 */
int UTF16CharFitWidth(const char* str, int max_width, int* byte_count);

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
int Max(int a, int b);

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
bool IsStepEnded(int elapsed_seconds, int total_minutes);

/**
 * Check if current_step is in the array of steps.
 * @param array Array of step values to check
 * @param array_size Number of elements in array
 * @param current_step Step value to find
 * @return true if current_step found in array, false otherwise
 */
bool IsCurrentStepInList(const int* array, size_t array_size, int current_step);

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
void ResetInput(AppData* app);

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
Dimensions GetWidestAndTallestAnimation(AppData* app);

#endif /* UTIL_H_ */
