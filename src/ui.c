#include "ui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "tomato.h"
#include "util.h"

/* PRIVATE UI FUNCTIONS */
/* Slides */
static int strDisplayWidth(const char* s);
static int utf8DisplayWidth(const char* s);
static SlideToken* parseSlideText(const char* text, int icon_type);
static SlideDef* buildSlideFromText(const char* text, int w, int h,
                                    int icon_type);
static void welcomeRender(AppData* app, SlideDef* def);
static void welcomeUpdate(AppData* app, SlideDef* def);
static void renderSlideBox(int x, int y, int w, int h);
static void renderSlideTokens(int x, int y, int w, int h, SlideToken* tokens);
static void continueAndClose(AppData* app);
static void abandonAndClose(AppData* app);
static void continueProgressRender(AppData* app, int x, int y, int w,
                                   SlideDef* def, SlideProgress* params);
static SlideProgress* slideProgressDup(const SlideProgress* p);
static SlideControls* slideControlsDup(const ControlButton* btns, int count);
/* Screen / Panel */
static Panel createPanel(Dimensions size, Vector2D position);
static void freePanel(Panel* panel);
static void updatePanel(Panel* panel, Dimensions size, Vector2D position);
static void renderAtPanelCenter(Panel* panel, const char* content,
                                Vector2D offset);
/* Menu */
static void printMenuSideBySide(AppData* app, Menu* menu, Vector2D offset,
                                int spacing, int container_width);
/* Floating Dialog */
static void renderFloatingDialogBorder(FloatingDialog* dialog);

/**
 * ---------------------------------------------------------------------------
 * Screen / Panel
 * ---------------------------------------------------------------------------
 */

/**
 * Create a screen struct with MAX_PANELS in horizontal rows.
 * Initializes panels based on current terminal size.
 * @return Pointer to the created Screen, or NULL on allocation failure
 */
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

    screen->panels[i] = createPanel(panel_dimensions, panel_position);
  }

  return screen;
}

/**
 * Free all memory associated with a Screen struct.
 * @param screen Pointer to the screen to free
 */
void FreeScreen(Screen* screen) {
  if (screen == NULL) return;

  for (int i = 0; i < MAX_PANELS; i++) freePanel(&screen->panels[i]);

  free(screen);
}

/**
 * Update panel positions and dimensions based on screen layout.
 * @param screen Pointer to the screen to update
 * @param has_error_line If true, reduce panel height to leave room for error line
 */
void UpdateScreen(Screen* screen, bool has_error_line) {
  getmaxyx(stdscr, screen->size.height, screen->size.width);

  int panels_width = screen->size.width / MAX_PANELS;
  int panels_height =
    has_error_line ? screen->size.height - 1 : screen->size.height;
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
      updatePanel(&screen->panels[i], panel_dimensions, panel_position);
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
      updatePanel(&screen->panels[current_panel], panel_dimensions,
                  panel_position);
    }
  }
}

/**
 * Render a border around a panel using ncurses.
 * @param panel Panel to render border around
 * @param border Border character configuration
 */
void RenderPanelBorder(Panel panel, Border border) {
  if (!panel.visible) return;
  int x, y;

  mvprintw(panel.position.y, panel.position.x, "%s", border.top_left);
  for (x = panel.position.x + 1; x < panel.position.x + panel.size.width - 1;
       x++)
    mvprintw(panel.position.y, x, "%s", border.horizontal);
  mvprintw(panel.position.y, panel.position.x + panel.size.width - 1, "%s",
           border.top_right);

  mvprintw(panel.position.y + panel.size.height - 1, panel.position.x, "%s",
           border.bottom_left);
  for (x = panel.position.x + 1; x < panel.position.x + panel.size.width - 1;
       x++)
    mvprintw(panel.position.y + panel.size.height - 1, x, "%s",
             border.horizontal);
  mvprintw(panel.position.y + panel.size.height - 1,
           panel.position.x + panel.size.width - 1, "%s", border.bottom_right);

  for (y = panel.position.y + 1; y < panel.position.y + panel.size.height - 1;
       y++) {
    mvprintw(y, panel.position.x, "%s", border.vertical);
    mvprintw(y, panel.position.x + panel.size.width - 1, "%s", border.vertical);
  }
}

/**
 * Render an animation at the center of a panel.
 * @param panel Pointer to the panel
 * @param animation Pointer to the animation to render
 * @param offset Offset from the center position
 */
void RenderAnimationAtPanelCenter(Panel* panel, Rollfilm* animation,
                                  Vector2D offset) {
  if (animation->render == NULL) return;

  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

  int frame_x = panel_center_x - animation->frame_width / 2 + offset.x;
  int frame_y = panel_center_y - animation->frame_height / 2 + offset.y;

  animation->render(animation, frame_y, frame_x);
}

/**
 * Render a screen size error message.
 * Displayed when terminal is too small for the app.
 * @param screen Pointer to the screen
 * @param panel Pointer to a panel to use for dimensions
 */
void RenderScreenSizeError(Screen* screen, Panel* panel) {
  char* content;
  int required_length;

  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  renderAtPanelCenter(panel, "TERMINAL SIZE TOO SMALL!", (Vector2D){0, -2});

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  required_length = snprintf(NULL, 0, "Width = %2d Height = %2d",
                             screen->size.width, screen->size.height) +
                    1;
  content = (char*)malloc(required_length);
  if (content != NULL) {
    snprintf(content, required_length, "Width = %2d Height = %2d",
             screen->size.width, screen->size.height);
    renderAtPanelCenter(panel, content, (Vector2D){0, -1});
    free(content);
  }

  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  renderAtPanelCenter(panel, "SIZE NEEDED IN CURRENT CONFIG", (Vector2D){0, 0});

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  required_length =
    snprintf(NULL, 0, "Width = %2d Height = %2d", screen->min_panel_size.width,
             screen->min_panel_size.height) +
    1;
  content = (char*)malloc(required_length);
  if (content != NULL) {
    snprintf(content, required_length, "Width = %2d Height = %2d",
             screen->min_panel_size.width, screen->min_panel_size.height);
    renderAtPanelCenter(panel, content, (Vector2D){0, 1});
    free(content);
  }
}

/**
 * Create a panel struct with the specified size and position.
 * Initializes history and sets default mode.
 * @param size Dimensions of the new panel
 * @param position Position of the panel on screen
 * @return New Panel struct (not a pointer)
 */
static Panel createPanel(Dimensions size, Vector2D position) {
  Panel panel;

  panel.mode = DEFAULT;
  panel.size = size;
  panel.visible = true;
  panel.position = position;
  panel.scene_history = CreateHistory();
  if (panel.scene_history == NULL) LogError("createPanel", MALLOC_ERROR);

  return panel;
}

/**
 * Free the memory of a panel (history).
 * @param panel Pointer to the panel to free
 */
static void freePanel(Panel* panel) {
  if (panel == NULL) return;
  FreeHistory(panel->scene_history, NULL);
}

/**
 * Update a single panel's dimensions and position.
 * @param panel Pointer to the panel to update
 * @param size New dimensions
 * @param position New position
 */
static void updatePanel(Panel* panel, Dimensions size, Vector2D position) {
  panel->size = size;
  panel->position = position;
}

/**
 * ---------------------------------------------------------------------------
 * Click Regions
 * ---------------------------------------------------------------------------
 */

/**
 * Clear all registered click regions for the current frame.
 * @param app Pointer to the application data
 */
void ClearClickRegions(AppData* app) { app->click_region_count = 0; }

