#include "init.h"

#include <ncurses.h>

#include "anim.h"
#include "tomato.h"

/* Initialize ncurses screen and configure settings */
void InitScreen(void) {
#ifdef XCURSES
  Xinitscr(argc, argv);
#else
  initscr();
#endif
  if (has_colors()) {
    if (BGTRANSPARENCY == 1) use_default_colors();
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
  app->screen_width = 0;
  app->screen_height = 0;

  app->user_input = -1;
  app->current_mode = MAIN_MENU;

  app->running = true;

  const char* main_menu_sprites_file = "../sprites/mainmenu.asc";
  app->animations[MAIN_MENU] = DeserializeSprites(main_menu_sprites_file);
  if (app->animations[MAIN_MENU] == NULL) return MALLOC_ERROR;

  app->frame_seconds = 0;
  app->frame_milliseconds = 0.0;

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
