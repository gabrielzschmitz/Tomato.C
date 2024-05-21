#include "draw.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "anim.h"
#include "tomato.h"
#include "util.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  switch (app->current_mode) {
    case MAIN_MENU:
      for (int i = 0; i < app->sprites[0].current_frame; i++) {
        for (int j = 0; j < app->sprites[0].frame_height; j++) {
          int length = strlen(
            app->sprites[0].frames[(i * app->sprites[0].frame_height) + j]);
          char** output_strings;
          int num_strings;

          BreakStringByColorMap(
            app->sprites[0].frames[(i * app->sprites[0].frame_height) + j],
            app->sprites[0].color_map[(i * app->sprites[0].frame_height) + j],
            length, &output_strings, &num_strings);

          int x_position = 0;
          for (int k = 0; k < num_strings; k++) {
            mvprintw(1 + j, x_position, "%s", output_strings[k]);
            x_position += strlen(output_strings[k]);
            if (app->sprites[0].color_map[(i * app->sprites[0].frame_height) +
                                          j][x_position - 1] > 0 &&
                app->sprites[0].color_map[(i * app->sprites[0].frame_height) +
                                          j][x_position - 1] <
                  MAX_COLOR_PAIRS) {
              SetColor(
                app->sprites[0].color_map[(i * app->sprites[0].frame_height) +
                                          j][x_position - 1],
                COLOR_BLACK, A_BOLD);
            }
          }
          free(output_strings);
        }
      }
      mvprintw(0, 0, "%02d", app->frame_seconds);
      break;
    default:
      break;
  }
  refresh();

  return status;
}
