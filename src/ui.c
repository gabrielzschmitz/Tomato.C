#include "ui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tomato.h"
#include "util.h"

/* Create a screen struct with MAX_PANELS in horizontal rows */
Screen *CreateScreen() {
  Screen *screen = (Screen *)malloc(sizeof(Screen));
  if (screen == NULL) return NULL;

  getmaxyx(stdscr, screen->size.height, screen->size.width);
  screen->current_panel = 0;
  int panels_width = screen->size.width / MAX_PANELS;
  int panels_height = screen->size.height;
  int remainder_width = screen->size.width % MAX_PANELS;
  for (int i = 0; i < MAX_PANELS; i++) {
    Dimensions panel_dimensions;
    panel_dimensions.width = panels_width + (i < remainder_width ? 1 : 0);
    panel_dimensions.height = panels_height;

    Vector2D panel_position;
    panel_position.x =
      (panels_width * i) + (i < remainder_width ? i : remainder_width);
    panel_position.y = 0;

    screen->panels[i] = CreatePanel(panel_dimensions, panel_position);
  }

  return screen;
}

/* Free all memory associated with a Screen struct */
void FreeScreen(Screen *screen) {
  if (screen == NULL) return;
  free(screen);
}

/* Create a screen struct with the current screen size and MAX_PANELS */
Panel CreatePanel(Dimensions size, Vector2D position) {
  Panel panel;

  panel.mode = NORMAL;
  panel.size = size;
  panel.visible = true;
  panel.position = position;
  panel.scene_history = NULL;

  return panel;
}

/* Render a border at a given panel */
void RenderPanelBorder(Panel panel, Border border) {
  if (!panel.visible) return;
  int x, y;

  mvprintw(panel.position.y, panel.position.x, "%s", border.top_left);
  for (x = panel.position.x + 1; x < panel.position.x + panel.size.width - 1;
       x++) {
    mvprintw(panel.position.y, x, "%s", border.horizontal);
  }
  mvprintw(panel.position.y, panel.position.x + panel.size.width - 1, "%s",
           border.top_right);

  mvprintw(panel.position.y + panel.size.height - 1, panel.position.x, "%s",
           border.bottom_left);
  for (x = panel.position.x + 1; x < panel.position.x + panel.size.width - 1;
       x++) {
    mvprintw(panel.position.y + panel.size.height - 1, x, "%s",
             border.horizontal);
  }
  mvprintw(panel.position.y + panel.size.height - 1,
           panel.position.x + panel.size.width - 1, "%s", border.bottom_right);

  for (y = panel.position.y + 1; y < panel.position.y + panel.size.height - 1;
       y++) {
    mvprintw(y, panel.position.x, "%s", border.vertical);
    mvprintw(y, panel.position.x + panel.size.width - 1, "%s", border.vertical);
  }
}

/* Update panels from a given screen */
void UpdateScreen(Screen *screen) {
  getmaxyx(stdscr, screen->size.height, screen->size.width);

  int panels_width = screen->size.width / MAX_PANELS;
  int panels_height = screen->size.height;
  int remainder_width = screen->size.width % MAX_PANELS;

  // Check if the screen can display all panels
  if (screen->size.width >= screen->min_panel_size.width * MAX_PANELS) {
    // Display all panels
    for (int i = 0; i < MAX_PANELS; i++) {
      Dimensions panel_dimensions;
      panel_dimensions.width = panels_width + (i < remainder_width ? 1 : 0);
      panel_dimensions.height = panels_height;

      Vector2D panel_position;
      panel_position.x =
        (panels_width * i) + (i < remainder_width ? i : remainder_width);
      panel_position.y = 0;

      screen->panels[i].visible = true;
      UpdatePanel(&screen->panels[i], panel_dimensions, panel_position);
    }
  } else {
    for (int i = 0; i < MAX_PANELS; i++) screen->panels[i].visible = false;

    // Display only the current panel
    int current_panel = screen->current_panel;
    if (current_panel >= 0 && current_panel < MAX_PANELS) {
      Dimensions panel_dimensions;
      panel_dimensions.width = screen->size.width;
      panel_dimensions.height = panels_height;

      Vector2D panel_position;
      panel_position.x = 0;
      panel_position.y = 0;

      // Update the current panel and make it visible
      screen->panels[current_panel].visible = true;
      UpdatePanel(&screen->panels[current_panel], panel_dimensions,
                  panel_position);
    }
  }
}

/* Update panel dimensions and position */
void UpdatePanel(Panel *panel, Dimensions size, Vector2D position) {
  panel->size = size;
  panel->position = position;
}

/* Render content at panel center */
void RenderAtPanelCenter(Panel panel, const char *content, Vector2D offset) {
  int panel_center_x = panel.position.x + panel.size.width / 2;
  int panel_center_y = panel.position.y + panel.size.height / 2;

  int content_x = panel_center_x - strlen(content) / 2 + offset.x;
  int content_y = panel_center_y + offset.y;

  mvprintw(content_y, content_x, "%s", content);
}

/* Render animation at panel center */
void RenderAnimationAtPanelCenter(Panel panel, Rollfilm animation,
                                  Vector2D offset) {
  int panel_center_x = panel.position.x + panel.size.width / 2;
  int panel_center_y = panel.position.y + panel.size.height / 2;

  int frame_x = panel_center_x - animation.frame_width / 2 + offset.x;
  int frame_y = panel_center_y - animation.frame_height / 2 + offset.y;

  if (animation.render != NULL) {
    animation.render(&animation, frame_y, frame_x);
  }
}

/* Render screen size error */
void RenderScreenSizeError(Screen *screen, Panel panel) {
  char *content;
  int required_length;

  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  RenderAtPanelCenter(panel, "TERMINAL SIZE TOO SMALL!", (Vector2D){0, -2});

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  required_length = snprintf(NULL, 0, "Width = %2d Height = %2d",
                             screen->size.width, screen->size.height) +
                    1;
  content = (char *)malloc(required_length);
  snprintf(content, required_length, "Width = %2d Height = %2d",
           screen->size.width, screen->size.height);
  RenderAtPanelCenter(panel, content, (Vector2D){0, -1});
  free(content);

  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  RenderAtPanelCenter(panel, "SIZE NEEDED IN CURRENT CONFIG", (Vector2D){0, 0});

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  required_length =
    snprintf(NULL, 0, "Width = %2d Height = %2d", screen->min_panel_size.width,
             screen->min_panel_size.height) +
    1;
  content = (char *)malloc(required_length);
  snprintf(content, required_length, "Width = %2d Height = %2d",
           screen->min_panel_size.width, screen->min_panel_size.height);
  RenderAtPanelCenter(panel, content, (Vector2D){0, 1});
  free(content);
}
