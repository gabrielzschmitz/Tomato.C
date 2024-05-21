#include "tomato.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
  // ErrorCode init_windows_result = InitWindows(&app);
  // if (init_windows_result != NO_ERROR) {
  //   endwin();
  //   fprintf(stderr, "Error initializing windows: %d\n", init_windows_result);
  //   return init_windows_result;
  // }

  /* Main app loop */
  while (app.running) {
    ErrorType update_app = UpdateApp(&app);
    if (update_app != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error updating app: %d\n", update_app);
      return update_app;
    }

    ErrorType draw_screen = DrawScreen(&app);
    if (draw_screen != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error drawing screen: %d\n", draw_screen);
      return draw_screen;
    }

    ErrorType handling_input = HandleInputs(&app);
    if (handling_input != NO_ERROR) {
      endwin();
      fprintf(stderr, "Error handling input: %d\n", handling_input);
      return handling_input;
    }

    /* Setting the screen refresh rate in ms */
    napms(1000 / FPS);
  }
  for (int i = 0; i < app.sprites[0].frame_count; i++) {
    free(app.sprites[0].frames[i]);
  }

  endwin();
  printf("Goodbye!\n");

  return 0;
}
