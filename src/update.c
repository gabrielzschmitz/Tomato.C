#include "update.h"

#include <ncurses.h>

#include "anim.h"
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
  FrameTimer(&app->milliseconds, &app->frame_seconds);

  /* Tomato Animation */
  Sprite *sprites = &app->sprites[MAIN_MENU];
  int frame_count = sprites->frame_count;
  int final_frame = frame_count * frame_count;
  app->sprites[0].current_frame =
    (app->frame_seconds / app->sprites[0].frame_count);
  if (app->sprites[0].current_frame > app->sprites[0].frame_count) {
    app->sprites[0].current_frame = 0;
    app->frame_seconds = app->sprites[0].frame_count - 1;
  }
}
