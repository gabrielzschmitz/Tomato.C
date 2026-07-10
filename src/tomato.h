#ifndef TOMATO_H_
#define TOMATO_H_

#include <ncurses.h>
#include <stdbool.h>
#include <time.h>

#include "audio.h"
#include "bar.h"
#include "notes.h"
#include "ui.h"

#define PALETTE_SIZE (COLOR_WHITE - COLOR_BLACK + 1)
#define MAX_ANIMATIONS 8
#define MAX_MENUS 2
#define MAX_INPUT_MODES 3
#define MAX_RECENT_SESSIONS 3
#define HISTORY_VISIBLE_MONTHS 5
#define MAX_ERROR_ENTRIES 128
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
  int total_cycles;     /**< Total number of pomodoro cycles to complete */
  int current_cycle;    /**< Current cycle number (1 to total_cycles) */
  int work_time;        /**< Work time duration in minutes */
  int short_pause_time; /**< Short pause duration in minutes */
  int long_pause_time;  /**< Long pause duration in minutes */
  int current_step; /**< Current step (0=work, 1=short pause, 2=long pause) */
  int current_step_time;  /**< Elapsed time in current step (seconds) */
  int last_step_time;     /**< Time from previous step for comparison */
  int total_elapsed;      /**< Total elapsed time across all steps (seconds) */
  double delta_time_ms;   /**< Elapsed ms since last frame update */
  int session_index;      /**< Current session index for log entries */
  time_t step_start_time; /**< Timestamp when current step started */
  time_t session_start_time; /**< Timestamp when the session was started */
  int status;                /**< 0 = completed, 1 = uncompleted */
} PomodoroData;

/**
 * Stores cursor position within the contribution graph,
 * the scroll anchor (first visible month), the resolved
 * selected date, and scroll offset for the day-detail list.
 */
typedef struct {
  int cursorWeek; /**< Week column index (0 .. totalWeeks-1) */
  int cursorDow;  /**< Day-of-week row (0=Sun .. 6=Sat) */
  int firstYear;  /**< Year of the first of 5 visible months */
  int firstMonth; /**< Month of the first of 5 visible months (1-12) */
  int selYear;    /**< Resolved date — year */
  int selMonth;   /**< Resolved date — month (1-12) */
  int selDay;     /**< Resolved date — day (1-31) */
  int dayScroll;  /**< Scroll offset for day-detail session list */
  int hoverWeek;  /**< Week column under mouse (-1 = none) */
  int hoverDow;   /**< Day-of-week row under mouse */
} HistoryData;

/**
 * Main application data structure.
 * Contains all state and references needed by the application.
 */
struct AppData {
  struct Screen* screen;         /**< Current screen and its panels */
  struct StatusBar* status_bar;  /**< Status bar displaying app info */
  struct Menu* menus[MAX_MENUS]; /**< Array of menu structures */
  struct FloatingDialog*
    popup_dialog;   /**< Currently displayed popup dialog (or NULL) */
  int current_menu; /**< Index of the currently active menu */

  int user_input;   /**< Most recent user keypress */
  int last_input;   /**< Previous keypress for edge detection */
  bool block_input; /**< True to ignore user input temporarily */
  bool frozen;      /**< True if app is frozen due to critical error */

  struct Rollfilm*
    animations[MAX_ANIMATIONS]; /**< Loaded animations for scenes */
  bool is_paused;               /**< Whether the timer is paused */
  bool running; /**< Whether the app is running (false = exit) */

  PomodoroData pomodoro_data; /**< Pomodoro timer state */
  WhiteNoiseData noise_data;  /**< White noise playback state */
  NotesData* notes;           /**< Notes/text editor data */

  ClickRegion click_regions[MAX_CLICK_REGIONS]; /**< Mouse click regions */
  int click_region_count; /**< Number of registered click regions */

  HistoryData history_data; /**< History popup state */

  PrefsState prefs; /**< Preferences data & sub-dialog state */

  struct FloatingDialog* saved_popup; /**< Saved popup for help-menu restore */
  int help_scroll_row;    /**< Scroll offset for help content rows */
  int help_context_scene; /**< Scene bitmask for context-aware help */

  int mouse_x;      /**< Last mouse x coordinate */
  int mouse_y;      /**< Last mouse y coordinate */
  int mouse_bstate; /**< Last mouse button state */
};

#endif /* TOMATO_H_ */
