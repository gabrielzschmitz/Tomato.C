#include "input.h"

#include <ncurses.h>

#include "tomato.h"

/* Handle user input and app state */
ErrorType HandleInputs(AppData *app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  app->user_input = getch();
  if (app->user_input == 'q' && app->input_mode != INSERT) {
    app->running = false;
    return status;
  }

  if (!app->block_input) {
    if (app->input_mode == NORMAL)
      status = HandleNormalMode(app);
    else if (app->input_mode == INSERT)
      status = HandleInsertMode(app);
    // else if (app->input_mode == VISUAL)
    //   status = HandleVisualMode(app);
  }

  flushinp();

  return status;
}

/* Handle normal mode input */
ErrorType HandleNormalMode(AppData *app) {
  ErrorType status = NO_ERROR;

  switch (app->user_input) {
    case ' ':
      app->screen->current_panel =
        (app->screen->current_panel + 1) % MAX_PANELS;
      break;
    case 'n':
      // if (DEBUG) ChangeDebugAnimation(app, 1);
      break;
    case 'N':
      // if (DEBUG) ChangeDebugAnimation(app, -1);
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

  return status;
}

/* Handle insert mode input */
ErrorType HandleInsertMode(AppData *app) {
  ErrorType status = NO_ERROR;
  return status;
}

/* Update animation mode */
void ChangeDebugAnimation(AppData *app, int step) {
  int *present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}
