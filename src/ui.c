#include "ui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tomato.h"
#include "util.h"

/* Create a screen struct with MAX_PANELS in horizontal rows */
Screen* CreateScreen(void) {
  Screen* screen = (Screen*)malloc(sizeof(Screen));
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
void FreeScreen(Screen* screen) {
  if (screen == NULL) return;

  for (int i = 0; i < MAX_PANELS; i++)
    FreePanel(&screen->panels[i]);

  free(screen);
}

/* Create a screen struct with the current screen size and MAX_PANELS */
Panel CreatePanel(Dimensions size, Vector2D position) {
  Panel panel;

  panel.mode = NORMAL;
  panel.size = size;
  panel.visible = true;
  panel.position = position;
  panel.scene_history = CreateHistory();

  return panel;
}

/* Function to free the memory of a panel */
void FreePanel(Panel* panel) {
  if (panel == NULL) return;
  FreeHistory(panel->scene_history);
}

/* Render a border in a given panel */
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
void UpdateScreen(Screen* screen) {
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
    for (int i = 0; i < MAX_PANELS; i++)
      screen->panels[i].visible = false;

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
void UpdatePanel(Panel* panel, Dimensions size, Vector2D position) {
  panel->size = size;
  panel->position = position;
}

/* Render content at panel center */
void RenderAtPanelCenter(Panel* panel, const char* content, Vector2D offset) {
  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

  int content_x = panel_center_x - strlen(content) / 2 + offset.x;
  int content_y = panel_center_y + offset.y;

  mvprintw(content_y, content_x, "%s", content);
}

/* Render animation at panel center */
void RenderAnimationAtPanelCenter(Panel* panel, Rollfilm* animation,
                                  Vector2D offset) {
  if (animation->render == NULL) return;

  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

  int frame_x = panel_center_x - animation->frame_width / 2 + offset.x;
  int frame_y = panel_center_y - animation->frame_height / 2 + offset.y;

  animation->render(animation, frame_y, frame_x);
}

/* Render screen size error */
void RenderScreenSizeError(Screen* screen, Panel* panel) {
  char* content;
  int required_length;

  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  RenderAtPanelCenter(panel, "TERMINAL SIZE TOO SMALL!", (Vector2D){0, -2});

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  required_length = snprintf(NULL, 0, "Width = %2d Height = %2d",
                             screen->size.width, screen->size.height) +
                    1;
  content = (char*)malloc(required_length);
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
  content = (char*)malloc(required_length);
  snprintf(content, required_length, "Width = %2d Height = %2d",
           screen->min_panel_size.width, screen->min_panel_size.height);
  RenderAtPanelCenter(panel, content, (Vector2D){0, 1});
  free(content);
}

/* Function to push a scene onto a stack */
void PushHistory(HistoryNode** stack, int scene) {
  HistoryNode* new_node = (HistoryNode*)malloc(sizeof(HistoryNode));
  if (new_node == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  new_node->scene = scene;
  new_node->next = *stack;
  *stack = new_node;
}

/* Function to pop a scene from a stack */
int PopHistory(HistoryNode** stack) {
  if (*stack == NULL) {
    fprintf(stderr, "Stack underflow\n");
    exit(EXIT_FAILURE);
  }
  HistoryNode* temp = *stack;
  int scene = temp->scene;
  *stack = temp->next;
  free(temp);
  return scene;
}

/* Function to clear a stack */
void ClearStack(HistoryNode** stack) {
  while (*stack != NULL)
    PopHistory(stack);
}

/* Function to create and initialize a new history */
History* CreateHistory(void) {
  History* history = (History*)malloc(sizeof(History));
  if (history == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }
  history->future_stack = NULL;
  history->past_stack = NULL;
  history->present = -1;
  return history;
}

/* Function to free the memory of a history */
void FreeHistory(History* history) {
  if (history == NULL) return;

  ClearStack(&history->future_stack);
  ClearStack(&history->past_stack);

  free(history);
}

/* Function to perform undo operation */
void UndoHistory(History* history) {
  if (history->past_stack == NULL) {
    printf("No scenes in past stack to undo.\n");
    return;
  }

  PushHistory(&history->future_stack, history->present);

  history->present = PopHistory(&history->past_stack);
}

/* Function to perform redo operation */
void RedoHistory(History* history) {
  if (history->future_stack == NULL) {
    printf("No scenes in future stack to redo.\n");
    return;
  }

  PushHistory(&history->past_stack, history->present);

  history->present = PopHistory(&history->future_stack);
}

/* Function to perform do operation */
void ExecuteHistory(History* history, int new_scene) {
  if (history->present >= 0)
    PushHistory(&history->past_stack, history->present);

  history->present = new_scene;

  ClearStack(&history->future_stack);
}

/* Print a given menu at the center of the screen + offset */
void PrintMenuAtCenter(Panel* panel, Menu* menu, Vector2D offset,
                       int line_spacing) {
  for (int i = 0; i < menu->item_count; i++) {
    if (i == menu->selected_item) {
      SetColor(menu->focused_color, NO_COLOR, A_BOLD);

      size_t left_len = strlen(menu->select_style_left);
      size_t item_len = strlen(menu->items[i]);
      size_t right_len = strlen(menu->select_style_right);
      size_t full_text_size = left_len + item_len + right_len + 1;

      char full_text[full_text_size];
      snprintf(full_text, sizeof(full_text), "%s%s%s", menu->select_style_left,
               menu->items[i], menu->select_style_right);
      RenderAtPanelCenter(panel, full_text, offset);
    } else {
      SetColor(menu->unfocused_color, NO_COLOR, A_NORMAL);
      RenderAtPanelCenter(panel, menu->items[i], offset);
    }
    offset.y += line_spacing + 1;
  }
}

/* Function to initialize a menu and return a pointer to it */
Menu* CreateMenu(const char* items[], int num_items, int focused_color,
                 int unfocused_color, const char* select_style_left,
                 const char* select_style_right) {
  Menu* menu = malloc(sizeof(struct Menu));
  if (menu == NULL) return NULL;

  menu->items = malloc(num_items * sizeof(char*));
  if (menu->items == NULL) {
    free(menu);
    return NULL;
  }

  for (int i = 0; i < num_items; i++) {
    menu->items[i] = strdup(items[i]);
    if (menu->items[i] == NULL) {
      for (int j = 0; j < i; ++j)
        free((char*)menu->items[j]);
      free(menu->items);
      free(menu);
      return NULL;
    }
  }

  menu->selected_item = 0;
  menu->focused_color = focused_color;
  menu->unfocused_color = unfocused_color;
  menu->select_style_left = select_style_left;
  menu->select_style_right = select_style_right;
  menu->item_count = num_items;

  return menu;
}

/* Function to free the memory allocated for the menu */
void FreeMenu(Menu* menu) {
  if (menu == NULL) return;
  for (int i = 0; i < menu->item_count; i++)
    free((char*)menu->items[i]);
  free(menu->items);
  free(menu);
}

/* Function to change the selected item in the menu */
void ChangeSelectedItem(Menu* menu, int direction) {
  if (direction == -1)
    menu->selected_item =
      (menu->selected_item - 1 + menu->item_count) % menu->item_count;
  else if (direction == 1)
    menu->selected_item = (menu->selected_item + 1) % menu->item_count;
}
