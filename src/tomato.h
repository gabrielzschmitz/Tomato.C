#ifndef TOMATO_H_
#define TOMATO_H_

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>

#include "bar.h"
#include "config.h"

#define PALETTE_SIZE   (COLOR_WHITE - COLOR_BLACK + 1)
#define NO_COLOR       -1
#define MAX_ANIMATIONS 7
#define FPMS           1000.0 / FPS

#ifdef DEBUG_FLAG
#define DEBUG 1
#else
#define DEBUG 0
#endif

/* Defining error handling enum */
typedef enum {
  NO_ERROR,
  ANIMATION_DESERIALIZATION_ERROR,
  WINDOW_CREATION_ERROR,
  WINDOW_DELETION_ERROR,
  MALLOC_ERROR,
  INVALID_INPUT,
  INVALID_CONFIG,
  TOO_SMALL_SCREEN,
} ErrorType;

/* Defining current mode enum */
typedef enum {
  MAIN_MENU,
  WORK_TIME,
  SHORT_PAUSE,
  LONG_PAUSE,
  NOTES,
  HELP,
  CONTINUE,
} SceneType;

/* Defining input mode enum */
typedef enum { NORMAL, INSERT, VISUAL } InputMode;

/* Defining the app struct */
struct AppData {
  struct Screen *screen;
  struct StatusBar *status_bar;

  InputMode input_mode;
  int user_input;
  bool block_input;

  struct Rollfilm *animations[MAX_ANIMATIONS];
  bool is_paused;
  bool running;
};

#endif /* TOMATO_H_ */
