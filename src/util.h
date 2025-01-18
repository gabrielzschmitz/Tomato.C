#ifndef UTIL_H_
#define UTIL_H_

#include <ncurses.h>

typedef struct AppData AppData;

/* Enum for different types of icons */
typedef enum {
  NERD_ICONS, /* Icons using nerd symbols */
  EMOJIS,     /* Icons represented by emojis */
  ASCII,      /* Icons using ASCII characters */
} IconType;

/* Structure for storing a 2D vector */
typedef struct {
  int x;
  int y;
} Vector2D;

/* Structure for storing dimentions of a struct */
typedef struct {
  int width;
  int height;
} Dimensions;

/* Set text foreground and background colors */
void SetColor(short int fg, short int bg, chtype attr);

/* Get the widest and tallest animation */
Dimensions GetWidestAndTallestAnimation(AppData* app);

/* Flush input */
void ResetInput(AppData* app);

/* Check if the ICON_TYPE from the config.h is valid */
bool CheckConfigIconType();

/* Get the ICON_TYPE from the config.h */
IconType GetConfigIconType();

/* Helper function to count UTF-16 characters */
int UTF16CharCount(const char* str);

/* Helper function to calculate the number of UTF-16 characters that fit within
 * the maximum width */
int UTF16CharFitWidth(const char* str, int max_width, int* byte_count);

/* Helper function to get the current time as a string */
void GetCurrentTime(char* buffer, size_t buffer_size);

/* Returns which int is larger */
int Max(int a, int b);

/* Function to get the current time in milliseconds */
double GetCurrentTimeMS(void);

/* Convert elapsed seconds and total time in minutes to a time string */
char* FormatRemainingTime(int elapsed_seconds, int total_minutes);

/* Check if a pomodoro step has ended */
bool StepEnded(int elapsed_seconds, int total_minutes);

/* Check if current_step is in the array */
bool IsCurrentStepInList(const int* array, size_t array_size, int current_step);

#endif /* UTIL_H_ */
