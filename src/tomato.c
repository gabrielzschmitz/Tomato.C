#include "tomato.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>

#include "draw.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "update.h"

int main(void) {
  /* Enable emojis */
  setlocale(LC_CTYPE, "");

  /* Init everything */
  AppData app;
  InitScreen();
  ErrorType init_app = InitApp(&app);
  if (init_app != NO_ERROR) {
    endwin();
    LogError("Initializing app variables", init_app);
    return init_app;
  }

  /* Main app loop */
  while (app.running) {
    ErrorType update_app = UpdateApp(&app);
    if (update_app != NO_ERROR) {
      endwin();
      LogError("Updating app", update_app);
      return update_app;
    }

    ErrorType handling_input = HandleInputs(&app);
    if (handling_input != NO_ERROR) {
      endwin();
      LogError("Handling input", handling_input);
      return handling_input;
    }

    ErrorType draw_screen = DrawScreen(&app);
    if (draw_screen != NO_ERROR) {
      endwin();
      LogError("Drawing screen", draw_screen);
      return draw_screen;
    }

    napms(FPMS);
  }
  ErrorType end_screen = EndScreen();
  if (end_screen != NO_ERROR) {
    LogError("Ending app screen", end_screen);
    return end_screen;
  }
  EndApp(&app);

  printf("Goodbye!\n");

  return 0;
}