/**
 * Register a clickable screen region for mouse interaction.
 * @param app Pointer to the application data
 * @param x Top-left x coordinate
 * @param y Top-left y coordinate
 * @param width Region width in characters
 * @param height Region height in characters
 * @param type Region type (DIRECT, MENU_ITEM, POPUP_ITEM)
 * @param action Action function (for REGION_DIRECT)
 * @param menu_index Menu index (for REGION_MENU_ITEM)
 * @param item_index Item index within menu
 */
void RegisterClickRegion(AppData* app, int x, int y, int width, int height,
                         RegionType type, MenuAction action, int menu_index,
                         int item_index, int note_id) {
  if (app->click_region_count >= MAX_CLICK_REGIONS) return;
  ClickRegion* r = &app->click_regions[app->click_region_count++];
  r->pos.x = x;
  r->pos.y = y;
  r->size.width = width;
  r->size.height = height;
  r->type = type;
  r->action = action;
  r->menu_index = menu_index;
  r->item_index = item_index;
  r->note_id = note_id;
}

/**
 * Render content string at the center of a panel.
 * @param panel Pointer to the panel
 * @param content String to render
 * @param offset Offset from the center position
 */
static void renderAtPanelCenter(Panel* panel, const char* content,
                                Vector2D offset) {
  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

  int content_x = panel_center_x - strlen(content) / 2 + offset.x;
  int content_y = panel_center_y + offset.y;

  mvprintw(content_y, content_x, "%s", content);
}

/* ---------------------------------------------------------------------------
 * History
 * --------------------------------------------------------------------------- */

/**
 * Perform undo operation - move back in history.
 * Pops from past stack and pushes current to future stack.
 * @param history Pointer to the history manager
 */
void UndoHistory(History* history) {
  if (!history || !history->past) {
    printf("No scenes in past stack to undo.\n");
    return;
  }

  /* Save current to future */
  if (history->present >= 0) {
    int* scene_ptr = (int*)malloc(sizeof(int));
    if (scene_ptr) {
      *scene_ptr = history->present;
      HistoryPush(history, scene_ptr, free, true);
    }
  }

  /* Pop from past */
  int* prev = (int*)HistoryPop(history, true);
  if (prev) {
    history->present = *prev;
    free(prev);
  }
}

/**
 * Perform redo operation - move forward in history.
 * Pops from future stack and pushes current to past stack.
 * @param history Pointer to the history manager
 */
void RedoHistory(History* history) {
  if (!history || !history->future) {
    printf("No scenes in future stack to redo.\n");
    return;
  }

  /* Save current to past */
  if (history->present >= 0) {
    int* scene_ptr = (int*)malloc(sizeof(int));
    if (scene_ptr) {
      *scene_ptr = history->present;
      HistoryPush(history, scene_ptr, free, false);
    }
  }

  /* Pop from future */
  int* next = (int*)HistoryPop(history, false);
  if (next) {
    history->present = *next;
    free(next);
  }
}

/**
 * Execute a new scene, adding current to past stack.
 * Clears future stack and sets new present scene.
 * @param history Pointer to the history manager
 * @param new_scene SceneType to execute
 */
void ExecuteHistory(History* history, int new_scene) {
  if (!history) return;

  /* Save current to past */
  if (history->present >= 0) {
    int* scene_ptr = (int*)malloc(sizeof(int));
    if (scene_ptr) {
      *scene_ptr = history->present;
      HistoryPush(history, scene_ptr, free, false);
    }
  }

  history->present = new_scene;

  /* Clear future stack */
  while (history->future) {
    int* data = (int*)HistoryPop(history, false);
    if (data) free(data);
  }
}

/* ---------------------------------------------------------------------------
 * Menu
 * --------------------------------------------------------------------------- */

/**
 * Initialize a menu with items and styling.
 * @param items Array of MenuItem structures
 * @param num_items Number of items in the array
 * @param focused_color Color for selected item
 * @param unfocused_color Color for unselected items
 * @param select_style_left Left prefix for selected item
 * @param select_style_right Right suffix for selected item
 * @return Pointer to the created Menu, or NULL on allocation failure
 */
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

/**
 * Free memory allocated for a Menu.
 * @param menu Pointer to the menu to free
 */
void FreeMenu(Menu* menu) {
  if (menu == NULL) return;

  for (int i = 0; i < menu->item_count; i++) free((char*)menu->items[i].label);

  free(menu->items);
  free(menu);
}

/**
 * Print a menu centered on screen with offset and line spacing.
 * @param app Pointer to the application data (for click region tracking)
 * @param panel Pointer to the panel containing the menu
 * @param menu Pointer to the menu to print
 * @param offset Offset from screen center
 * @param line_spacing Extra lines between items
 */
void PrintMenuAtCenter(AppData* app, Panel* panel, Menu* menu, Vector2D offset,
                       int line_spacing) {
  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

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
        renderAtPanelCenter(panel, full_text, offset);
        free(full_text);
      } else
        renderAtPanelCenter(panel, item_label, offset);
    } else {
      SetColor(menu->unfocused_color, NO_COLOR, A_NORMAL);
      renderAtPanelCenter(panel, item_label, offset);
    }

    int content_x = panel_center_x - strlen(item_label) / 2 + offset.x;
    int content_y = panel_center_y + offset.y;
    int menu_idx = -1;
    for (int m = 0; m < MAX_MENUS; m++) {
      if (menu == app->menus[m]) {
        menu_idx = m;
        break;
      }
    }
    RegisterClickRegion(app, content_x, content_y, strlen(item_label), 1,
                        REGION_MENU_ITEM, menu->items[i].action, menu_idx, i,
                        -1);

    offset.y += line_spacing + 1;
  }
}

/**
 * Change the selected item in the menu.
 * @param menu Pointer to the menu
 * @param direction 1 for next, -1 for previous
 */
void ChangeSelectedItem(Menu* menu, int direction) {
  if (direction == -1)
    menu->selected_item =
      (menu->selected_item - 1 + menu->item_count) % menu->item_count;
  else if (direction == 1)
    menu->selected_item = (menu->selected_item + 1) % menu->item_count;
}

/**
 * Print a menu side by side (e.g., for work/pause display).
 * @param app Pointer to the application data (for click region tracking)
 * @param menu Pointer to the menu to print
 * @param offset Offset from screen top-left
 * @param spacing Space between items
 * @param container_width Total width available
 */
static void printMenuSideBySide(AppData* app, Menu* menu, Vector2D offset,
                                int spacing, int container_width) {
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
    int item_x = offset.x;
    int item_y = offset.y;

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
      } else
        mvprintw(offset.y, offset.x, "%s", item_label);
    } else {
      SetColor(menu->unfocused_color, NO_COLOR, A_NORMAL);
      mvprintw(offset.y, offset.x, "%s", item_label);
    }

    RegisterClickRegion(app, item_x, item_y, strlen(item_label), 1,
                        REGION_POPUP_ITEM, NULL, -1, i, -1);

    /* Move the offset horizontally to the right for the next menu item */
    offset.x += strlen(item_label) + strlen(menu->select_style_left) +
                strlen(menu->select_style_right) + spacing;
  }
}

/* ---------------------------------------------------------------------------
 * Floating Dialog
 * --------------------------------------------------------------------------- */

