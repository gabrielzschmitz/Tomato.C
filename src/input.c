#include "input.h"

#include <ncurses.h>

#include "tomato.h"

/* Handle user input and app state */
ErrorType HandleInputs(AppData *app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  // if (app->input_mode == NORMAL)
  //   status = HandleNormalMode(app);
  // else if (app->input_mode == INSERT)
  //   status = HandleInsertMode(app);
  // else if (app->input_mode == COMMAND)
  //   status = HandleCommandMode(app);

  app->user_input = getch();
  if (app->user_input == 'q') {
    app->running = false;
    return status;
  }

  if (!app->block_input) {
    switch (app->user_input) {
      case ' ':
        app->screen->current_panel =
          (app->screen->current_panel + 1) % MAX_PANELS;
        break;
      case 'n':
        if (DEBUG) ChangeDebugAnimation(app, 1);
        break;
      case 'N':
        if (DEBUG) ChangeDebugAnimation(app, -1);
        break;
      case 'p':
        app->is_paused = !app->is_paused;
        break;
      case 'm':
        app->screen->panels[app->screen->current_panel].mode =
          (app->screen->panels[app->screen->current_panel].mode + 1) % 3;
        break;
      default:
        break;
    }
  }
  flushinp();

  return status;
}

/* Update animation mode */
void ChangeDebugAnimation(AppData *app, int step) {
  app->current_scene =
    (app->current_scene + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}
