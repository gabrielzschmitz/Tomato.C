#include "init.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "anim.h"
#include "audio.h"
#include "bar.h"
#include "config.h"
#include "error.h"
#include "input.h"
#include "log.h"
#include "notes.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* PRIVATE INIT FUNCTIONS */
/* Components */
static ErrorType initMenus(AppData* app);

static ErrorType debugSeedHistory(void);
static ErrorType initStatusBar(AppData* app);
static ErrorType initAnimations(AppData* app);
static ErrorType initPomodoroData(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * App Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize application variables and data structures.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType InitApp(AppData* app) {
  ErrorType status = NO_ERROR;
  if (!CheckConfigIconType()) return INVALID_CONFIG;

  app->screen = CreateScreen();
  if (app->screen == NULL) return MALLOC_ERROR;

  status = initStatusBar(app);
  if (status != NO_ERROR) return status;

  app->current_menu = -1;
  status = initMenus(app);
  if (status != NO_ERROR) return status;

  status = initAnimations(app);
  if (status != NO_ERROR) return status;

  ExecuteHistory(app->screen->panels[0].scene_history, MAIN_MENU);
  ExecuteHistory(app->screen->panels[1].scene_history, NOTES);
  app->screen->panels[0].menu_index = MAIN_MENU_MENU;
  app->screen->panels[1].menu_index = -1;
  app->screen->panels[1].input = InputStateCreate();
  app->screen->panels[1].mode = DEFAULT;
  app->is_paused = false;
  app->popup_dialog = NULL;

  app->notes = CreateNotesData();
  if (app->notes == NULL) return MALLOC_ERROR;

  if (NOTEPAD_LOG) LoadNotes(NOTES_LOG, app->notes);

  /* Add some example notes */
  if (DEBUG && app->notes->count == 0) {
    AddNote(app, app->notes, "Buy groceries", NOTE_UNDONE);
    AddNote(app, app->notes, "Read a book", NOTE_DONE);
    AddNote(app, app->notes, "This is a note", NOTE_PLAIN);
  }

  if (app->notes->count > 0 && app->notes->current_id < 0)
    app->notes->current_id = app->notes->items[0]->id;

  app->user_input = -1;
  app->last_input = -1;
  app->block_input = false;
  app->frozen = false;
  app->mouse_x = -1;
  app->mouse_y = -1;
  app->mouse_bstate = 0;

  InitWhiteNoiseData(&app->noise_data);
  {
    static const WhiteNoiseTrackDef default_tracks[] = {
      {"Rain", (const char**)RAIN_ICONS, "./sounds/ambience-rain.mp3",
       NOISE_MASTER_VOLUME, 14},
      {"Fire", (const char**)FIRE_ICONS, "./sounds/ambience-fire.mp3",
       NOISE_MASTER_VOLUME, 13},
      {"Wind", (const char**)WIND_ICONS, "./sounds/ambience-wind.mp3",
       NOISE_MASTER_VOLUME, 15},
      {"Thunder", (const char**)THUNDER_ICONS, "./sounds/ambience-thunder.mp3",
       NOISE_MASTER_VOLUME, 11},
    };
    status = RegisterWhiteNoiseTracks(
      &app->noise_data, default_tracks,
      sizeof(default_tracks) / sizeof(default_tracks[0]));
    if (status != NO_ERROR) return status;
  }

  status = initPomodoroData(app);
  if (status != NO_ERROR) return status;
  app->running = true;
  return status;
}

/**
 * End application and free all allocated resources.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType EndApp(AppData* app) {
  for (int i = 0; i < MAX_ANIMATIONS; i++) {
    FreeRollfilm(app->animations[i]);
    app->animations[i] = NULL;
  }
  for (int i = 0; i < MAX_MENUS; i++) {
    FreeMenu(app->menus[i]);
    app->menus[i] = NULL;
  }
  ShutdownNoiseAudio();
  FreeFloatingDialog(app->popup_dialog);
  FreeStatusBar(app->status_bar);
  if (NOTEPAD_LOG)
    if (SaveNotes(NOTES_LOG, app->notes) != NO_ERROR)
      LogError("Saving notes on exit", END_APP_ERROR);
  if (WORK_LOG && app->pomodoro_data.current_step != MAIN_MENU)
    if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
      LogError("Saving pomodoro data on exit", END_APP_ERROR);
  FreeNotesData(app->notes);
  FreeScreen(app->screen);
  return NO_ERROR;
}

/**
 * Initialize ncurses screen and configure settings.
 * Sets up terminal for curses mode with required features.
 * @return ErrorType NO_ERROR on success, or INIT_ERROR on failure
 */
ErrorType InitScreen(void) {
#ifdef XCURSES
  Xinitscr(argc, argv);
#else
  if (initscr() == NULL) return WINDOW_CREATION_ERROR;
#endif
  if (has_colors()) {
    if (BG_TRANSPARENCY == 1) use_default_colors();
    if (start_color() == ERR) return WINDOW_CREATION_ERROR;

    /* Initialize pairs with both foreground and background colors */
    for (int bg = 0; bg < PALETTE_SIZE; bg++) {
      for (int fg = 0; fg < PALETTE_SIZE; fg++) {
        int pair_number = (bg * PALETTE_SIZE) + fg + 1;
        if (init_pair(pair_number, fg, bg) == ERR) return WINDOW_CREATION_ERROR;
      }
    }

    /* Initialize pairs with foreground colors and transparent background */
    for (int fg = 0; fg < PALETTE_SIZE; fg++) {
      int pair_number = (fg + 1) + (PALETTE_SIZE * PALETTE_SIZE);
      if (init_pair(pair_number, fg, -1) == ERR) return WINDOW_CREATION_ERROR;
    }
  }
  /* Disable echoing user input */
  noecho();
  /* Enable reporting mouse movement events */
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  /* Enable xterm mouse tracking so terminal sends motion events */
  printf("\033[?1003h\n");
  /* Set mouse input to be immediately available */
  mouseinterval(0);
  /* Enable raw mode to receive input character-by-character */
  raw();
  /* Hide cursor */
  curs_set(0);
  /* Set getch() to non-blocking mode */
  nodelay(stdscr, TRUE);
  /* Enable keypad mode for extended keyboard input */
  keypad(stdscr, TRUE);
  /* Discard any stale terminal events from before ncurses init */
  flushinp();
  return NO_ERROR;
}

/**
 * End ncurses screen and clean up default window.
 * Must be called before program exit.
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType EndScreen(void) {
  /* Disable xterm mouse tracking */
  printf("\033[?1003l\033[?1000l\033[?1006l");
  int err = endwin();
  if (err == ERR) return WINDOW_DELETION_ERROR;

  err = delwin(stdscr);
  if (err == ERR) return WINDOW_DELETION_ERROR;

  extern SCREEN* SP;
  delscreen(SP);

  return NO_ERROR;
}