/**
 * Create a FloatingDialog on given position.
 * @param position Vector2D of where to initialize the dialog
 * @param size Dimensions size of the dialog
 * @param border Border character configuration
 * @param menu Menu to display in the dialog
 * @param message Message text to display
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
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
  if (!dialog->menu.select_style_left || !dialog->menu.select_style_right) {
    free((char*)dialog->menu.select_style_left);
    free((char*)dialog->menu.select_style_right);
    free(dialog);
    return NULL;
  }

  dialog->menu.items = (MenuItem*)malloc(menu.item_count * sizeof(MenuItem));
  if (!dialog->menu.items) {
    free((char*)dialog->menu.select_style_left);
    free((char*)dialog->menu.select_style_right);
    free(dialog);
    return NULL;
  }

  for (int i = 0; i < menu.item_count; i++) {
    dialog->menu.items[i].label = strdup(menu.items[i].label);
    if (!dialog->menu.items[i].label) {
      for (int j = 0; j < i; j++) free((char*)dialog->menu.items[j].label);
      free(dialog->menu.items);
      free((char*)dialog->menu.select_style_left);
      free((char*)dialog->menu.select_style_right);
      free(dialog);
      return NULL;
    }
    dialog->menu.items[i].action = menu.items[i].action;
  }

  dialog->message = strdup(message);
  if (!dialog->message) {
    for (int i = 0; i < menu.item_count; i++)
      free((char*)dialog->menu.items[i].label);
    free(dialog->menu.items);
    free((char*)dialog->menu.select_style_left);
    free((char*)dialog->menu.select_style_right);
    free(dialog);
    return NULL;
  }
  dialog->visible = true;
  dialog->slides = NULL;
  dialog->slideCount = 0;
  dialog->currentSlide = 0;
  dialog->slide_type = SLIDE_TYPE_NONE;
  dialog->hovered_button = -1;

  return dialog;
}

/**
 * Create a FloatingDialog centered on the screen.
 * @param screen Pointer to the screen for dimensions
 * @param menu Menu to display in the dialog
 * @param message Message text to display
 * @param border Border character configuration
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreateCenterFloatingDialog(Screen* screen, Menu menu,
                                           const char* message, Border border) {
  const int padding = 4;
  int msg_lines = 1, longest_line = 0, current_line_len = 0;

  for (const char* p = message; *p; p++) {
    if (*p == '\n') {
      msg_lines++;
      longest_line = Max(longest_line, current_line_len);
      current_line_len = 0;
    } else {
      current_line_len++;
    }
  }
  longest_line = Max(longest_line, current_line_len);

  int menu_width = 0;
  for (int i = 0; i < menu.item_count; i++) {
    int label_len = strlen(menu.items[i].label);
    menu_width = Max(menu_width, label_len);
    menu_width += padding;
  }

  int width = Max(longest_line, menu_width) + padding;
  int height = menu.item_count + padding + (msg_lines - 1);

  Vector2D position = {.x = (screen->size.width - width) / 2,
                       .y = (screen->size.height - height) / 2};

  Dimensions size = {.width = width, .height = height};

  return CreateFloatingDialog(position, size, border, menu, message);
}

/**
 * Free all memory of a FloatingDialog.
 * @param dialog Pointer to the dialog to free
 */
void FreeFloatingDialog(FloatingDialog* dialog) {
  if (!dialog) return;

  /* Free menu items */
  for (int i = 0; i < dialog->menu.item_count; i++)
    free((char*)dialog->menu.items[i]
           .label); /* Cast to `char*` for `const char*` */
  free(dialog->menu.items);

  /* Free menu styles */
  free((char*)dialog->menu.select_style_left);
  free((char*)dialog->menu.select_style_right);

  /* Free message */
  free(dialog->message);

  /* Free welcome slides if present */
  if (dialog->slides) {
    FreeWelcomeSlides(dialog->slides, dialog->slideCount);
    dialog->slides = NULL;
  }

  /* Free the dialog itself */
  free(dialog);
}

/**
 * Update a FloatingDialog to be centered on the current screen.
 * Call this before rendering to handle screen resize.
 * @param dialog Pointer to the dialog to update
 * @param screen Pointer to the screen for current dimensions
 */
void UpdateFloatingDialog(FloatingDialog* dialog, Screen* screen) {
  if (!dialog || !screen) return;

  int width = dialog->size.width;
  int height = dialog->size.height;

  dialog->position.x = (screen->size.width - width) / 2;
  dialog->position.y = (screen->size.height - height) / 2;
}

/**
 * Render a FloatingDialog using ncurses.
 * @param app Pointer to the application data (for click region tracking)
 * @param dialog Pointer to the dialog to render
 */
void RenderFloatingDialog(AppData* app, FloatingDialog* dialog) {
  if (!dialog || !dialog->visible) return;

  int x = dialog->position.x;
  int y = dialog->position.y;
  int width = dialog->size.width;
  int height = dialog->size.height;

  /* Draw borders */
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  renderFloatingDialogBorder(dialog);

  /* Draw background */
  for (int i = 1; i < height - 1; i++)
    for (int j = 1; j < width - 1; j++) mvprintw(y + i, x + j, " ");

  /* Print message (supports multi-line with \n) */
  int msg_x = x + 2;
  int msg_y = y + 1;
  int msg_lines = 0;
  const char* msg = dialog->message;
  while (*msg) {
    const char* nl = strchr(msg, '\n');
    int len = nl ? (int)(nl - msg) : (int)strlen(msg);
    mvprintw(msg_y + msg_lines, msg_x, "%.*s", len, msg);
    msg_lines++;
    msg = nl ? nl + 1 : msg + len;
  }

  /* Render menu */
  Vector2D menu_offset = {x, y + msg_lines + 2};
  int menu_spacing = 4;
  printMenuSideBySide(app, &dialog->menu, menu_offset, menu_spacing, width);
}

/**
 * Render a border for a FloatingDialog.
 * @param dialog Pointer to the dialog to render
 */
static void renderFloatingDialogBorder(FloatingDialog* dialog) {
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

/* ---------------------------------------------------------------------------
 * Popups
 * --------------------------------------------------------------------------- */

/**
 * Render a quit confirmation message at the center of the screen.
 * @param app Pointer to the application data
 */
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

  UpdateFloatingDialog(app->popup_dialog, app->screen);
  RenderFloatingDialog(app, app->popup_dialog);
}

/**
 * Render a critical error quit confirmation - no cancel option.
 * Used when app is frozen due to critical error.
 * @param app Pointer to the application data
 */
void RenderCriticalQuitConfirmation(AppData* app) {
  if (app->popup_dialog == NULL) {
    const char* message = "Critical error - app must quit";
    MenuItem menu_items[] = {{"Quit", ForcefullyQuitApp}};
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

  UpdateFloatingDialog(app->popup_dialog, app->screen);
  RenderFloatingDialog(app, app->popup_dialog);
}

/**
 * Render a reset pomodoro menu at the center of the screen.
 * @param app Pointer to the application data
 */
void RenderResetMenu(AppData* app) {
  if (app->popup_dialog == NULL) {
    /* Create the popup dialog if it doesn't exist */
    const char* message = "Do you want to reset this this pomodoro?";
    MenuItem menu_items[] = {
      {"Cycle", ResetPomodoroCycle},
      {"Step", ResetPomodoroStep},
      {"Cancel", ClosePopup},
    };
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

  UpdateFloatingDialog(app->popup_dialog, app->screen);
  RenderFloatingDialog(app, app->popup_dialog);
}

/**
 * Render a skip confirmation message at the center of the screen.
 * @param app Pointer to the application data
 */
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

  UpdateFloatingDialog(app->popup_dialog, app->screen);
  RenderFloatingDialog(app, app->popup_dialog);
}

/**
 * Create a welcome popup dialog for first-time users.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateWelcomeDialog(AppData* app) {
  (void)app;
  Dimensions size = {.width = 38, .height = 26};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Get Started", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "[",
               .select_style_right = "]",
               .item_count = 1};
  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (dialog != NULL) {
    dialog->slide_type = SLIDE_TYPE_WELCOME;
    dialog->slides = BuildWelcomeSlides();
    dialog->slideCount = 3 * WELCOME_SLIDE_COUNT;
    dialog->currentSlide = 0;
    /* "Next  >" is always at index 1 on the first slide */
    dialog->hovered_button = 1;
  }
  return dialog;
}

