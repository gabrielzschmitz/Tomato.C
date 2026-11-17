#ifndef TOMATO_H_
#define TOMATO_H_

#include <ncurses.h>
#include <stdbool.h>

#include "audio.h"
#include "bar.h"
#include "notes.h"
#include "notify.h"
#include "pomodoro.h"
#include "ui.h"

#define PALETTE_SIZE (COLOR_WHITE - COLOR_BLACK + 1)
#define MAX_ANIMATIONS 8
#define MAX_ICON_TYPES 3
#define MAX_MENUS 2
#define MAX_INPUT_MODES 3
#define MAX_RECENT_SESSIONS 3
#define HISTORY_VISIBLE_MONTHS 5
#define MAX_ERROR_ENTRIES 128
#define FPMS (1000.0 / (FPS))

#ifdef DEBUG_FLAG
#define DEBUG 1
#else
#define DEBUG 0
#endif

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

  int user_input;      /**< Most recent user keypress */
  int last_input;      /**< Previous keypress for edge detection */
  bool block_input;    /**< True to ignore user input temporarily */
  bool frozen;         /**< True if app is frozen due to critical error */
  bool redraw_pending; /**< True when visible state needs a redraw */

  struct Rollfilm* icon_animations
    [MAX_ICON_TYPES]
    [MAX_ANIMATIONS];           /**< Loaded animations for all icon types */
  struct Rollfilm** animations; /**< Pointer to active icon type's animations */
  bool is_paused;               /**< Whether the timer is paused */
  bool running; /**< Whether the app is running (false = exit) */

  PomodoroData pomodoro_data; /**< Pomodoro timer state */
  WhiteNoiseData noise_data;  /**< White noise playback state */
  NotesData* notes;           /**< Notes/text editor data */

  Notification notification_work;        /**< Work notification */
  Notification notification_short_pause; /**< Short pause notification */
  Notification notification_long_pause;  /**< Long pause notification */
  Notification notification_end_cycle;   /**< End-of-cycle notification */

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
