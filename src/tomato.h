#ifndef TOMATO_H_
#define TOMATO_H_

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>

#include "bar.h"
#include "config.h"

#define PALETTE_SIZE (COLOR_WHITE - COLOR_BLACK + 1)
#define NO_COLOR -1
#define MAX_ANIMATIONS 7
#define MAX_MENUS 2
#define MAX_INPUT_MODES 3
#define FPMS 1000.0 / FPS

#ifdef DEBUG_FLAG
#define DEBUG 1
#else
#define DEBUG 0
#endif

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
typedef enum {
  NORMAL = 1 << 0,
  INSERT = 1 << 1,
  VISUAL = 1 << 2,
} InputMode;

/* Defining the app struct */
struct AppData {
  struct Screen* screen;
  struct StatusBar* status_bar;
  struct Menu* menus[MAX_MENUS];
  int current_menu;

  int user_input;
  int last_input;
  bool block_input;

  struct Rollfilm* animations[MAX_ANIMATIONS];
  bool is_paused;
  bool running;
};

#endif /* TOMATO_H_ */
