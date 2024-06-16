#include "tomato.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>

#include "anim.h"
#include "config.h"
#include "draw.h"
#include "init.h"
#include "input.h"
#include "update.h"

int main(int argc, char *argv[]) {
  /* Enable emojis */
  setlocale(LC_CTYPE, "");

  /* Init everything */
  AppData app;
  InitScreen();
  ErrorType init_app = InitApp(&app);
  if (init_app != NO_ERROR) {
    endwin();
    fprintf(stderr, "Error initializing app variables: %d\n", init_app);
    return init_app;
  }

  /* Main app loop */
  while (app.running) {
    ErrorType update_app = UpdateApp(&app);
    if (update_app != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error updating app: %d\n", update_app);
      return update_app;
    }

    ErrorType handling_input = HandleInputs(&app);
    if (handling_input != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error handling input: %d\n", handling_input);
      return handling_input;
    }

    ErrorType draw_screen = DrawScreen(&app);
    if (draw_screen != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error drawing screen: %d\n", draw_screen);
      return draw_screen;
    }

    napms(FPMS);
  }
  ErrorType end_screen = EndScreen();
  if (end_screen != NO_ERROR) {
    fprintf(stderr, "Error ending app screen: %d\n", end_screen);
    return end_screen;
  }
  EndApp(&app);

  printf("Goodbye!\n");

  return 0;
}
