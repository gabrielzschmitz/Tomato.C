#ifndef TOMATO_H_
#define TOMATO_H_

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>

#include "anim.h"
#include "config.h"

#define PALETTE_SIZE   (COLOR_WHITE - COLOR_BLACK + 1)
#define NO_COLOR       -1
#define MAX_ANIMATIONS 6
#define BGTRANSPARENCY 1
#define REAL_SECOND    1000.0 / FPS

/* Defining error handling enum */
typedef enum {
  NO_ERROR,
  WINDOW_CREATION_ERROR,
  WINDOW_DELETION_ERROR,
  MALLOC_ERROR,
  INVALID_INPUT,
  TOO_SMALL_SCREEN,
} ErrorType;

/* Defining current mode enum */
typedef enum {
  HELP_PAGE,
  NOTEPAD,
  MAIN_MENU,
  WORK_TIME,
  SHORT_BREAK,
  LONG_BREAK,
} ModeType;

/* Defining input mode enum */
typedef enum { NORMAL, INSERT, COMMAND } InputMode;

/* Defining the app struct */
typedef struct AppData AppData;
struct AppData {
  Rollfilm *animations[MAX_ANIMATIONS];
  double frame_milliseconds;
  int frame_seconds;

  int screen_width;
  int screen_height;
  int user_input;

  ModeType current_mode;
  bool running;
};

#endif /* TOMATO_H_ */
