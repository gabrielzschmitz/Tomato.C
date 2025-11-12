#ifndef TOMATO_H_
#define TOMATO_H_

#include <ncurses.h>
#include <stdbool.h>

#include "bar.h"
#include "ui.h"

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

/* Defining the PomodoroConfig struct */
typedef struct {
  int total_cycles;      /* Total Pomodoro cycles configured */
  int current_cycle;     /* Current Pomodoro cycle */
  int work_time;         /* Work time duration (minutes) */
  int short_pause_time;  /* Short pause duration (minutes) */
  int long_pause_time;   /* Long pause duration (minutes) */
  int current_step;      /* Current step (work, short pause, etc.) */
  int current_step_time; /* Time in the current step (seconds) */
  int last_step_time;    /* Time in the last step (seconds) */
  double delta_time_ms;  /* Elapsed time in milliseconds since last frame */
} PomodoroData;

/* Defining the app struct */
struct AppData {
  struct Screen* screen;
  struct StatusBar* status_bar;
  struct Menu* menus[MAX_MENUS];
  struct FloatingDialog* popup_dialog;
  int current_menu;

  int user_input;
  int last_input;
  bool block_input;

  struct Rollfilm* animations[MAX_ANIMATIONS];
  bool is_paused;
  bool running;

  PomodoroData pomodoro_data;
};

#endif /* TOMATO_H_ */
