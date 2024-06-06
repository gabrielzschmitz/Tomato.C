#include "draw.h"

#include <ncurses.h>

#include "anim.h"
#include "tomato.h"
#include "util.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  switch (app->current_mode) {
    case MAIN_MENU:
      DrawCurrentFrame(app->animations[MAIN_MENU], 0, 0);
      SetColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
      mvprintw(9, 3, "%02d - %02lf", app->frame_seconds,
               app->frame_milliseconds);
      break;
    default:
      break;
  }
  refresh();

  return status;
}
