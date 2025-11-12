#include "ui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "init.h"
#include "input.h"
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

  for (int i = 0; i < MAX_PANELS; i++) FreePanel(&screen->panels[i]);

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

/* Render a border for a FloatingDialog */
void RenderFloatingDialogBorder(FloatingDialog* dialog) {
  if (!dialog || !dialog->visible) return;

  int x = dialog->position.x;
  int y = dialog->position.y;
  int width = dialog->size.width;
  int height = dialog->size.height;

  /* Draw top border */
  mvprintw(y, x, "%s", dialog->border.top_left);
  for (int i = 1; i < width - 1; i++)
    mvprintw(y, x + i, "%s", dialog->border.horizontal);
  mvprintw(y, x + width - 1, "%s", dialog->border.top_right);

  /* Draw bottom border */
  mvprintw(y + height - 1, x, "%s", dialog->border.bottom_left);
  for (int i = 1; i < width - 1; i++)
    mvprintw(y + height - 1, x + i, "%s", dialog->border.horizontal);
  mvprintw(y + height - 1, x + width - 1, "%s", dialog->border.bottom_right);

  /* Draw vertical borders */
  for (int i = 1; i < height - 1; i++) {
    mvprintw(y + i, x, "%s", dialog->border.vertical);
    mvprintw(y + i, x + width - 1, "%s", dialog->border.vertical);
  }
}

/* Render a quit confirmation message at the center of the screen */
void RenderQuitConfirmation(AppData* app) {
  if (app->popup_dialog == NULL) {
    /* Create the popup dialog if it doesn't exist */
    const char* message = "Are you sure you want to quit?";
    MenuItem menu_items[] = {{"Confirm", ForcefullyQuitApp},
                             {"Cancel", ClosePopup}};
    Menu menu = {.items = menu_items,
                 .selected_item = 0,
                 .focused_color = COLOR_WHITE,
                 .unfocused_color = COLOR_WHITE,
                 .select_style_left = "<",
                 .select_style_right = ">",
                 .item_count = sizeof(menu_items) / sizeof(MenuItem)};
    Border border = InitBorder();

    app->popup_dialog =
      CreateCenterFloatingDialog(app->screen, menu, message, border);
  }

  RenderFloatingDialog(app->popup_dialog);
}

/* Render a skip confirmation message at the center of the screen */
void RenderSkipConfirmation(AppData* app) {
  if (app->popup_dialog == NULL) {
    /* Create the popup dialog if it doesn't exist */
    const char* message = "Are you sure you want to skip this pomodoro step?";
    MenuItem menu_items[] = {{"Confirm", ForcefullySkipPomodoroStep},
                             {"Cancel", ClosePopup}};
    Menu menu = {.items = menu_items,
                 .selected_item = 0,
                 .focused_color = COLOR_WHITE,
                 .unfocused_color = COLOR_WHITE,
                 .select_style_left = "<",
                 .select_style_right = ">",
                 .item_count = sizeof(menu_items) / sizeof(MenuItem)};
    Border border = InitBorder();

    app->popup_dialog =
      CreateCenterFloatingDialog(app->screen, menu, message, border);
  }

  RenderFloatingDialog(app->popup_dialog);
}