/**
 * Create a continue/cancel popup dialog for unfinished sessions.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateContinueDialog(AppData* app) {
  Dimensions size = {.width = 39, .height = 19};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Continue", continueAndClose},
                      {"Discard", abandonAndClose}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "[",
               .select_style_right = "]",
               .item_count = 2};
  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (dialog != NULL) {
    dialog->slide_type = SLIDE_TYPE_CONTINUE;
    dialog->hovered_button = 0;
    dialog->slides = BuildContinueSlides(app);
    if (!dialog->slides) {
      FreeFloatingDialog(dialog);
      return NULL;
    }
    dialog->slideCount = 3;
    dialog->currentSlide = 0;
  }
  return dialog;
}

/* ---------------------------------------------------------------------------
 * Pomodoro
 * --------------------------------------------------------------------------- */

/**
 * Render pomodoro status (timer display) with animation.
 * @param app Pointer to the application data
 * @param anim_size Dimensions of the animation area
 * @param anim_pos Position of the animation
 */
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

/**
 * Render pomodoro control icons (pause, skip, etc.).
 * @param app Pointer to the application data
 * @param pos Position to render controls
 */
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

  RegisterClickRegion(app, pos.x, pos.y, skip_length, 1, REGION_DIRECT,
                      ForcefullySkipPomodoroStep, -1, -1, -1);
  RegisterClickRegion(app, pos.x + skip_length, pos.y, strlen(pause_icon), 1,
                      REGION_DIRECT, TogglePause, -1, -1, -1);

  SetColor(app->is_paused ? color : COLOR_BLACK, NO_COLOR, A_BOLD);
  mvprintw(pos.y, pos.x + skip_length, "%s ", pause_icon);
}

/**
 * ---------------------------------------------------------------------------
 * Slides
 * ---------------------------------------------------------------------------
 */

#define SLIDE_W 41
#define SLIDE_INNER_W (SLIDE_W - 2)

/**
 * Build all welcome slide definitions for all icon types.
 * Allocates 3 * stride SlideDef instances, one per icon
 * type per slide. Index with [iconType * stride + slideIdx].
 * @return Pointer to array of SlideDef pointers, or NULL on allocation failure
 */
