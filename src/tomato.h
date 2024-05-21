#ifndef TOMATO_H_
#define TOMATO_H_

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>

#include "config.h"

#define PALLETE_SIZE      (COLOR_WHITE - COLOR_BLACK + 1)
#define MAX_COLOR_PAIRS   8
#define NO_COLOR          -1
#define BGTRANSPARENCY    1
#define REAL_SECONDS      sqrt(FPS)
#define MAX_FRAME_COLUMNS 120
#define ANIMATION_COUNT   32

/* Defining error handling enum */
typedef enum {
  NO_ERROR,
  WINDOW_CREATION_ERROR,
  MALLOC_ERROR,
  INVALID_INPUT,
  TOO_SMALL_SCREEN,
} ErrorType;

/* Defining error handling enum */
typedef enum {
  HELP_PAGE,
  NOTEPAD,
  MAIN_MENU,
  WORK_TIME,
  SHORT_BREAK,
  LONG_BREAK,
} ModeType;

/* Structure for storing sprite information */
typedef struct Sprite Sprite;
struct Sprite {
  char* frames[MAX_FRAME_COLUMNS];
  int color_map[MAX_FRAME_COLUMNS][MAX_FRAME_COLUMNS];
  int frame_count;
  int frame_height;
  int current_frame;
};

/* Defining input mode enum */
typedef enum { NORMAL, INSERT, COMMAND } InputMode;

/* Defining the app struct */
typedef struct AppData AppData;
struct AppData {
  double milliseconds;
  int frame_seconds;
  Sprite sprites[ANIMATION_COUNT];

  int screen_width;
  int screen_height;
  int user_input;

  ModeType current_mode;
  bool running;
};

/* Main function */
int main(int argc, char* argv[]);

#endif /* TOMATO_H_ */