/**
 * ---------------------------------------------------------------------------
 * Components
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize a Border struct with configured character values.
 * @return Border struct with default border characters
 */
Border InitBorder(void) {
  Border border;
  border.top_left = BORDER_CHARS[0];     /* "┏" */
  border.top_right = BORDER_CHARS[1];    /* "┓" */
  border.bottom_left = BORDER_CHARS[2];  /* "┗" */
  border.bottom_right = BORDER_CHARS[3]; /* "┛" */
  border.horizontal = BORDER_CHARS[4];   /* "━" */
  border.vertical = BORDER_CHARS[5];     /* "┃" */
  return border;
}

/**
 * Initialize the application menus with menu items and actions.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static ErrorType initMenus(AppData* app) {
  /* MAIN_MENU */
  const int n_mainmenu = 5;
  MenuItem main_menu_items[5] = {{"start", StartPomodoro},
                                 {"preferences", NULL},
                                 {"help menu", NULL},
                                 {"history", OpenHistoryPopup},
                                 {"leave", ForcefullyQuitApp}};

  Menu* main_menu_menu =
    CreateMenu(main_menu_items, 5, COLOR_WHITE, COLOR_WHITE, "-> ", " <-");
  if (main_menu_menu == NULL) return MALLOC_ERROR;
  app->menus[MAIN_MENU_MENU] = main_menu_menu;
  app->current_menu = MAIN_MENU_MENU;

  /* PREFERENCES */
  const int n_preferences = 4;
  MenuItem preferences_items[4] = {{"pomodoros", NULL},
                                   {"work time", NULL},
                                   {"short pause", NULL},
                                   {"long pause", NULL}};

  Menu* preferences_menu = CreateMenu(preferences_items, n_preferences,
                                      COLOR_WHITE, COLOR_WHITE, "-> ", " <-");
  if (preferences_menu == NULL) return MALLOC_ERROR;
  app->menus[PREFERENCES_MENU] = preferences_menu;

  return NO_ERROR;
}

