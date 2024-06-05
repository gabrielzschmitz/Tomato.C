#include "init.h"

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
    int realbg;
    int bg = 0, fg = 0;
    start_color();
    for (bg = COLOR_BLACK; bg <= COLOR_WHITE; bg++)
      for (fg = COLOR_BLACK; fg <= COLOR_WHITE; fg++) {
        if (BGTRANSPARENCY == 1)
          realbg = -1;
        else
          realbg = bg;
        init_pair(bg * PALLETE_SIZE + fg + 1, fg, realbg);
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

  app->milliseconds = 0;
  app->frame_seconds = 0;

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
