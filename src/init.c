#include "init.h"

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#include "anim.h"
#include "tomato.h"

/* Initialize ncurses screen and configure settings */
ErrorType InitScreen(void) {
  ErrorType status = NO_ERROR;
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

  return status;
}

/* Initialize variables */
ErrorType InitApp(AppData* app) {
  ErrorType status = NO_ERROR;

  app->screen_width = 0;
  app->screen_height = 0;

  app->user_input = -1;
  app->current_mode = MAIN_MENU;

  app->running = true;

  const char* main_menu_sprites_file = "../sprites/mainmenu.asc";
  DeserializeSprite(main_menu_sprites_file, &app->sprites[0]);
  app->sprites[0].current_frame = 0;

  app->milliseconds = 0;
  app->frame_seconds = app->sprites[0].frame_count - 1;

  return status;
}
