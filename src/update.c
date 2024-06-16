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
  /* Tomato Animation */
  if (ANIMATIONS) {
    Rollfilm *animation = app->animations[MAIN_MENU];
    animation->update(animation);
  }
}