/* Update panels from a given screen */
void UpdateScreen(Screen* screen) {
  getmaxyx(stdscr, screen->size.height, screen->size.width);

  int panels_width = screen->size.width / MAX_PANELS;
  int panels_height = screen->size.height;
  int remainder_width = screen->size.width % MAX_PANELS;

  /* Check if the screen can display all panels */
  if (screen->size.width >= screen->min_panel_size.width * MAX_PANELS) {
    /* Display all panels */
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

    /* Display only the current panel */
    int current_panel = screen->current_panel;
    if (current_panel >= 0 && current_panel < MAX_PANELS) {
      Dimensions panel_dimensions;
      panel_dimensions.width = screen->size.width;
      panel_dimensions.height = panels_height;

      Vector2D panel_position;
      panel_position.x = 0;
      panel_position.y = 0;

      /* Update the current panel and make it visible */
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
  while (*stack != NULL) PopHistory(stack);
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
    const char* item_label = menu->items[i].label;

    if (i == menu->selected_item) {
      SetColor(menu->focused_color, NO_COLOR, A_BOLD);

      size_t left_len = strlen(menu->select_style_left);
      size_t item_len = strlen(item_label);
      size_t right_len = strlen(menu->select_style_right);
      size_t full_text_size = left_len + item_len + right_len + 1;

      char* full_text = malloc(full_text_size);
      if (full_text) {
        snprintf(full_text, full_text_size, "%s%s%s", menu->select_style_left,
                 item_label, menu->select_style_right);
        RenderAtPanelCenter(panel, full_text, offset);
        free(full_text);
      } else {
        RenderAtPanelCenter(panel, item_label, offset);
      }
    } else {
      SetColor(menu->unfocused_color, NO_COLOR, A_NORMAL);
      RenderAtPanelCenter(panel, item_label, offset);
    }

    offset.y += line_spacing + 1;
  }
}

/* Print a given menu side by side with offset and spacing */
void PrintMenuSideBySide(Menu* menu, Vector2D offset, int spacing,
                         int container_width) {
  if (!container_width % 2 == 0) container_width += 1;
  int total_width = 0;

  /* Calculate the total width of the menu items */
  for (int i = 0; i < menu->item_count; i++) {
    total_width += strlen(menu->items[i].label) +
                   strlen(menu->select_style_left) +
                   strlen(menu->select_style_right);
    if (i < menu->item_count - 1) total_width += spacing;
  }

  /* Adjust the initial x-offset to center the menu */
  offset.x += (container_width - total_width) / 2;

  /* Render each menu item */
  for (int i = 0; i < menu->item_count; i++) {
    const char* item_label = menu->items[i].label;

    if (i == menu->selected_item) {
      SetColor(menu->focused_color, NO_COLOR, A_BOLD);

      size_t left_len = strlen(menu->select_style_left);
      size_t item_len = strlen(item_label);
      size_t right_len = strlen(menu->select_style_right);
      size_t full_text_size = left_len + item_len + right_len + 1;

      char* full_text = malloc(full_text_size);
      if (full_text) {
        snprintf(full_text, full_text_size, "%s%s%s", menu->select_style_left,
                 item_label, menu->select_style_right);
        mvprintw(offset.y, offset.x, "%s", full_text);
        free(full_text);
      } else {
        mvprintw(offset.y, offset.x, "%s", item_label);
      }
    } else {
      SetColor(menu->unfocused_color, NO_COLOR, A_NORMAL);
      mvprintw(offset.y, offset.x, "%s", item_label);
    }

    /* Move the offset horizontally to the right for the next menu item */
    offset.x += strlen(item_label) + strlen(menu->select_style_left) +
                strlen(menu->select_style_right) + spacing;
  }
}

/* Function to initialize a menu and return a pointer to it */
Menu* CreateMenu(MenuItem items[], int num_items, int focused_color,
                 int unfocused_color, const char* select_style_left,
                 const char* select_style_right) {
  Menu* menu = malloc(sizeof(struct Menu));
  if (menu == NULL) return NULL;

  menu->items = malloc(num_items * sizeof(MenuItem));
  if (menu->items == NULL) {
    free(menu);
    return NULL;
  }

  for (int i = 0; i < num_items; i++) {
    menu->items[i].label = strdup(items[i].label);
    menu->items[i].action = items[i].action;
    if (menu->items[i].label == NULL) {
      for (int j = 0; j < i; ++j) free((char*)menu->items[j].label);
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

  for (int i = 0; i < menu->item_count; i++) free((char*)menu->items[i].label);

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

/* Create a new FloatingDialog */
FloatingDialog* CreateFloatingDialog(Vector2D position, Dimensions size,
                                     Border border, Menu menu,
                                     const char* message) {
  FloatingDialog* dialog = (FloatingDialog*)malloc(sizeof(FloatingDialog));
  if (!dialog) return NULL;

  dialog->size = size;
  dialog->position = position;
  dialog->border = border;

  /* Deep copy the menu */
  dialog->menu.item_count = menu.item_count;
  dialog->menu.selected_item = menu.selected_item;
  dialog->menu.focused_color = menu.focused_color;
  dialog->menu.unfocused_color = menu.unfocused_color;
  dialog->menu.select_style_left = strdup(menu.select_style_left);
  dialog->menu.select_style_right = strdup(menu.select_style_right);

  dialog->menu.items = (MenuItem*)malloc(menu.item_count * sizeof(MenuItem));
  if (!dialog->menu.items) {
    free(dialog);
    return NULL;
  }

  for (int i = 0; i < menu.item_count; i++) {
    dialog->menu.items[i].label = strdup(menu.items[i].label);
    dialog->menu.items[i].action = menu.items[i].action;
  }

  dialog->message = strdup(message);
  dialog->visible = true;

  return dialog;
}

/* Free all memory of a FloatingDialog */
void FreeFloatingDialog(FloatingDialog* dialog) {
  if (!dialog) return;

  /* Free menu items */
  for (int i = 0; i < dialog->menu.item_count; i++) {
    free((char*)dialog->menu.items[i]
           .label); /* Cast to `char*` for `const char*` */
  }
  free(dialog->menu.items);

  /* Free menu styles */
  free((char*)dialog->menu.select_style_left);
  free((char*)dialog->menu.select_style_right);

  /* Free message */
  free(dialog->message);

  /* Free the dialog itself */
  free(dialog);
}

/* Render a FloatingDialog using ncurses */
void RenderFloatingDialog(FloatingDialog* dialog) {
  if (!dialog || !dialog->visible) return;

  int x = dialog->position.x;
  int y = dialog->position.y;
  int width = dialog->size.width;
  int height = dialog->size.height;

  /* Draw borders */
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  RenderFloatingDialogBorder(dialog);

  /* Draw background */
  for (int i = 1; i < height - 1; i++)
    for (int j = 1; j < width - 1; j++) mvprintw(y + i, x + j, " ");

  /* Print message */
  int msg_x = x + 2;
  int msg_y = y + 1;
  mvprintw(msg_y, msg_x, "%s", dialog->message);

  /* Render menu */
  Vector2D menu_offset = {x, y + 3};
  int menu_spacing = 4;
  PrintMenuSideBySide(&dialog->menu, menu_offset, menu_spacing, width);
}

/* Create a FloatingDialog centered on the screen */
FloatingDialog* CreateCenterFloatingDialog(Screen* screen, Menu menu,
                                           const char* message, Border border) {
  const int padding = 4;
  const int msg_len = strlen(message);
  int menu_width = 0;

  for (int i = 0; i < menu.item_count; i++) {
    int label_len = strlen(menu.items[i].label);
    menu_width = Max(menu_width, label_len);
    menu_width += padding;
  }

  int width = Max(msg_len, menu_width) + padding;
  int height = menu.item_count + padding;

  Vector2D position = {.x = (screen->size.width - width) / 2,
                       .y = (screen->size.height - height) / 2};

  Dimensions size = {.width = width, .height = height};

  return CreateFloatingDialog(position, size, border, menu, message);
}

/* Render a pomodoro status */
void RenderPomodoroStatus(AppData* app, Dimensions anim_size,
                          Vector2D anim_pos) {
  int step = app->pomodoro_data.current_step;
  int icon_type = GetConfigIconType();
  const char *icon, *status_text;
  int color, duration;
  char message[64];
  char total_time[32];
  char cycle_info[16];

  /* Determine the properties based on the step */
  switch (step) {
    case WORK_TIME:
      color = COLOR_MAGENTA;
      icon = WORK_ICONS[icon_type];
      if (icon_type == ASCII)
        status_text = "Pomodoro  ";
      else
        status_text = "Pomodoro";
      duration = app->pomodoro_data.work_time;
      break;
    case SHORT_PAUSE:
      color = COLOR_CYAN;
      icon = SHORT_PAUSE_ICONS[icon_type];
      status_text = "Pause";
      duration = app->pomodoro_data.short_pause_time;
      break;
    case LONG_PAUSE:
      color = COLOR_CYAN;
      icon = LONG_PAUSE_ICONS[icon_type];
      status_text = "Long pause";
      duration = app->pomodoro_data.long_pause_time;
      break;
    default:
      return;
  }

  snprintf(cycle_info, sizeof(cycle_info), "%02d/%02d",
           app->pomodoro_data.current_cycle + 1,
           app->pomodoro_data.total_cycles);
  snprintf(message, sizeof(message), "%s %s", icon, status_text);
  snprintf(total_time, sizeof(total_time), "[%d minutes]", duration);

  /* Calculate centered positions */
  int total_width = strlen(message) + strlen(total_time);
  int start_x = anim_pos.x + (anim_size.width - total_width) / 2 + 1;
  int render_y = anim_pos.y + anim_size.height + 1;
  int start_x_above = start_x + strlen(message) + 1 + strlen(cycle_info);
  int render_y_above = anim_pos.y - 1;

  /* Render the cycle info and controls above the animation */
  SetColor(color, NO_COLOR, A_BOLD);
  mvprintw(render_y_above, start_x_above, "%s", cycle_info);
  RenderPomodoroControls(app, (Vector2D){start_x, render_y_above});

  /* Render the message and total_time side by side */
  SetColor(color, NO_COLOR, A_BOLD);
  mvprintw(render_y, start_x, "%s", message);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(render_y, start_x + strlen(message) - 1, "%s", total_time);

  /* Render the current_time centered below message and total_time */
  char* current_time =
    FormatRemainingTime(app->pomodoro_data.current_step_time, duration);
  int current_time_width = strlen(current_time);
  int time_start_x = start_x + (total_width - current_time_width) / 2;
  mvprintw(render_y + 1, time_start_x, "%s", current_time);
  free(current_time);
}

/* Render a pomodoro controllers */
void RenderPomodoroControls(AppData* app, Vector2D pos) {
  int step = app->pomodoro_data.current_step;
  int icon_type = GetConfigIconType();
  const char* skip_icon = SKIP_ICONS[icon_type];
  const char* pause_icon = PAUSE_ICONS[icon_type];
  int color;

  if (step == WORK_TIME)
    color = COLOR_MAGENTA;
  else if (step == SHORT_PAUSE || step == LONG_PAUSE)
    color = COLOR_CYAN;
  else
    return;

  SetColor(color, NO_COLOR, A_BOLD);
  mvprintw(pos.y, pos.x, "%s ", skip_icon);
  int skip_length = strlen(skip_icon);
  if (icon_type == NERD_ICONS)
    skip_length--;
  else if (icon_type == EMOJIS)
    skip_length -= 2;
  else if (icon_type == ASCII)
    skip_length++;

  SetColor(app->is_paused ? color : COLOR_BLACK, NO_COLOR, A_BOLD);
  mvprintw(pos.y, pos.x + skip_length, "%s ", pause_icon);
}
