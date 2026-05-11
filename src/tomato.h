#ifndef TOMATO_H_
#define TOMATO_H_

#include <ncurses.h>
#include <stdbool.h>

#include "bar.h"
#include "notes.h"
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

/**
 * Pomodoro timer data structure.
 * Tracks the current state of the pomodoro cycle.
 */
typedef struct {
  int total_cycles;     /* Total number of pomodoro cycles to complete */
  int current_cycle;    /* Current cycle number (1 to total_cycles) */
  int work_time;        /* Work time duration in minutes */
  int short_pause_time; /* Short pause duration in minutes */
  int long_pause_time;  /* Long pause duration in minutes */
  int current_step;     /* Current step (0=work, 1=short pause, 2=long pause) */
  int current_step_time; /* Elapsed time in current step (seconds) */
  int last_step_time;    /* Time from previous step for comparison */
  double
    delta_time_ms; /* Elapsed time in milliseconds since last frame update */
} PomodoroData;

/**
 * Main application data structure.
 * Contains all state and references needed by the application.
 */
struct AppData {
  struct Screen* screen;         /* Current screen and its panels */
  struct StatusBar* status_bar;  /* Status bar displaying app info */
  struct Menu* menus[MAX_MENUS]; /* Array of menu structures */
  struct FloatingDialog*
    popup_dialog;   /* Currently displayed popup dialog (or NULL) */
  int current_menu; /* Index of the currently active menu */

  int user_input;   /* Most recent user keypress */
  int last_input;   /* Previous keypress for edge detection */
  bool block_input; /* True to ignore user input temporarily */

  struct Rollfilm*
    animations[MAX_ANIMATIONS]; /* Loaded animations for scenes */
  bool is_paused;               /* Whether the timer is paused */
  bool running;                 /* Whether the app is running (false = exit) */

  PomodoroData pomodoro_data; /* Pomodoro timer state */
  NotesData* notes;           /* Notes/text editor data */
};

#endif /* TOMATO_H_ */
