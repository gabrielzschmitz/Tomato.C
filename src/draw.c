#include "draw.h"

#include <ncurses.h>

#include "anim.h"
#include "config.h"
#include "tomato.h"
#include "util.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  Rollfilm* animation = NULL;
  switch (app->current_mode) {
    case MAIN_MENU:
      if (ANIMATIONS) {
        animation = app->animations[MAIN_MENU];
        animation->render(animation, 0, 0);
        SetColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(9, 0, "%dC - %dH - %1.3lfS - %dID", animation->frame_count,
                 animation->frame_height, animation->frames->seconds_multiplier,
                 animation->frames->id);
        mvprintw(10, 4, "%lf", animation->delta_frame_ms);
      }
      break;
    default:
      break;
  }
  refresh();

  return status;
}