SlideDef** BuildWelcomeSlides(void) {
  /**
   * Slide text definitions in token-format with escape sequences.
   * Three entries per slide: [0]=nerd-icons, [1]=emoji, [2]=ascii.
   */

  /* s0 – Title slide */
  static const char* s0_texts[3] = {
    /* nerd & emoji: icon placeholder + text */
    "\\n\\n\\n\\n\\c13\\aC{W}  Tomato.C\\n\\n\\aCPomodoro + notes in\\n\\aC"
    "your terminal\\n\\n\\c13\\aCStay focused. Stay fast",
    "\\n\\n\\n\\n\\c13\\aC{W}  Tomato.C\\n\\n\\aCPomodoro + notes in\\n\\aC"
    "your terminal\\n\\n\\c13\\aCStay focused. Stay fast",
    /* ascii: icon is empty → just extra space */
    "\\n\\n\\n\\n\\c13\\aCTomato.C\\n\\n\\aCPomodoro + notes in\\n\\aC"
    "your terminal\\n\\n\\c13\\aCStay focused. Stay fast",
  };

  /* s1 – Split-panel layout */
  static const char* s1_texts[3] = {
    /* nerd (1-col icons: left_pad=5) */
    "\\n\\n\\n\\n\\c09   ┏━━━━━━━━━━━━━┳\\x18\\c16━━━━━━━━━━━━━━━━━┓  \\n"
    "\\c16   ┃ TIMER PANEL ┃ NOTES PANEL     ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃             ┃                 ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃ {S} {P}   01/04 ┃ [ ] Undone task ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃      {W}      ┃ [X] Done task   ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃    24:59    ┃ ─ Note          ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c09   ┗━━━━━━━━━━━━━┻\\x18\\c16━━━━━━━━━━━━━━━━━┛  \\n"
    "\\n"
    "\\c16   • SPACE → switch focus\\n"
    "   • Responsive terminal layout\\n"
    "   • Mouse support available",
    /* emoji (2-col icons: left_pad=4) */
    "\\n\\n\\n\\n\\c09   ┏━━━━━━━━━━━━━┳\\x18\\c16━━━━━━━━━━━━━━━━━┓  \\n"
    "\\c16   ┃ TIMER PANEL ┃ NOTES PANEL     ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃             ┃                 ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃ {S} {P}   01/04 ┃ [ ] Undone task ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃     {W}      ┃ [X] Done task   ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c16   ┃    24:59    ┃ ─ Note          ┃   \\x03\\c09┃\\x17\\c09┃\\n"
    "\\c09   ┗━━━━━━━━━━━━━┻\\x18\\c16━━━━━━━━━━━━━━━━━┛  \\n"
    "\\n"
    "\\c16   • SPACE → switch focus\\n"
    "   • Responsive terminal layout\\n"
    "   • Mouse support available",
    /* ascii (1-col icons: same padding as nerd) */
    "\\n\\n\\n\\n\\c09   +-------------+\\x18\\c16-----------------+  \\n"
    "\\c16   | TIMER PANEL | NOTES PANEL     |   \\x03\\c09|\\x17\\c09|\\n"
    "\\c16   |             |                 |   \\x03\\c09|\\x17\\c09|\\n"
    "\\c16   | {S} {P}   01/04 | [ ] Undone task |   \\x03\\c09|\\x17\\c09|\\n"
    "\\c16   |      {W}      | [X] Done task   |   \\x03\\c09|\\x17\\c09|\\n"
    "\\c16   |    24:59    | ─ Note          |   \\x03\\c09|\\x17\\c09|\\n"
    "\\c09   +-------------+\\x18\\c16-----------------+  \\n"
    "\\n"
    "\\c16   * SPACE -> switch focus\\n"
    "   * Responsive terminal layout\\n"
    "   * Mouse support available",
  };

  /* s2 – Pomodoro workflow */
  static const char* s2_texts[3] = {
    "\\n\\n\\n\\n\\c13\\aC{W} POMODORO WORKFLOW\\n\\n\\aC"
    "Work → Break → Work → Long Break\\n\\n\\c16"
    "\\x03\\c15•\\x05\\c07Pause / resume\\n"
    "\\x03\\c15•\\x05\\c07Auto-save sessions\\n"
    "\\x03\\c15•\\x05\\c07Notifications + sound\\n"
    "\\x03\\c15•\\x05\\c07Continue unfinished timers\\n\\n"
    "\\x01\\c15[p]\\x05\\c07pause\\x13\\c15[s]\\x17\\c07skip\\x24\\c15[Ctrl+r]"
    "\\x33\\c07reset",
    "\\n\\n\\n\\n\\c13\\aC{W} POMODORO WORKFLOW\\n\\n\\aC"
    "Work → Break → Work → Long Break\\n\\n\\c16"
    "\\x03\\c15•\\x05\\c07Pause / resume\\n"
    "\\x03\\c15•\\x05\\c07Auto-save sessions\\n"
    "\\x03\\c15•\\x05\\c07Notifications + sound\\n"
    "\\x03\\c15•\\x05\\c07Continue unfinished timers\\n\\n"
    "\\x01\\c15[p]\\x05\\c07pause\\x13\\c15[s]\\x17\\c07skip\\x24\\c15[Ctrl+r]"
    "\\x33\\c07reset",
    "\\n\\n\\n\\n\\c13\\aCPOMODORO WORKFLOW\\n\\n\\aC"
    "Work -> Break -> Work -> Long Break\\n\\n\\c16"
    "\\x03\\c15*\\x05\\c07Pause / resume\\n"
    "\\x03\\c15*\\x05\\c07Auto-save sessions\\n"
    "\\x03\\c15*\\x05\\c07Notifications + sound\\n"
    "\\x03\\c15*\\x05\\c07Continue unfinished timers\\n\\n"
    "\\x01\\c15[p]\\x05\\c07pause\\x13\\c15[s]\\x17\\c07skip\\x24\\c15[Ctrl+r]"
    "\\x33\\c07reset",
  };

  /* s3 – Hierarchical notes / Vim editing */
  static const char* s3_texts[3] = {
    "\\n\\n\\n\\n\\c11\\aC{N} HIERARCHICAL NOTES\\n\\n\\c16"
    "\\x07\\c16• -\\x12\\c07Plain notes\\n"
    "\\x07\\c16• [ ]\\x14\\c07Undone tasks\\n"
    "\\x07\\c16• [X]\\x14\\c07Done tasks\\n\\n\\c16"
    "\\aC──────────────────────\\n"
    "\\c14\\aC{M}  VIM-LIKE EDITING\\n\\n\\c16"
    "\\x07\\c16• DEFAULT →\\x20\\c07manage\\n"
    "\\x07\\c16• NORMAL  →\\x20\\c07navigate\\n"
    "\\x07\\c16• INSERT  →\\x20\\c07type\\n"
    "\\x07\\c16• VISUAL  →\\x20\\c07select\\n\\n"
    "\\x03\\c15[n/t]\\x09\\c07add\\x16\\c15[u/Ctrl+r]\\x27\\c07undo/redo\\n"
    "\\x03\\c15[e]\\x09\\c07edit\\x16\\c15[V]\\x27\\c07move",
    "\\n\\n\\n\\n\\c11\\aC{N} HIERARCHICAL NOTES\\n\\n\\c16"
    "\\x07\\c16• -\\x12\\c07Plain notes\\n"
    "\\x07\\c16• [ ]\\x14\\c07Undone tasks\\n"
    "\\x07\\c16• [X]\\x14\\c07Done tasks\\n\\n\\c16"
    "\\aC──────────────────────\\n"
    "\\c14\\aC{M}  VIM-LIKE EDITING\\n\\n\\c16"
    "\\x07\\c16• DEFAULT →\\x20\\c07manage\\n"
    "\\x07\\c16• NORMAL  →\\x20\\c07navigate\\n"
    "\\x07\\c16• INSERT  →\\x20\\c07type\\n"
    "\\x07\\c16• VISUAL  →\\x20\\c07select\\n\\n"
    "\\x03\\c15[n/t]\\x09\\c07add\\x16\\c15[u/Ctrl+r]\\x27\\c07undo/redo\\n"
    "\\x03\\c15[e]\\x09\\c07edit\\x16\\c15[V]\\x27\\c07move",
    "\\n\\n\\n\\n\\c11\\aCHIERARCHICAL NOTES\\n\\n\\c16"
    "\\x07\\c16* -\\x12\\c07Plain notes\\n"
    "\\x07\\c16* [ ]\\x14\\c07Undone tasks\\n"
    "\\x07\\c16* [X]\\x14\\c07Done tasks\\n\\n\\c16"
    "\\aC----------------------\\n"
    "\\c14\\aCVIM-LIKE EDITING\\n\\n\\c16"
    "\\x07\\c16* DEFAULT ->\\x21\\c07manage\\n"
    "\\x07\\c16* NORMAL  ->\\x21\\c07navigate\\n"
    "\\x07\\c16* INSERT  ->\\x21\\c07type\\n"
    "\\x07\\c16* VISUAL  ->\\x21\\c07select\\n\\n"
    "\\x03\\c15[n/t]\\x09\\c07add\\x16\\c15[u/Ctrl+r]\\x27\\c07undo/redo\\n"
    "\\x03\\c15[e]\\x09\\c07edit\\x16\\c15[V]\\x27\\c07move",
  };

  /* s4 – Finish */
  static const char* s4_texts[3] = {
    "\\n\\n\\n\\n\\c13\\aCReady to focus?\\n\\n"
    "\\aCStart your first cycle\\n"
    "\\aCand organize your work",
    "\\n\\n\\n\\n\\c13\\aCReady to focus?\\n\\n"
    "\\aCStart your first cycle\\n"
    "\\aCand organize your work",
    "\\n\\n\\n\\n\\c13\\aCReady to focus?\\n\\n"
    "\\aCStart your first cycle\\n"
    "\\aCand organize your work",
  };

  static const struct {
    const char* (*texts)[3];
    int h;
    int ctrl_set;
  } slideData[WELCOME_SLIDE_COUNT] = {
    {&s0_texts, 14, 0}, {&s1_texts, 19, 1}, {&s2_texts, 18, 1},
    {&s3_texts, 24, 1}, {&s4_texts, 12, 2},
  };

  const SlideProgress prog = {"Welcome", "●", "○", WELCOME_SLIDE_COUNT, 0};

  const ControlButton first_btns[] = {
    {"[Close]", ALIGN_SLIDE_CENTER, ClosePopup},
    {"Next  >", ALIGN_SLIDE_RIGHT, GoNextSlide},
  };
  const ControlButton mid_btns[] = {
    {"<  Prev", ALIGN_SLIDE_LEFT, GoPrevSlide},
    {"[Close]", ALIGN_SLIDE_CENTER, ClosePopup},
    {"Next  >", ALIGN_SLIDE_RIGHT, GoNextSlide},
  };
  const ControlButton last_btns[] = {
    {"[ Get Started ]", ALIGN_SLIDE_CENTER, ClosePopup},
  };

  int total = 3 * WELCOME_SLIDE_COUNT;
  SlideDef** slides = (SlideDef**)calloc(total, sizeof(SlideDef*));
  if (!slides) return NULL;

  for (int ic = 0; ic < 3; ic++) {
    for (int si = 0; si < WELCOME_SLIDE_COUNT; si++) {
      int idx = ic * WELCOME_SLIDE_COUNT + si;
      const char* text = (*slideData[si].texts)[ic];
      slides[idx] = buildSlideFromText(text, SLIDE_W, slideData[si].h, ic);
      if (!slides[idx]) {
        FreeWelcomeSlides(slides, total);
        return NULL;
      }
      slides[idx]->render_progress = SlideProgressRender;
      slides[idx]->progress = slideProgressDup(&prog);
      slides[idx]->render_controls = SlideControlsRender;
      switch (slideData[si].ctrl_set) {
        case 0:
          slides[idx]->controls = slideControlsDup(first_btns, 2);
          break;
        case 1:
          slides[idx]->controls = slideControlsDup(mid_btns, 3);
          break;
        case 2:
          slides[idx]->controls = slideControlsDup(last_btns, 1);
          break;
      }
      slides[idx]->slide_type = SLIDE_TYPE_WELCOME;
    }
  }

  return slides;
}

/**
 * Free all memory associated with a welcome slides array.
 * Walks and frees each slide's token linked list, then the slide array.
 * @param slides Array of SlideDef pointers to free
 * @param count Number of elements in the array
 */
void FreeWelcomeSlides(SlideDef** slides, int count) {
  if (!slides) return;
  for (int i = 0; i < count; i++) {
    if (slides[i]) {
      SlideToken* t = slides[i]->tokens;
      while (t) {
        SlideToken* next = t->next;
        free(t->text);
        free(t);
        t = next;
      }
      free(slides[i]->progress);
      if (slides[i]->controls) {
        free(slides[i]->controls->buttons);
        free(slides[i]->controls);
      }
      free(slides[i]);
    }
  }
  free(slides);
}

/**
 * Continue the session then close the dialog.
 * @param app Application state
 */
static void continueAndClose(AppData* app) {
  ContinuePreviousSession(app);
  ClosePopup(app);
}

