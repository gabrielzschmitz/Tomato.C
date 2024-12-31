#include "init.h"

#include <ncurses.h>

#include "anim.h"
#include "bar.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* Initialize ncurses screen and configure settings */
void InitScreen(void) {
#ifdef XCURSES
  Xinitscr(argc, argv);
#else
  initscr();
#endif
  if (has_colors()) {
    if (BG_TRANSPARENCY == 1) use_default_colors();
    start_color();

    // Initialize pairs with both foreground and background colors
    for (int bg = 0; bg < PALETTE_SIZE; bg++) {
      for (int fg = 0; fg < PALETTE_SIZE; fg++) {
        int pair_number = (bg * PALETTE_SIZE) + fg + 1;
        init_pair(pair_number, fg, bg);
      }
    }

    // Initialize pairs with foreground colors and transparent background
    for (int fg = 0; fg < PALETTE_SIZE; fg++) {
      int pair_number = (fg + 1) + (PALETTE_SIZE * PALETTE_SIZE);
      init_pair(pair_number, fg, -1);
    }
  }
  /* Disable echoing user input */
  noecho();
  /* Enable reporting mouse movement events */
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
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
}

/* Initialize variables */
ErrorType InitApp(AppData* app) {
  ErrorType status = NO_ERROR;
  if (!CheckConfigIconType()) return INVALID_CONFIG;

  app->screen = CreateScreen();
  if (app->screen == NULL) return MALLOC_ERROR;

  status = InitStatusBar(app);
  if (status != NO_ERROR) return status;

  status = InitMenus(app);
  if (status != NO_ERROR) return status;

  app->user_input = -1;
  app->last_input = -1;
  app->block_input = false;

  ExecuteHistory(app->screen->panels[0].scene_history, MAIN_MENU);
  ExecuteHistory(app->screen->panels[1].scene_history, NOTES);
  app->is_paused = false;
  app->running = true;

  status = InitAnimations(app);
  if (status != NO_ERROR) return status;

  return status;
}

/* Function to initialize the app menus */
ErrorType InitMenus(AppData* app) {
  /* MAIN_MENU */
  const char* main_menu_items[] = {"start", "preferences", "help menu",
                                   "leave"};
  size_t main_menu_items_size =
    sizeof(main_menu_items) / sizeof(main_menu_items[0]);
  Menu* main_menu_menu = CreateMenu(main_menu_items, main_menu_items_size,
                                    COLOR_WHITE, COLOR_WHITE, "-> ", " <-");
  if (main_menu_menu == NULL) return MALLOC_ERROR;
  app->menus[0] = main_menu_menu;

  /* PREFERENCES */
  const char* preferences_items[] = {"pomodoros", "work time", "short pause",
                                     "long pause"};
  size_t preferences_items_size =
    sizeof(preferences_items) / sizeof(preferences_items[0]);
  Menu* preferences_menu = CreateMenu(preferences_items, preferences_items_size,
                                      COLOR_WHITE, COLOR_WHITE, "-> ", " <-");
  if (preferences_menu == NULL) return MALLOC_ERROR;
  app->menus[1] = preferences_menu;

  return NO_ERROR;
}

/* Function to initialize the status bar */
ErrorType InitStatusBar(AppData* app) {
  StatusBarPosition position = BOTTOM;
  if (STATUS_BAR_POSITION) position = TOP;

  app->status_bar = CreateStatusBar(position);
  if (app->status_bar == NULL) return MALLOC_ERROR;

  AddStatusBarModule(app->status_bar, LEFT, InputModeModule);
  AddStatusBarModule(app->status_bar, LEFT, RealTimeModule);
  AddStatusBarModule(app->status_bar, RIGHT, SceneModule);

  app->status_bar->right_modules =
    InvertModulesOrder(app->status_bar->right_modules);

  return NO_ERROR;
}

/* Initialize animations from sprites */
ErrorType InitAnimations(AppData* app) {
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
  SetAnimationsLoop(app->animations, dont_loop, list_size, false);

  return NO_ERROR;
}

/* End ncurses screen and delete default window and screen */
ErrorType EndScreen(void) {
  int err = endwin();
  if (err == ERR) return WINDOW_DELETION_ERROR;

  err = delwin(stdscr);
  if (err == ERR) return WINDOW_DELETION_ERROR;

  extern SCREEN* SP;
  delscreen(SP);

  return NO_ERROR;
}

/* End/Free variables */
ErrorType EndApp(AppData* app) {
  for (int i = 0; i < MAX_ANIMATIONS; i++) {
    FreeRollfilm(app->animations[i]);
    app->animations[i] = NULL;
  }
  for (int i = 0; i < MAX_MENUS; i++) {
    FreeMenu(app->menus[i]);
    app->menus[i] = NULL;
  }
  FreeStatusBar(app->status_bar);
  FreeScreen(app->screen);
  return NO_ERROR;
}

/* Init a Border struct with the config values */
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
