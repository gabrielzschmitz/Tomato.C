#include "draw.h"

#include <ncurses.h>

#include "anim.h"
#include "tomato.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  switch (app->current_mode) {
    case MAIN_MENU:
      mvprintw(0, 0, "%02d", app->frame_seconds);
      DrawCurrentFrame(app->animations[MAIN_MENU], 1, 2);
      break;
    default:
      break;
  }
  refresh();

  return status;
}
