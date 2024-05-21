#include "input.h"

#include <ncurses.h>

#include "tomato.h"

/* Handle user input and app state */
ErrorType HandleInputs(AppData *app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;
  app->user_input = getch();

  // if (app->input_mode == NORMAL)
  //   status = HandleNormalMode(app);
  // else if (app->input_mode == INSERT)
  //   status = HandleInsertMode(app);
  // else if (app->input_mode == COMMAND)
  //   status = HandleCommandMode(app);

  if (app->user_input == 'q') app->running = false;

  flushinp();
  return status;
}