/**
 * Initialize the status bar with modules for different information.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static ErrorType initStatusBar(AppData* app) {
  StatusBarPosition position = BOTTOM;
  if (STATUS_BAR_POSITION) position = TOP;

  app->status_bar = CreateStatusBar(position);
  if (app->status_bar == NULL) return MALLOC_ERROR;

  AddStatusBarModule(app->status_bar, LEFT, InputModeModule);
  AddStatusBarModule(app->status_bar, LEFT, RealTimeModule);
  AddStatusBarModule(app->status_bar, RIGHT, SceneModule);
  AddStatusBarModule(app->status_bar, RIGHT, LineColumnModule);

  app->status_bar->right_modules =
    InvertModulesOrder(app->status_bar->right_modules);

  return NO_ERROR;
}

/**
 * Initialize animations by loading sprites from files.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static ErrorType initAnimations(AppData* app) {
  const char* animation_files[MAX_ANIMATIONS] = {
    "./sprites/mainmenu.asc",   "./sprites/worktime.asc",
    "./sprites/shortpause.asc", "./sprites/longpause.asc",
    "./sprites/notes.asc",      "./sprites/help.asc",
    "./sprites/continue.asc"};

  for (int i = 0; i < MAX_ANIMATIONS; ++i) {
    app->animations[i] = DeserializeSprites(animation_files[i]);
    if (app->animations[i] == NULL) return ANIMATION_DESERIALIZATION_ERROR;
  }
  app->screen->min_panel_size = GetWidestAndTallestAnimation(app);

  const int dont_loop[] = {NOTES, HELP, CONTINUE};
  size_t list_size = sizeof(dont_loop) / sizeof(dont_loop[0]);
  SetRollfilmLoop(app, app->animations, dont_loop, list_size, false);

  return NO_ERROR;
}

/**
 * Initialize pomodoro timer data with configured values.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
static ErrorType initPomodoroData(AppData* app) {
  bool session_loaded = false;
  bool log_had_data = false;

  if (WORK_LOG) {
    FILE* file = fopen(POMODORO_LOG, "rb");
    if (file) {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      fclose(file);
      if (size > 0) {
        log_had_data = true;
        if (LoadPomodoro(POMODORO_LOG, &app->pomodoro_data) == NO_ERROR) {
          if (app->pomodoro_data.status == 0) {
            app->pomodoro_data.current_step = MAIN_MENU;
            app->pomodoro_data.current_cycle = 0;
            app->pomodoro_data.current_step_time = 0;
            app->pomodoro_data.total_elapsed = 0;
            app->pomodoro_data.session_index = 0;
          } else if (app->pomodoro_data.status == 1) {
            if (app->pomodoro_data.current_step == WORK_TIME ||
                app->pomodoro_data.current_step == SHORT_PAUSE ||
                app->pomodoro_data.current_step == LONG_PAUSE) {
              app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
              session_loaded = true;
              app->pomodoro_data.last_step_time = -1;
              if (app->pomodoro_data.session_index == 0)
                app->pomodoro_data.session_index = 1;
              app->popup_dialog = CreateContinueDialog(app);
            }
          }
        }
      }
    }
  }

  /* Initialise history popup state */
  memset(&app->history_data, 0, sizeof(app->history_data));

  /* In DEBUG mode, seed the log with fake completed sessions */
  if (DEBUG) { ErrorType err = debugSeedHistory(); (void)err; }

  /* Discard any stale input before showing popups */
  flushinp();

  if (!session_loaded) {
    if (DEBUG)
      app->pomodoro_data.total_cycles = 2;
    else
      app->pomodoro_data.total_cycles = POMODOROS_AMOUNT;
    app->pomodoro_data.work_time = WORKTIME_TIME;
    app->pomodoro_data.short_pause_time = SHORT_PAUSE_TIME;
    app->pomodoro_data.long_pause_time = LONG_PAUSE_TIME;
    app->pomodoro_data.current_cycle = 0;
    app->pomodoro_data.current_step = MAIN_MENU;
    app->pomodoro_data.last_step_time = -1;
    app->pomodoro_data.current_step_time = 0;
    app->pomodoro_data.total_elapsed = 0;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    app->pomodoro_data.session_index = 0;

    if (!log_had_data) app->popup_dialog = CreateWelcomeDialog(app);
  }

  return NO_ERROR;
}

