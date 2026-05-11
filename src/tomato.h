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
 * Scene type enum representing the current application view.
 * Used for routing input and determining which UI to display.
 */
typedef enum {
  MAIN_MENU,   /* Main menu scene */
  WORK_TIME,   /* Work session timer scene */
  SHORT_PAUSE, /* Short break timer scene */
  LONG_PAUSE,  /* Long break timer scene */
  NOTES,       /* Notes/text editor scene */
  HELP,        /* Help screen scene */
  CONTINUE,    /* Continue/pause scene */
} SceneType;

/* Scene type bitmasks for key binding filters */
#define SCENE_MAIN_MENU (1 << MAIN_MENU)
#define SCENE_WORK_TIME (1 << WORK_TIME)
#define SCENE_SHORT_PAUSE (1 << SHORT_PAUSE)
#define SCENE_LONG_PAUSE (1 << LONG_PAUSE)
#define SCENE_NOTES (1 << NOTES)
#define SCENE_HELP (1 << HELP)
#define SCENE_CONTINUE (1 << CONTINUE)

/**
 * Input mode enum for the text editor (vim-like modes).
 * Determines how keyboard input is interpreted.
 */
typedef enum {
  DEFAULT = 1 << 0, /* Default mode for menu navigation */
  NORMAL = 1 << 1,  /* Normal mode for text commands and navigation */
  INSERT = 1 << 2,  /* Insert mode for text input */
  VISUAL = 1 << 3,  /* Visual mode for text selection */
} InputMode;

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