/**
 * Abandon the session then close the dialog.
 * @param app Application state
 */
static void abandonAndClose(AppData* app) {
  AbandonPreviousSession(app);
  ClosePopup(app);
}

/**
 * Continue-dialog progress renderer: shows "{W} Resume Pomodoro Session"
 * centred in the title bar, with the icon expanded per icon type.
 * @param app    Application state (for icon type)
 * @param x      Absolute column position (slide left edge)
 * @param y      Absolute row position (progress line)
 * @param w      Slide width (unused)
 * @param def    Slide definition (unused)
 * @param params Unused (no dots needed)
 */
static void continueProgressRender(AppData* app, int x, int y, int w,
                                   SlideDef* def, SlideProgress* params) {
  (void)app;
  (void)def;
  (void)params;
  int icon_type = GetConfigIconType();
  const char* icon = WORK_ICONS[icon_type];
  char title[64];
  snprintf(title, sizeof(title), "%s Resume Pomodoro Session",
           (icon && icon[0]) ? icon : "");
  int tw = utf8DisplayWidth(title);
  SetColor(COLOR_MAGENTA, NO_COLOR, A_BOLD);
  mvprintw(y, x + 1 + ((w - 2) - tw) / 2, "%s", title);
}

/**
 * Build a set of continue session slides (one per icon type).
 * Reads current session data from app->pomodoro_data and
 * formats it into token-format text with escape sequences.
 * @param app Application state with loaded pomodoro session data
 * @return Array of 3 SlideDef pointers (one per icon type), or NULL on failure
 */
SlideDef** BuildContinueSlides(AppData* app) {
  PomodoroData* pd = &app->pomodoro_data;

  const char* step_name;
  switch (pd->current_step) {
    case WORK_TIME:
      step_name = "Work";
      break;
    case SHORT_PAUSE:
      step_name = "Short Break";
      break;
    case LONG_PAUSE:
      step_name = "Long Pause";
      break;
    default:
      step_name = "Unknown";
      break;
  }

  int icon_type = GetConfigIconType();
  const char* filled = ACTIVE_PROGRESS_BAR_ICONS[icon_type];
  const char* empty_ch = INACTIVE_PROGRESS_BAR_ICONS[icon_type];

  /* Total session remaining + progress across all cycles */
  int w = pd->work_time, s = pd->short_pause_time, l = pd->long_pause_time;
  int total_session_sec = ((pd->total_cycles - 1) * (w + s) + w + l) * 60;
  int completed_sec = 0;
  for (int c = 0; c < pd->current_cycle; c++) completed_sec += (w + s) * 60;
  if (pd->current_step == SHORT_PAUSE)
    completed_sec += w * 60;
  else if (pd->current_step == LONG_PAUSE)
    completed_sec += (w + s) * 60;
  completed_sec += pd->current_step_time;

  int total_remaining = total_session_sec - completed_sec;
  if (total_remaining < 0) total_remaining = 0;
  int tr_m = total_remaining / 60;
  int tr_s = total_remaining % 60;
  int total_pct =
    (total_session_sec > 0) ? (completed_sec * 100) / total_session_sec : 0;
  int total_bar_filled =
    (total_session_sec > 0) ? (completed_sec * 10) / total_session_sec : 0;

  char total_bar[64];
  char* bp = total_bar;
  for (int i = 0; i < 10; i++)
    bp += sprintf(bp, "%s", i < total_bar_filled ? filled : empty_ch);
  *bp = '\0';

  char started_str[64] = "--";
  if (pd->session_start_time > 0) {
    struct tm* tm_info = localtime(&pd->session_start_time);
    strftime(started_str, sizeof(started_str), "%d/%m/%y %H:%M", tm_info);
  }

  char text[512];
  snprintf(text, sizeof(text),
           "\\n\\n\\n\\n"
           "\\aCAn unfinished focus session was\\n"
           "\\aCdetected from your previous run\\n"
           "\\aC\\n"
           "\\aL\\x03─ Session Details ─\\n"
           "\\aC\\n"
           "\\aL\\x03Phase\\x16\\c07%s %02d/%02d\\n"
           "\\x03Remaining\\x16\\c07%02d:%02d\\n"
           "\\x03Progress\\x16\\c07%s %d%%\\n"
           "\\x03Started\\x16\\c07%s\\n"
           "\\aC\\n"
           "\\aL\\x03Continue where you left off?\\n",
           step_name, pd->current_cycle + 1, pd->total_cycles, tr_m, tr_s,
           total_bar, total_pct, started_str);

  int total = 3;
  SlideDef** slides = (SlideDef**)calloc(total, sizeof(SlideDef*));
  if (!slides) return NULL;

  for (int ic = 0; ic < 3; ic++) {
    slides[ic] = buildSlideFromText(text, 39, 19, ic);
    if (!slides[ic]) {
      FreeWelcomeSlides(slides, total);
      return NULL;
    }
    slides[ic]->slide_type = SLIDE_TYPE_CONTINUE;
    slides[ic]->render_controls = SlideControlsRender;
    slides[ic]->render_progress = continueProgressRender;
    {
      const SlideProgress dummy = {"", "", "", 1, 0};
      slides[ic]->progress = slideProgressDup(&dummy);
    }
    const ControlButton btns[] = {
      {"[ Continue ]", ALIGN_SLIDE_LEFT, continueAndClose},
      {"[ Discard ]", ALIGN_SLIDE_RIGHT, abandonAndClose},
    };
    slides[ic]->controls = slideControlsDup(btns, 2);
  }

  return slides;
}

/**
 * Default progress renderer for slides.
 * Draws the title with filled/empty dots ("Welcome ●●○○○") in
 * UTF-8/nerd mode, or as a counter ("Welcome 3/5") in ASCII mode.
 * Current slide index is obtained from app->popup_dialog->currentSlide.
 * @param app    Application state
 * @param x      Absolute column position (slide left edge)
 * @param y      Absolute row position (progress line)
 * @param w      Slide width
 * @param def    Slide definition (unused)
 * @param params Progress parameters (title, icons, total)
 */
void SlideProgressRender(AppData* app, int x, int y, int w, SlideDef* def,
                         SlideProgress* params) {
  (void)def;
  (void)w;
  int slide_idx = app->popup_dialog->currentSlide;
  int icon_type = GetConfigIconType();
  char buf[64];

  if (icon_type == ASCII || !params->icon_on[0] || !params->icon_off[0])
    snprintf(buf, sizeof(buf), "%s %d/%d", params->title, slide_idx + 1,
             params->total);
  else {
    char* p = buf;
    p += sprintf(p, "%s ", params->title);
    for (int i = 0; i < params->total; i++)
      p +=
        sprintf(p, "%s", i <= slide_idx ? params->icon_on : params->icon_off);
    *p = '\0';
  }
  int dot_w = utf8DisplayWidth(buf);
  mvprintw(y, x + 1 + (SLIDE_INNER_W - dot_w) / 2, "%s", buf);
}

/**
 * Default controls renderer for slides.
 * Positions each button by its align field:
 *   LEFT   → x + 2
 *   CENTER → centered within SLIDE_INNER_W
 *   RIGHT  → x + SLIDE_W - 2 - text_width
 * Draws with A_REVERSE when def->hovered matches the button index,
 * and registers a REGION_WELCOME_NAV click region with the action.
 * @param app    Application state
 * @param x      Absolute column position (slide left edge)
 * @param y      Absolute row position (controls line)
 * @param w      Slide width
 * @param def    Slide definition (provides hovered index)
 * @param params Controls parameters (button array + count)
 */
