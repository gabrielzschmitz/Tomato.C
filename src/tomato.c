#include "tomato.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <time.h>
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
  clock_t last_time = clock();
  float frame_time = 1000.0f / FPS;
  while (app.running) {
    clock_t current_time = clock();
    float delta_time = (float)(current_time - last_time) / CLOCKS_PER_SEC;
    last_time = current_time;
    if (delta_time < frame_time / 1000.0f) {
      int sleep_time =
        (int)(frame_time -
              delta_time *
                1000.0f);  // Convert frame_time and delta_time to milliseconds
      napms(sleep_time);   // Sleep using napms
      current_time = clock();  // Update current time after sleeping
      delta_time = (float)(current_time - last_time) / CLOCKS_PER_SEC;
      last_time = current_time;
    }
    app.delta_time = delta_time;

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
  }
  ErrorType end_screen = EndScreen();
  if (end_screen != NO_ERROR) {
    fprintf(stderr, "Error ending app screen: %d\n", end_screen);
    return end_screen;
  }

  FreeRollfilm(app.animations[MAIN_MENU]);

  printf("Goodbye!\n");

  return 0;
}