/**
 * Seed the pomodoro log with fake completed sessions for testing.
 * Creates a variety of daily session counts to exercise all 4 history levels
 * (░░ 0, ▒▒ 1-2, ▓▓ 3-5, ██ 6+) and the streak counter.
 * @return ErrorType NO_ERROR on success, or FILE_ERROR on failure
 */
static ErrorType debugSeedHistory(void) {
  FILE* f = fopen(POMODORO_LOG, "wb");
  if (!f) return FILE_ERROR;

  /* Days ago (positive) → session count */
  static const int seed[] = {
    0,  2, /* today:      ▒▒ */
    1,  1, /* yesterday:  ▒▒ */
    2,  5, /*             ▓▓ */
    3,  1, /*             ▒▒ */
    4,  8, /*             ██ */
    5,  3, /*             ▓▓ */
    6,  7, /*             ██ */
    30, 4, /*             ▓▓ (isolated, breaks streak) */
  };
  int n = sizeof(seed) / sizeof(seed[0]) / 2;

  time_t now = time(NULL);
  struct tm* tm = localtime(&now);
  int curYear = tm->tm_year + 1900;
  int curMonth = tm->tm_mon + 1;
  int curDay = tm->tm_mday;

  uint32_t sessionIdx = 1;

  for (int i = 0; i < n; i++) {
    int daysAgo = seed[i * 2];
    int count = seed[i * 2 + 1];

    /* Compute the date for this batch */
    struct tm dayTm = {0};
    dayTm.tm_year = curYear - 1900;
    dayTm.tm_mon = curMonth - 1;
    dayTm.tm_mday = curDay - daysAgo;
    mktime(&dayTm);

    uint32_t startOfDay = (uint32_t)mktime(&dayTm);

    for (int s = 0; s < count; s++) {
      pomodoroLogRecord rec;
      rec.session_index = (uint16_t)sessionIdx++;
      rec.current_step = 1; /* WORK_TIME */
      rec.current_cycle = (uint8_t)(s % 4);
      rec.total_cycles = 4;
      rec.work_time = 25;
      rec.short_pause_time = 5;
      rec.long_pause_time = 15;
      rec.total_elapsed = (uint32_t)(s * 1800);
      rec.current_step_time = 1500;
      rec.status = 0; /* completed */
      rec.session_start_time = startOfDay + (uint32_t)(s * 1800);
      fwrite(&rec, sizeof(rec), 1, f);
    }
  }

  fclose(f);
  return NO_ERROR;
}