void SlideControlsRender(AppData* app, int x, int y, int w, SlideDef* def,
                         SlideControls* params) {
  for (int i = 0; i < params->count; i++) {
    const ControlButton* btn = &params->buttons[i];
    int tw = utf8DisplayWidth(btn->text);
    int bx;
    switch (btn->align) {
      case ALIGN_SLIDE_LEFT:
        bx = x + 2;
        break;
      case ALIGN_SLIDE_CENTER:
        bx = x + 1 + ((w - 2) - tw) / 2;
        break;
      case ALIGN_SLIDE_RIGHT:
        bx = x + w - 2 - tw;
        break;
      default:
        bx = x + 2;
        break;
    }
    if (def->hovered == i) attron(A_REVERSE);
    mvprintw(y, bx, "%s", btn->text);
    if (def->hovered == i) attroff(A_REVERSE);
    RegisterClickRegion(app, bx, y, tw, 1, REGION_WELCOME_NAV,
                        (MenuAction)btn->action, -1, i, 0);
  }
}

/**
 * Measure the total display width of a UTF-8 string.
 * Icons (4-byte sequences) count as 2 columns; everything else counts as 1.
 * @param s The UTF-8 input string
 * @return Display width in terminal columns
 */
static int strDisplayWidth(const char* s) {
  int total = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) {
      s++;
      total++;
    } else if (c < 0xC0) {
      s++;
    } else {
      int len = (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
      total += (c >= 0xF0) ? 2 : 1;
      s += len;
    }
  }
  return total;
}

/**
 * Count the number of visible characters in a UTF-8 string.
 * Each multi-byte sequence counts as a single character regardless of byte width.
 * @param s The UTF-8 input string
 * @return Number of visible characters
 */
static int utf8DisplayWidth(const char* s) {
  int w = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) {
      s++;
      w++;
    } else if (c < 0xC0) {
      s++;
    } else {
      int len = (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
      s += len;
      w++;
    }
  }
  return w;
}

/**
 * Parse a slide text string with escape sequences into a linked list of
 * SlideToken structs.
 *
 * Supported sequences:
 *   \\cN    – Set color (0-15; 0-7 no bold, 8-15 bold, ≥16 = NO_COLOR)
 *   \\aL/C/R – Set alignment (LEFT, CENTER, RIGHT)
 *   \\xNN   – Set absolute column offset (decimal digits)
 *   \\n     – Newline (emit buffer, y++, reset color/align/x)
 *   {W}{S}{P}{N}{M} – Expand icon placeholder for current icon_type
 *   \\\\    – Literal backslash
 *
 * Returns the head of the allocated linked list, or NULL on allocation failure.
 * The caller (buildSlideFromText) owns the list.
 *
 * @param text      Raw slide text with escape sequences
 * @param icon_type Icon type index (0=nerd, 1=emoji, 2=ascii)
 * @return Head of SlideToken linked list, or NULL on failure
 */
static SlideToken* parseSlideText(const char* text, int icon_type) {
  if (!text || !*text) return NULL;

  SlideToken head = {0, NULL, NO_COLOR, ALIGN_SLIDE_LEFT, 0, NULL};
  SlideToken* tail = &head;
  int y = 0;
  int color = NO_COLOR;
  SlideAlign align = ALIGN_SLIDE_LEFT;
  int x = 0;
  char buf[1024];
  int bufpos = 0;
  buf[0] = '\0';

  while (*text) {
    if (*text == '\\') {
      text++;
      if (!*text) break;

      switch (*text) {
        case 'n': {
          if (bufpos > 0) {
            buf[bufpos] = '\0';
            SlideToken* t = (SlideToken*)calloc(1, sizeof(SlideToken));
            if (!t) return NULL;
            t->y = y;
            t->text = strdup(buf);
            t->color = color;
            t->align = align;
            t->x = x;
            tail->next = t;
            tail = t;
            bufpos = 0;
            buf[0] = '\0';
          }
          y++;
          color = NO_COLOR;
          align = ALIGN_SLIDE_LEFT;
          x = 0;
          text++;
          break;
        }
        case 'c': {
          if (bufpos > 0) {
            buf[bufpos] = '\0';
            SlideToken* t = (SlideToken*)calloc(1, sizeof(SlideToken));
            if (!t) return NULL;
            t->y = y;
            t->text = strdup(buf);
            t->color = color;
            t->align = align;
            t->x = x;
            tail->next = t;
            tail = t;
            bufpos = 0;
            buf[0] = '\0';
          }
          text++;
          if (*text >= '0' && *text <= '9') {
            color = (*text - '0') * 10;
            text++;
            if (*text >= '0' && *text <= '9') {
              color += (*text - '0');
              text++;
            }
            if (color >= 16) color = NO_COLOR;
          } else {
            color = NO_COLOR;
          }
          break;
        }
        case 'a': {
          if (bufpos > 0) {
            buf[bufpos] = '\0';
            SlideToken* t = (SlideToken*)calloc(1, sizeof(SlideToken));
            if (!t) return NULL;
            t->y = y;
            t->text = strdup(buf);
            t->color = color;
            t->align = align;
            t->x = x;
            tail->next = t;
            tail = t;
            bufpos = 0;
            buf[0] = '\0';
          }
          text++;
          if (*text == 'L')
            align = ALIGN_SLIDE_LEFT;
          else if (*text == 'C')
            align = ALIGN_SLIDE_CENTER;
          else if (*text == 'R')
            align = ALIGN_SLIDE_RIGHT;
          if (*text) text++;
          break;
        }
        case 'x': {
          if (bufpos > 0) {
            buf[bufpos] = '\0';
            SlideToken* t = (SlideToken*)calloc(1, sizeof(SlideToken));
            if (!t) return NULL;
            t->y = y;
            t->text = strdup(buf);
            t->color = color;
            t->align = align;
            t->x = x;
            tail->next = t;
            tail = t;
            bufpos = 0;
            buf[0] = '\0';
          }
          text++;
          if (*text >= '0' && *text <= '9') {
            x = (*text - '0') * 10;
            text++;
            if (*text >= '0' && *text <= '9') {
              x += (*text - '0');
              text++;
            }
          }
          break;
        }
        case '\\': {
          if (bufpos < (int)sizeof(buf) - 4) buf[bufpos++] = '\\';
          text++;
          break;
        }
        default: {
          if (bufpos < (int)sizeof(buf) - 4) buf[bufpos++] = *text;
          text++;
          break;
        }
      }
    } else if (*text == '{') {
      text++;
      if (!*text) break;

      const char* icon = NULL;
      if (*text == 'W')
        icon = WORK_ICONS[icon_type];
      else if (*text == 'S')
        icon = SKIP_ICONS[icon_type];
      else if (*text == 'P')
        icon = PAUSE_ICONS[icon_type];
      else if (*text == 'N')
        icon = NOTES_ICONS[icon_type];
      else if (*text == 'M')
        icon = DEFAULT_MODE_ICONS[icon_type];

      text++;
      if (*text == '}') text++;

      if (icon) {
        if (!*icon) {
          if (bufpos < (int)sizeof(buf) - 2) buf[bufpos++] = ' ';
        } else {
          int slen = (int)strlen(icon);
          if (bufpos + slen < (int)sizeof(buf) - 1) {
            memcpy(buf + bufpos, icon, slen);
            bufpos += slen;
          }
        }
      }
    } else {
      if (bufpos < (int)sizeof(buf) - 4) buf[bufpos++] = *text;
      text++;
    }
  }

  if (bufpos > 0) {
    buf[bufpos] = '\0';
    SlideToken* t = (SlideToken*)calloc(1, sizeof(SlideToken));
    if (!t) return NULL;
    t->y = y;
    t->text = strdup(buf);
    t->color = color;
    t->align = align;
    t->x = x;
    t->next = NULL;
    tail->next = t;
  }

  return head.next;
}

