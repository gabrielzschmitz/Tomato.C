#include "update.h"

#include <ncurses.h>

#include "anim.h"
#include "config.h"
#include "tomato.h"

/* Update variables */
ErrorType UpdateApp(AppData *app) {
  ErrorType status = NO_ERROR;

  switch (app->current_mode) {
    case MAIN_MENU:
      UpdateMainMenu(app);
      break;
    default:
      break;
  }

  /* Update all the app modes */
  // if (app->currentMode == -3)
  //   updateHelpPage(app);
  // else if (app->currentMode == -2)
  //   updateNotepad(app);
  // if (app->current_mode == MAIN_MENU) UpdateMainMenu(app);
  // else if (app->currentMode == 1)
  //   updateWorkTime(app);
  // else if (app->currentMode == 2)
  //   updateShortPause(app);
  // else if (app->currentMode == 3)
  //   updateLongPause(app);

  /* Get X and Y window size */
  GetScreenSize(app);

  return status;
}

/* Get the screen size */
void GetScreenSize(AppData *app) {
  getmaxyx(stdscr, app->screen_height, app->screen_width);
}

/* Update MAIN_MENU */
void UpdateMainMenu(AppData *app) {
  FrameTimer(&app->frame_seconds, &app->frame_milliseconds);

  if (ANIMATIONS) {
    /* Tomato Animation */
    Rollfilm *animation = app->animations[MAIN_MENU];
    Frame *frames = animation->frames;
    if (frames != NULL && app->frame_seconds != frames->id) {
      animation->frames = frames->next;
      animation->current_frame = frames->id;
    }
    if (app->frame_seconds == animation->frame_count) app->frame_seconds = 0;
  }
}