/**
 * Build a SlideDef from a single pre-selected text string for a given
 * icon type. Parses escape sequences via parseSlideText.
 * @param text      Slide text with escape sequences (already resolved for icon_type)
 * @param w         Slide width in columns
 * @param h         Slide height in rows
 * @param icon_type Icon type index for placeholder expansion
 * @return Pointer to allocated SlideDef, or NULL on failure
 */
static SlideDef* buildSlideFromText(const char* text, int w, int h,
                                    int icon_type) {
  SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
  if (!def) return NULL;

  def->tokens = parseSlideText(text, icon_type);
  if (!def->tokens) {
    free(def);
    return NULL;
  }

  def->size.width = w;
  def->size.height = h;
  def->render = welcomeRender;
  def->update = welcomeUpdate;
  def->hovered = -1;

  return def;
}

/**
 * Render the content-area tokens between the horizontal rules of the dialog.
 * Fills each row with spaces and vertical-side characters, then draws each
 * token from the linked list at its computed position.
 * @param x      X position of the dialog
 * @param y      Y position of the dialog
 * @param w      Width of the dialog
 * @param h      Height of the dialog
 * @param tokens Head of SlideToken linked list
 */
static void renderSlideTokens(int x, int y, int w, int h, SlideToken* tokens) {
  for (int row = y + 3; row <= y + h - 4; row++) {
    mvaddch(row, x, ACS_VLINE);
    for (int ci = 1; ci < w - 1; ci++) mvaddch(row, x + ci, ' ');
    mvaddch(row, x + w - 1, ACS_VLINE);
  }

  for (SlideToken* t = tokens; t; t = t->next) {
    if (!t->text || !*(t->text)) continue;
    int display_w = strDisplayWidth(t->text);
    int line_x;
    if (t->x > 0)
      line_x = x + 1 + t->x;
    else
      line_x =
        x + 1 +
        ((t->align == ALIGN_SLIDE_CENTER) ? ((w - 2) - display_w) / 2 : 0);

    if (t->color < 0 || t->color >= 16)
      SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    else if (t->color >= 8)
      SetColor(t->color - 8, NO_COLOR, A_BOLD);
    else
      SetColor(t->color, NO_COLOR, A_NORMAL);

    mvprintw(y + t->y, line_x, "%s", t->text);
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  }
}

/**
 * Render the current welcome slide into the popup dialog.
 * Draws the box frame, progress indicator, content tokens, and nav controls.
 * @param app Application state (provides popup_dialog, screen)
 * @param def Slide definition to render
 */
static void welcomeRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  int w = def->size.width;
  int h = def->size.height;

  d->size.width = w;
  d->size.height = h;
  UpdateFloatingDialog(d, app->screen);

  int x = d->position.x;
  int y = d->position.y;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) mvprintw(y + r, x + c, " ");

  /* Sync keyboard-hovered button index into the slide def so
   * SlideControlsRender highlights the right button.
   * Only override when keyboard has made a selection (>=0)
   * so mouse hover set by welcomeUpdate is not cleared for WELCOME slides. */
  if (d->hovered_button >= 0) def->hovered = d->hovered_button;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  renderSlideBox(x, y, w, h);
  if (def->render_progress && def->progress)
    def->render_progress(app, x, y + 1, w, def, def->progress);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  renderSlideTokens(x, y, w, h, def->tokens);
  if (def->render_controls && def->controls)
    def->render_controls(app, x, y + h - 2, w, def, def->controls);
}

/**
 * Track mouse hover over the nav controls (prev/next/close/start) on the
 * current slide and update def->hovered accordingly.
 * @param app Application state (provides click_regions, debug_mouse coordinates)
 * @param def Slide definition whose hovered field to update
 */
static void welcomeUpdate(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int x = d->position.x;
  int y = d->position.y + d->size.height - 2;
  int w = def->size.width;
  int hover_idx = -1;
  if (def->controls) {
    for (int i = 0; i < def->controls->count; i++) {
      const ControlButton* btn = &def->controls->buttons[i];
      int tw = utf8DisplayWidth(btn->text);
      int bx;
      switch (btn->align) {
        case ALIGN_SLIDE_LEFT:
          bx = x + 2;
          break;
        case ALIGN_SLIDE_CENTER:
          bx = x + 1 + ((w - 2) - tw) / 2;
          break;
        case ALIGN_SLIDE_RIGHT:
          bx = x + w - 2 - tw;
          break;
        default:
          bx = x + 2;
          break;
      }
      if (app->debug_mouse_x >= bx && app->debug_mouse_x < bx + tw &&
          app->debug_mouse_y == y) {
        hover_idx = i;
        break;
      }
    }
  }
  /* Only update hovered when a fresh mouse position is available.
   * debug_mouse_x/y are reset to -1 on non-mouse input cycles, so
   * skipping the update when they are stale preserves the highlight */
  if (app->debug_mouse_x >= 0) {
    def->hovered = hover_idx;
    if (d->slide_type == SLIDE_TYPE_CONTINUE ||
        d->slide_type == SLIDE_TYPE_WELCOME)
      d->hovered_button = hover_idx;
  }
}

/**
 * Draw the dialog frame with ACS line-drawing characters.
 * The frame has a top rule, middle rule (at y+2), bottom rule, and vertical sides.
 * @param x X position of the dialog
 * @param y Y position of the dialog
 * @param w Width of the dialog
 * @param h Height of the dialog
 */
static void renderSlideBox(int x, int y, int w, int h) {
  mvaddch(y, x, ACS_ULCORNER);
  for (int i = 1; i < w - 1; i++) mvaddch(y, x + i, ACS_HLINE);
  mvaddch(y, x + w - 1, ACS_URCORNER);

  mvaddch(y + h - 3, x, ACS_LTEE);
  for (int i = 1; i < w - 1; i++) mvaddch(y + h - 3, x + i, ACS_HLINE);
  mvaddch(y + h - 3, x + w - 1, ACS_RTEE);

  mvaddch(y + h - 1, x, ACS_LLCORNER);
  for (int i = 1; i < w - 1; i++) mvaddch(y + h - 1, x + i, ACS_HLINE);
  mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);

  mvaddch(y + 2, x, ACS_LTEE);
  for (int i = 1; i < w - 1; i++) mvaddch(y + 2, x + i, ACS_HLINE);
  mvaddch(y + 2, x + w - 1, ACS_RTEE);

  for (int i = 0; i < h; i++) {
    if (i == 0 || i == 2 || i == h - 3 || i == h - 1) continue;
    mvaddch(y + i, x, ACS_VLINE);
    mvaddch(y + i, x + w - 1, ACS_VLINE);
  }
}

/**
 * Duplicate a SlideProgress struct onto the heap.
 * @param p Source progress parameters
 * @return Heap-allocated copy, or NULL on failure
 */
static SlideProgress* slideProgressDup(const SlideProgress* p) {
  SlideProgress* d = (SlideProgress*)malloc(sizeof(SlideProgress));
  if (d) *d = *p;
  return d;
}

/**
 * Duplicate a SlideControls struct (including buttons array) onto the heap.
 * @param btns Source buttons array
 * @param count Number of buttons
 * @return Heap-allocated SlideControls with copied buttons, or NULL on failure
 */
static SlideControls* slideControlsDup(const ControlButton* btns, int count) {
  SlideControls* c = (SlideControls*)malloc(sizeof(SlideControls));
  if (!c) return NULL;
  c->buttons = (ControlButton*)malloc(count * sizeof(ControlButton));
  if (!c->buttons) {
    free(c);
    return NULL;
  }
  memcpy(c->buttons, btns, count * sizeof(ControlButton));
  c->count = count;
  return c;
}
