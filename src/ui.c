#include "ui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "config.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "log.h"
#include "notify.h"
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
static void continueProgressRender(AppData* app, int x, int y, int w,
                                   SlideDef* def, SlideProgress* params);
static void continueAndClose(AppData* app);
static void abandonAndClose(AppData* app);
static void noiseSlideRender(AppData* app, SlideDef* def);
static void noiseSlideUpdate(AppData* app, SlideDef* def);
static SlideToken* parseSlideText(const char* text, int icon_type);
static SlideDef* buildSlideFromText(const char* text, int w, int h,
                                    int icon_type);
static void renderSlideTokens(int x, int y, int w, int h, SlideToken* tokens);
static SlideProgress* slideProgressDup(const SlideProgress* p);
static SlideControls* slideControlsDup(const ControlButton* btns, int count);
static int strDisplayWidth(const char* s);
static int utf8DisplayWidth(const char* s);
/* History */
static int computeGraphLayout(int firstYear, int firstMonth, int* monthWeeks,
                              int* monthStartWeek, int* monthDays,
                              int* monthStartDow);
static int weekDowToDay(int weekCol, int dow, int firstYear, int firstMonth,
                        int* monthWeeks, int* monthStartWeek, int* monthDays,
                        int* monthStartDow, int* outYear, int* outMonth);
static void historyOverviewRender(AppData* app, SlideDef* def);
static void historyOverviewUpdate(AppData* app, SlideDef* def);
static int computeMonthWeeks(int year, int month, int startDow);

/* Preferences */
static void setPrefInt(AppData* app, int idx, int val);
static int selectablePrefCount(AppData* app);
static void prefsSlideRender(AppData* app, SlideDef* def);
static void prefsSlideUpdate(AppData* app, SlideDef* def);
static void prefsStepperRender(AppData* app, SlideDef* def);
static void prefsSelectRender(AppData* app, SlideDef* def);
static void prefsSelectUpdate(AppData* app, SlideDef* def);

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
  Dimensions size = {.width = 41, .height = -1};
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
    dialog->slides = BuildWelcomeSlides(size);
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
  Dimensions size = {.width = 51, .height = 19};
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
    dialog->slides = BuildContinueSlides(app, size);
    if (!dialog->slides) {
      FreeFloatingDialog(dialog);
      return NULL;
    }
    dialog->slideCount = 3;
    dialog->currentSlide = 0;
  }
  return dialog;
}

/**
 * Create a white noise control dialog for ambient sounds.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateNoiseDialog(AppData* app) {
  Dimensions size = {.width = 51,
                     .height = -1}; /* height grows with track count */
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Close", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 1};
  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (dialog) {
    dialog->slides = BuildNoiseSlides(app, size);
    if (!dialog->slides) {
      FreeFloatingDialog(dialog);
      return NULL;
    }
    dialog->slideCount = 1;
    dialog->currentSlide = 0;
    dialog->slide_type = SLIDE_TYPE_NOISE;
    dialog->hovered_button = -1;
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
  const char* pause_icon =
    app->is_paused ? PLAY_ICONS[icon_type] : PAUSE_ICONS[icon_type];
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

  SetColor(color, NO_COLOR, A_BOLD);
  mvprintw(pos.y, pos.x + skip_length, "%s ", pause_icon);
}

/**
 * ---------------------------------------------------------------------------
 * Slides
 * ---------------------------------------------------------------------------
 */

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
 * Build all welcome slide definitions for all icon types.
 * Allocates 3 * stride SlideDef instances, one per icon
 * type per slide. Index with [iconType * stride + slideIdx].
 * @param size Slide dimensions: width=41, height=-1 to use per-slide height
 * @return Pointer to array of SlideDef pointers, or NULL on allocation failure
 */
SlideDef** BuildWelcomeSlides(Dimensions size) {
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

  const SlideProgress prog = {"WELCOME", "●", "○", WELCOME_SLIDE_COUNT, 0};

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
      int h = size.height >= 0 ? size.height : slideData[si].h;
      slides[idx] = buildSlideFromText(text, size.width, h, ic);
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
  RenderSlideBox(x, y, w, h);
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
 * @param app Application state (provides click_regions, mouse coordinates)
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
      if (app->mouse_x >= bx && app->mouse_x < bx + tw && app->mouse_y == y) {
        hover_idx = i;
        break;
      }
    }
  }
  /* Update hovered_button when mouse is over a button */
  if (hover_idx >= 0) {
    def->hovered = hover_idx;
    d->hovered_button = hover_idx;
  }
  /* When mouse is not over any button, preserve existing keyboard-set
   * hovered values (do not reset to -1). */
}

/**
 * Build a set of continue session slides (one per icon type).
 * Reads current session data from app->pomodoro_data and
 * formats it into token-format text with escape sequences.
 * @param app Application state with loaded pomodoro session data
 * @param size Slide dimensions (width=39, height=19)
 * @return Array of 3 SlideDef pointers (one per icon type), or NULL on failure
 */
SlideDef** BuildContinueSlides(AppData* app, Dimensions size) {
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
    slides[ic] = buildSlideFromText(text, size.width, size.height, ic);
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
  snprintf(title, sizeof(title), "%s RESUME POMODORO SESSION",
           (icon && icon[0]) ? icon : "");
  int tw = utf8DisplayWidth(title);
  SetColor(COLOR_MAGENTA, NO_COLOR, A_BOLD);
  mvprintw(y, x + 1 + ((w - 2) - tw) / 2, "%s", title);
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
 * Build an array of white noise slides (one slide).
 * Height is computed dynamically from the registered track count
 * as h = 15 + 2 * track_count (minimum 15).
 * @param app  Application state
 * @param size Slide dimensions (width used; height recomputed)
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildNoiseSlides(AppData* app, Dimensions size) {
  int track_count = app->noise_data.track_count;
  int h = (track_count == 0) ? 15 : 15 + 2 * track_count;

  SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
  if (!slides) return NULL;

  SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
  if (!def) {
    free(slides);
    return NULL;
  }

  def->size.width = size.width;
  def->size.height = h;
  def->render = noiseSlideRender;
  def->update = noiseSlideUpdate;
  def->slide_type = SLIDE_TYPE_NOISE;
  def->hovered = -1;
  {
    SlideProgress prog = {"WHITE NOISE", " ", " ", 1, 0};
    def->progress = slideProgressDup(&prog);
  }
  def->render_progress = SlideProgressRender;
  slides[0] = def;
  return slides;
}

/**
 * Handle mouse click and scroll events on the white noise control slide.
 *
 * Determines which track row the event falls on (y + 8 + i*2 for tracks,
 * y + 18 for master), then for clicks divides the row horizontally:
 * left of the \x17 (minus) column toggles play/pause, the minus icon
 * zone decreases volume, and the plus icon zone increases it.  Scroll
 * events (BUTTON4/BUTTON5) adjust the volume of the targeted row by ±10.
 *
 * @param app     Pointer to the application data
 * @param event   The ncurses mouse event
 * @param is_click True if this is a click (BUTTON1_PRESSED), false for scroll
 */
void NoiseSlideMouseAction(AppData* app, MEVENT* event, bool is_click) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  WhiteNoiseData* data = &app->noise_data;
  int track_count = data->track_count;
  int master_row_y = d->position.y + 10 + track_count * 2;

  int x = d->position.x;
  int y = d->position.y;

  int row = -1;
  for (int i = 0; i < track_count; i++) {
    if (event->y == y + 8 + i * 2) {
      row = i;
      break;
    }
  }
  if (row < 0 && event->y == master_row_y) row = track_count;
  if (row < 0) return;

  data->selected = row;

  if (is_click) {
    int icon_type = GetConfigIconType();
    int minus_w = utf8DisplayWidth(MINUS_VOLUME_ICONS[icon_type]);
    int plus_w = utf8DisplayWidth(PLUS_VOLUME_ICONS[icon_type]);
    int bar_w = (row == track_count) ? 24 : 19;

    int content_x = x + 1;
    int minus_col = content_x + ((row == track_count) ? 12 : 17);
    int bar_start = minus_col + minus_w + 1;
    int plus_col = bar_start + bar_w + 1;
    int plus_end = plus_col + plus_w - 1;

    if (event->x >= minus_col && event->x < bar_start)
      NoiseVolumeDown(app);
    else if (event->x >= plus_col && event->x <= plus_end)
      NoiseVolumeUp(app);
    else if (event->x < minus_col && row != track_count)
      NoiseTogglePlay(app);
  } else {
    if (event->bstate & BUTTON4_PRESSED)
      NoiseVolumeUp(app);
    else if (event->bstate & BUTTON5_PRESSED)
      NoiseVolumeDown(app);
  }
}

/**
 * Render the white noise control slide.
 *
 * Builds the text string dynamically every frame from the registered
 * track list, injecting live volume-bar and playing-indicator strings,
 * then parses via parseSlideText, overrides token colours for selection
 * highlighting, and draws via renderSlideTokens. Keyboard hints are drawn
 * directly at the bottom.
 *
 * @param app Pointer to the application data
 * @param def Slide definition (provides size, progress, callbacks)
 */
static void noiseSlideRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  int w = def->size.width;
  int h = def->size.height;
  d->size.width = w;
  d->size.height = h;
  UpdateFloatingDialog(d, app->screen);
  int x = d->position.x;
  int y = d->position.y;
  int icon_type = GetConfigIconType();
  WhiteNoiseData* data = &app->noise_data;
  int track_count = data->track_count;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) mvprintw(y + r, x + c, " ");

  if (d->hovered_button >= 0) def->hovered = d->hovered_button;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  RenderSlideBox(x, y, w, h);

  if (def->render_progress && def->progress)
    def->render_progress(app, x, y + 1, w, def, def->progress);

  char play_strs[track_count][8];
  for (int i = 0; i < track_count; i++) {
    if (data->playing[i]) {
      const char* raw = PLAYING_ICONS[icon_type];
      if (utf8DisplayWidth(raw) >= 2)
        snprintf(play_strs[i], sizeof(play_strs[i]), "%s", raw);
      else
        snprintf(play_strs[i], sizeof(play_strs[i]), "%s ", raw);
    } else
      snprintf(play_strs[i], sizeof(play_strs[i]), "  ");
  }

  char vol_bars[track_count + 1][128];
  for (int i = 0; i < track_count; i++) {
    int bar_w = 19;
    int filled = (data->volume[i] * bar_w) / 100;
    int empty = bar_w - filled;
    char* bp = vol_bars[i];
    for (int j = 0; j < filled; j++)
      bp += sprintf(bp, "%s", ACTIVE_VOLUME_BAR_ICONS[icon_type]);
    for (int j = 0; j < empty; j++)
      bp += sprintf(bp, "%s", INACTIVE_VOLUME_BAR_ICONS[icon_type]);
    *bp = '\0';
  }
  {
    int bar_w = 24;
    int filled = (data->master_volume * bar_w) / 100;
    int empty = bar_w - filled;
    char* bp = vol_bars[track_count];
    for (int j = 0; j < filled; j++)
      bp += sprintf(bp, "%s", ACTIVE_VOLUME_BAR_ICONS[icon_type]);
    for (int j = 0; j < empty; j++)
      bp += sprintf(bp, "%s", INACTIVE_VOLUME_BAR_ICONS[icon_type]);
    *bp = '\0';
  }

  char text[4096];
  char* tp = text;
  int remaining = sizeof(text);
  int n;

  n = snprintf(tp, remaining,
               "\\n\\n\\n\\n"
               "\\aC\\c07Mix ambient sounds for focus and relaxation\\n"
               "\\n"
               "\\c07\\x03───────────────────────────────────────────\\n"
               "\\n");
  tp += n;
  remaining -= n;

  if (track_count == 0) {
    n = snprintf(tp, remaining,
                 "\\c15\\x03No ambient sound tracks registered.\\n"
                 "\\n"
                 "\\c07\\x03───────────────────────────────────────────\\n"
                 "\\n");
    tp += n;
    remaining -= n;
    goto done_text;
  }

  for (int i = 0; i < track_count; i++) {
    const char* icon = data->tracks[i].icons[icon_type];
    n = snprintf(tp, remaining,
                 "\\x03%s%s %s\\x17{m} %s {p}  %3d%%\\n"
                 "\\n",
                 play_strs[i], icon, data->tracks[i].name, vol_bars[i],
                 data->volume[i]);
    tp += n;
    remaining -= n;
  }

  n = snprintf(tp, remaining,
               "\\c07\\x03───────────────────────────────────────────\\n"
               "\\n");
  tp += n;
  remaining -= n;

  n = snprintf(tp, remaining,
               "\\x03Master\\x12{m} %s {p}  %3d%%\\n"
               "\\n",
               vol_bars[track_count], data->master_volume);

done_text:

  if (def->tokens) {
    SlideToken* t = def->tokens;
    while (t) {
      SlideToken* n = t->next;
      free(t->text);
      free(t);
      t = n;
    }
    def->tokens = NULL;
  }

  def->tokens = parseSlideText(text, icon_type);
  if (def->tokens) {
    if (track_count > 0) {
      int sep_row = 8 + track_count * 2;
      int master_row = 10 + track_count * 2;
      for (SlideToken* t = def->tokens; t; t = t->next) {
        int row = t->y;
        if (row >= 8 && row < sep_row && (row % 2 == 0)) {
          int ti = (row - 8) / 2;
          t->color = (ti == data->selected) ? data->tracks[ti].sel_color : 7;
        } else if (row == master_row)
          t->color =
            (data->selected == track_count) ? data->master_sel_color : 7;
      }
    }
    renderSlideTokens(x, y, w, h, def->tokens);

    SlideToken* t = def->tokens;
    while (t) {
      SlideToken* n = t->next;
      free(t->text);
      free(t);
      t = n;
    }
    def->tokens = NULL;
  }

  char hint_buf[128];
  if (track_count == 0)
    snprintf(hint_buf, sizeof(hint_buf), "\\c15q \\c07Close");
  else
    snprintf(hint_buf, sizeof(hint_buf),
             "\\c15k/j \\c07Navigate  \\c15h/l \\c07Volume  \\c15space "
             "\\c07Toggle  \\c15q \\c07Close");
  SlideToken* hint_toks = parseSlideText(hint_buf, icon_type);
  if (hint_toks) {
    int total_w = 0;
    for (SlideToken* t = hint_toks; t; t = t->next)
      if (t->text) total_w += strDisplayWidth(t->text);
    int cx = x + 1 + ((w - 2) - total_w) / 2;
    int cy = y + h - 2;
    int cx_save = cx;
    for (SlideToken* t = hint_toks; t; t = t->next) {
      if (!t->text || !*t->text) continue;
      int dw = strDisplayWidth(t->text);
      if (t->color < 0 || t->color >= 16)
        SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      else if (t->color >= 8)
        SetColor(t->color - 8, NO_COLOR, A_BOLD);
      else
        SetColor(t->color, NO_COLOR, A_NORMAL);
      mvprintw(cy, cx, "%s", t->text);
      cx += dw;
    }
    /* Mouse hover + click region for "q Close" (last 7 display cols) */
    int close_x = cx_save + total_w - 7;
    int close_w = 7;
    bool on_close = (app->mouse_y == cy && app->mouse_x >= close_x &&
                     app->mouse_x < close_x + close_w);
    if (on_close) {
      SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      attron(A_REVERSE);
      mvprintw(cy, close_x, "q Close");
      attroff(A_REVERSE);
    }
    RegisterClickRegion(app, close_x, cy, close_w, 1, REGION_SLIDE_NAV,
                        (MenuAction)NoiseClose, -1, 0, 0);
    SlideToken* t = hint_toks;
    while (t) {
      SlideToken* n = t->next;
      free(t->text);
      free(t);
      t = n;
    }
  }
}

/**
 * Update the selected noise track based on mouse position.
 *
 * Checks the mouse cursor against each track row (y + 8 + i*2)
 * and the master row (y + 10 + track_count*2), updating
 * data->selected and dialog->hovered_button on match.
 *
 * @param app Pointer to the application data
 * @param def Slide definition (unused — state lives in app->noise_data)
 */
static void noiseSlideUpdate(AppData* app, SlideDef* def) {
  (void)def;
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int x = d->position.x;
  int y = d->position.y;

  WhiteNoiseData* data = &app->noise_data;
  int track_count = data->track_count;
  int master_row = y + 10 + track_count * 2;

  int hovered = -1;
  for (int i = 0; i < track_count; i++) {
    int ry = y + 8 + i * 2;
    if (app->mouse_y == ry && app->mouse_x >= x + 2 &&
        app->mouse_x <= x + d->size.width - 3) {
      hovered = i;
      break;
    }
  }

  if (hovered < 0)
    if (app->mouse_y == master_row && app->mouse_x >= x + 2 &&
        app->mouse_x <= x + d->size.width - 3)
      hovered = track_count;

  if (hovered >= 0) {
    data->selected = hovered;
    d->hovered_button = hovered;
  }
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
  mvprintw(y, x + 1 + ((w - 2) - dot_w) / 2, "%s", buf);
}

/**
 * Default controls renderer for slides.
 * Positions each button by its align field:
 *   LEFT   → x + 2
 *   CENTER → centered within (w - 2)
 *   RIGHT  → x + w - 2 - text_width
 * Draws with A_REVERSE when def->hovered matches the button index,
 * and registers a REGION_SLIDE_NAV click region with the action.
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
    int hover = (def->hovered == i);
    const char* space = strchr(btn->text, ' ');
    int keyW = space ? (int)(space - btn->text) : tw;
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    if (hover) attron(A_REVERSE);
    mvprintw(y, bx, "%.*s", keyW, btn->text);
    if (hover) attroff(A_REVERSE);
    if (space) {
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      if (hover) attron(A_REVERSE);
      mvprintw(y, bx + keyW, "%s", space);
      if (hover) attroff(A_REVERSE);
    }
    RegisterClickRegion(app, bx, y, tw, 1, REGION_SLIDE_NAV,
                        (MenuAction)btn->action, -1, i, 0);
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
void RenderSlideBox(int x, int y, int w, int h) {
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
 * Parse a slide text string with escape sequences into a linked list of
 * SlideToken structs.
 *
 * Supported sequences:
 *   \\cNN   – Set color (0-15; 0-7 = normal, 8-15 = bold, >= 16 = NO_COLOR)
 *   \\aL/C/R – Set alignment (LEFT, CENTER, RIGHT)
 *   \\xNN   – Set absolute column offset (decimal 0-99)
 *   \\n     – Newline (flush buffer, y++, reset colour/align/x)
 *   {W}{S}{P}{N}{M} – Uppercase pomodoro/welcome icon placeholders
 *   {r}{f}{w}{t}    – Lowercase noise track icon placeholders
 *   {m}{p}          – Noise minus/plus volume icon placeholders
 *   \\\\    – Literal backslash
 *
 * The caller owns the returned linked list and must free each token's
 * text and the token itself.
 *
 * @param text      Raw slide text with escape sequences
 * @param icon_type Icon type index (0 = nerd, 1 = emoji, 2 = ascii)
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
      else if (*text == 'r')
        icon = RAIN_ICONS[icon_type];
      else if (*text == 'f')
        icon = FIRE_ICONS[icon_type];
      else if (*text == 'w')
        icon = WIND_ICONS[icon_type];
      else if (*text == 't')
        icon = THUNDER_ICONS[icon_type];
      else if (*text == 'm')
        icon = MINUS_VOLUME_ICONS[icon_type];
      else if (*text == 'p')
        icon = PLUS_VOLUME_ICONS[icon_type];

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

/**
 * ---------------------------------------------------------------------------
 * History Popup 
 * ---------------------------------------------------------------------------
 */

/**
 * Build a single history slide with custom render/update callbacks.
 * The slide is not token-based — the render function draws directly.
 * @param size Slide dimensions
 * @param render Custom render callback
 * @param update Custom update callback (may be NULL)
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildHistorySlide(Dimensions size,
                             void (*render)(AppData*, SlideDef*),
                             void (*update)(AppData*, SlideDef*)) {
  SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
  if (!slides) return NULL;

  SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
  if (!def) {
    free(slides);
    return NULL;
  }

  def->size = size;
  def->render = render;
  def->update = update;
  def->slide_type = SLIDE_TYPE_HISTORY_OVERVIEW;
  def->hovered = -1;
  slides[0] = def;
  return slides;
}

/**
 * Build a token-based history slide from formatted text.
 * @param text Token-format text with escape sequences
 * @param size Slide dimensions
 * @param slide_type Slide type identifier
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildHistoryTextSlide(const char* text, Dimensions size,
                                 int slide_type) {
  SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
  if (!slides) return NULL;

  SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
  if (!def) {
    free(slides);
    return NULL;
  }

  def->size = size;
  def->tokens = parseSlideText(text, GetConfigIconType());
  def->render = welcomeRender;
  def->update = NULL;
  def->slide_type = slide_type;
  def->hovered = -1;
  slides[0] = def;
  return slides;
}

/**
 * Resolve cursor position (cursorWeek, cursorDow) into a concrete date
 * stored in history_data (selYear, selMonth, selDay).
 * @param app Application state
 */
void HistoryResolveCursor(AppData* app) {
  HistoryData* h = &app->history_data;
  int monthWeeks[HISTORY_VISIBLE_MONTHS];
  int monthStartWeek[HISTORY_VISIBLE_MONTHS];
  int monthDays[HISTORY_VISIBLE_MONTHS];
  int monthStartDow[HISTORY_VISIBLE_MONTHS];
  int totalWeeks = computeGraphLayout(h->firstYear, h->firstMonth, monthWeeks,
                                      monthStartWeek, monthDays, monthStartDow);

  if (h->cursorWeek < 0) h->cursorWeek = 0;
  if (h->cursorWeek >= totalWeeks) h->cursorWeek = totalWeeks - 1;
  if (h->cursorDow < 0) h->cursorDow = 0;
  if (h->cursorDow > 6) h->cursorDow = 6;

  int y = 0, m = 0;
  int d =
    weekDowToDay(h->cursorWeek, h->cursorDow, h->firstYear, h->firstMonth,
                 monthWeeks, monthStartWeek, monthDays, monthStartDow, &y, &m);
  if (d > 0) {
    h->selYear = y;
    h->selMonth = m;
    h->selDay = d;
  }
}

/**
 * Create the History Overview popup (contribution graph).
 * @param app Application state
 */
void CreateHistoryOverviewDialog(AppData* app) {
  HistoryData* h = &app->history_data;

  /* Compute total weeks to determine width */
  int monthWeeks[HISTORY_VISIBLE_MONTHS];
  int monthStartWeek[HISTORY_VISIBLE_MONTHS];
  int monthDays[HISTORY_VISIBLE_MONTHS];
  int monthStartDow[HISTORY_VISIBLE_MONTHS];
  int totalWeeks = computeGraphLayout(h->firstYear, h->firstMonth, monthWeeks,
                                      monthStartWeek, monthDays, monthStartDow);

  /* Resolve cursor → date */
  HistoryResolveCursor(app);

  /* Dialog dimensions — width adapts to grid size */
  int dayLabelW = 3;
  int dlgW = 2 + dayLabelW + 1 + totalWeeks * 3 + 1;
  if (dlgW < 63) dlgW = 63;
  int dlgH = 5 + 7 + 1 + 2 + 3 + 1 + 1;
  if (dlgH < 18) dlgH = 18;

  Dimensions size = {.width = dlgW, .height = dlgH};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Close", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 1};

  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (!dialog) return;

  SlideDef** slides =
    BuildHistorySlide(size, historyOverviewRender, historyOverviewUpdate);
  if (!slides) {
    FreeFloatingDialog(dialog);
    return;
  }

  dialog->slides = slides;
  dialog->slideCount = 1;
  dialog->currentSlide = 0;
  dialog->slide_type = SLIDE_TYPE_HISTORY_OVERVIEW;
  dialog->hovered_button = -1;

  app->popup_dialog = dialog;
}

/**
 * Create the Day Detail popup showing sessions for the selected date.
 * @param app Application state
 */
void CreateHistoryDayDialog(AppData* app) {
  HistoryData* h = &app->history_data;

  int indices[100];
  time_t startTimes[100];
  int durations[100];
  int statuses[100];
  int scount =
    HistSessionsForDay(POMODORO_LOG, h->selYear, h->selMonth, h->selDay,
                       indices, startTimes, durations, statuses, 100);

  if (h->dayScroll < 0) h->dayScroll = 0;
  int maxVisible = 10;
  if (h->dayScroll > scount - maxVisible) h->dayScroll = scount - maxVisible;
  if (h->dayScroll < 0) h->dayScroll = 0;

  int visible = (scount < maxVisible) ? scount : maxVisible;
  int hdr =
    11; /* borders + title + separator + stats + table header + separator + nav hints */
  int height = hdr + visible;
  if (height < 14) height = 14;

  int w = 38;

  /* Build text buffer for the slide */
  char text[2048];
  char* tp = text;
  int remaining = (int)sizeof(text);
  int n;

  /* Title at y=1 (title bar), \\n\\n skips separator row to y=3 */
  n = snprintf(tp, remaining, "\\n\\c13\\aC%04d-%02d-%02d\\n\\n", h->selYear,
               h->selMonth, h->selDay);
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  /* Stats summary */
  int completed = 0, totalMins = 0;
  for (int i = 0; i < scount; i++) {
    if (statuses[i] == 0) completed++;
    totalMins += durations[i] / 60;
  }
  int pct = (scount > 0) ? (completed * 100 / scount) : 0;
  n = snprintf(tp, remaining,
               "\\aL\\c15Sessions:\\x13\\c07%d\\n"
               "\\aL\\c15Focus Time:\\x13\\c07%d min\\n"
               "\\aL\\c15Completed:\\x13\\c07%d%%\\n",
               scount, totalMins, pct);
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  /* Table header */
  n = snprintf(tp, remaining,
               "\\aL\\x01\\c15#\\x07Start\\x17End\\x27Duration\\n");
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  /* Session rows */
  int startRow = h->dayScroll;
  int endRow = startRow + visible;
  for (int i = startRow; i < endRow && i < scount; i++) {
    time_t endT = startTimes[i] + durations[i];
    struct tm* stm = localtime(&startTimes[i]);
    struct tm* etm = localtime(&endT);
    char sts[6], ets[6];
    strftime(sts, 6, "%H:%M", stm);
    strftime(ets, 6, "%H:%M", etm);
    n = snprintf(tp, remaining, "\\aL\\c07\\x01#%-3d\\x07%s\\x17%s\\x27%dm\\n",
                 indices[i], sts, ets, durations[i] / 60);
    tp += n;
    remaining = (int)(sizeof(text) - (tp - text));
  }

  /* Fill remaining lines with blanks, leaving row for controls */
  int usedLines =
    3 + 1 + (endRow - startRow); /* title + stats(3) + header + rows */
  int totalBody = height - 4;    /* minus borders */
  int maxUsed = totalBody - 2;   /* leave last 2 rows for controls gap */
  for (int r = usedLines; r < maxUsed; r++) {
    n = snprintf(tp, remaining, "\\aL\\n");
    tp += n;
    remaining = (int)(sizeof(text) - (tp - text));
  }

  /* Create dialog */
  Dimensions size = {.width = w, .height = height};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Close", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 1};

  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (!dialog) return;

  SlideDef** slides = BuildHistoryTextSlide(text, size, SLIDE_TYPE_HISTORY_DAY);

  if (!slides) {
    FreeFloatingDialog(dialog);
    return;
  }

  /* Add clickable controls for day detail */
  {
    ControlButton btns[] = {
      {"Esc Back", ALIGN_SLIDE_RIGHT, (SlideNavAction)HistoryCloseToOverview}};
    slides[0]->controls = slideControlsDup(btns, 1);
    slides[0]->render_controls = SlideControlsRender;
    slides[0]->update = welcomeUpdate;
  }

  dialog->slides = slides;
  dialog->slideCount = 1;
  dialog->currentSlide = 0;
  dialog->slide_type = SLIDE_TYPE_HISTORY_DAY;
  dialog->hovered_button = -1;

  app->popup_dialog = dialog;
}

/**
 * Create the Statistics popup with aggregated history data.
 * @param app Application state
 */
void CreateHistoryStatsDialog(AppData* app) {
  HistoryData* h = &app->history_data;

  /* Compute stats from 365 days of data */
  int currentStreak = 0, longestStreak = 0;
  HistStreak(POMODORO_LOG, h->selYear, h->selMonth, h->selDay, &currentStreak,
             &longestStreak);

  /* Total stats for last 365 days */
  time_t now = time(NULL);
  struct tm* tmNow = localtime(&now);
  int endYear = tmNow->tm_year + 1900;
  int endMonth = tmNow->tm_mon + 1;
  int endDay = tmNow->tm_mday;

  /* Scan log for totals */
  int totalSessions = 0;
  int totalFocusMins = 0;
  int dayCounts[7] = {0}; /* Sun=0...Sat=6 */
  int monthCounts[12] = {0};
  int bestDayCount = 0, bestDayIdx = -1;
  int bestDow = 0, bestDowCount = 0;
  int bestMonth = 0, bestMonthCount = 0;
  int totalDaysWithData = 0;
  int levelCounts[4] = {0};

  /* Read log and aggregate */
  FILE* file = fopen(POMODORO_LOG, "rb");
  if (file) {
    pomodoroLogRecord rec;
    while (fread(&rec, sizeof(rec), 1, file) == 1) {
      if (rec.session_index == 0) continue;
      time_t ts = (time_t)rec.session_start_time;
      struct tm* rtm = localtime(&ts);
      if (!rtm) continue;
      int y = rtm->tm_year + 1900;
      int m = rtm->tm_mon + 1;

      /* Check if within last year: =end year-1 to end year */
      bool inRange = false;
      if (y == endYear || (y == endYear - 1 && m > endMonth) ||
          (y == endYear && m <= endMonth)) {
        inRange = true;
      }
      if (!inRange && endYear > 0) continue;

      totalSessions++;
      totalFocusMins += rec.current_step_time / 60;
      dayCounts[rtm->tm_wday]++;
      monthCounts[m - 1]++;
    }
    fclose(file);
  }

  for (int i = 0; i < 7; i++) {
    if (dayCounts[i] > bestDowCount) {
      bestDowCount = dayCounts[i];
      bestDow = i;
    }
  }
  for (int i = 0; i < 12; i++) {
    if (monthCounts[i] > bestMonthCount) {
      bestMonthCount = monthCounts[i];
      bestMonth = i;
    }
  }

  /* Build a 365-day breakdown of level counts */
  {
    struct tm cursor = {0};
    cursor.tm_year = endYear - 1900;
    cursor.tm_mon = endMonth - 1;
    cursor.tm_mday = endDay;
    time_t endTS = mktime(&cursor);
    if (endTS != (time_t)-1) {
      for (int i = 0; i < 365; i++) {
        time_t dayTS = endTS - (time_t)(i * 86400);
        struct tm* dtm = localtime(&dayTS);
        if (!dtm) continue;
        int y = dtm->tm_year + 1900;
        int m = dtm->tm_mon + 1;
        int d = dtm->tm_mday;

        int ids[50];
        time_t sts[50];
        int drs[50];
        int stsArr[50];
        int count =
          HistSessionsForDay(POMODORO_LOG, y, m, d, ids, sts, drs, stsArr, 50);
        int level = HistLevelForCount(count);
        levelCounts[level]++;
      }
    }
  }

  /* Build text */
  char text[2048];
  char* tp = text;
  int remaining = (int)sizeof(text);
  int n;

  n = snprintf(tp, remaining, "\\n\\c13\\aCHISTORY STATISTICS\\n\\n");
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  n = snprintf(tp, remaining,
               "\\aL\\c15Current Streak:         \\x24\\c07%d days\\n"
               "\\aL\\c15Longest Streak:         \\x24\\c07%d days\\n"
               "\\n"
               "\\aL\\c15Total Sessions:         \\x24\\c07%d\\n"
               "\\aL\\c15Total Focus Time:       \\x24\\c07%dh %dm\\n"
               "\\n"
               "\\aL\\c15Most Productive DOW:    \\x24\\c07%s\\n"
               "\\aL\\c15Most Productive Month:  \\x24\\c07%s %d\\n",
               currentStreak, longestStreak, totalSessions, totalFocusMins / 60,
               totalFocusMins % 60,
               (const char*[]){"Sunday", "Monday", "Tuesday", "Wednesday",
                               "Thursday", "Friday", "Saturday"}[bestDow],
               (const char*[]){"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                               "Aug", "Sep", "Oct", "Nov", "Dec"}[bestMonth],
               endYear);
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  // if (count == 0) return 0;
  // if (count <= 2) return 1;
  // if (count <= 5) return 2;
  // return 3;
  n = snprintf(tp, remaining,
               "\\n\\aL\\c15Last 365 Days\\n"
               "\\n"
               "\\aL\\c07%s (0 s) %d days\\x19%s (1-2 s) %d days\\n"
               "\\aL\\c07%s (3-5 s) %d days\\x19%s (6+ s) %d days\\n",
               HISTORY_ICONS[0], levelCounts[0], HISTORY_ICONS[1],
               levelCounts[1], HISTORY_ICONS[2], levelCounts[2],
               HISTORY_ICONS[3], levelCounts[3]);
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  n = snprintf(tp, remaining, "\\aL\\c07\\n");
  tp += n;
  remaining = (int)(sizeof(text) - (tp - text));

  /* Create dialog */
  int w = 45;
  int height = 19;
  Dimensions size = {.width = w, .height = height};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Close", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 1};

  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (!dialog) return;

  SlideDef** slides =
    BuildHistoryTextSlide(text, size, SLIDE_TYPE_HISTORY_STATS);
  if (!slides) {
    FreeFloatingDialog(dialog);
    return;
  }

  /* Add clickable controls for statistics */
  {
    ControlButton btns[] = {
      {"Tab Overview", ALIGN_SLIDE_LEFT,
       (SlideNavAction)HistoryCloseToOverview},
      {"Esc Back", ALIGN_SLIDE_RIGHT, (SlideNavAction)HistoryCloseToOverview}};
    slides[0]->controls = slideControlsDup(btns, 2);
    slides[0]->render_controls = SlideControlsRender;
    slides[0]->update = welcomeUpdate;
  }

  dialog->slides = slides;
  dialog->slideCount = 1;
  dialog->currentSlide = 0;
  dialog->slide_type = SLIDE_TYPE_HISTORY_STATS;
  dialog->hovered_button = -1;

  app->popup_dialog = dialog;
}

/**
 * Compute the number of 7-day week columns a month spans.
 * Accounts for the starting day-of-week offset.
 * @param year Gregorian year
 * @param month Month (1-12)
 * @param startDow Day-of-week of the 1st of the month (0=Sunday)
 * @return Number of week columns
 */
static int computeMonthWeeks(int year, int month, int startDow) {
  int dim = HistDaysInMonth(year, month);
  int totalCells = startDow + dim;
  return (totalCells + 6) / 7; /* ceiling division */
}

/**
 * Dynamically compute the contribution graph dimensions and the list
 * of month boundaries.
 * @param firstYear First visible year
 * @param firstMonth First visible month (1-12)
 * @param monthWeeks Output array[HISTORY_VISIBLE_MONTHS] — weeks per month
 * @param monthStartWeek Output array[HISTORY_VISIBLE_MONTHS] — starting week column per month
 * @param monthDays Output array[HISTORY_VISIBLE_MONTHS] — days per month
 * @param monthStartDow Output array[HISTORY_VISIBLE_MONTHS] — starting day-of-week per month
 * @return Total week columns across all 5 months
 */
static int computeGraphLayout(int firstYear, int firstMonth, int* monthWeeks,
                              int* monthStartWeek, int* monthDays,
                              int* monthStartDow) {
  int totalWeeks = 0;
  int y = firstYear, m = firstMonth;
  for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
    monthDays[i] = HistDaysInMonth(y, m);
    monthStartDow[i] = HistDayOfWeek(y, m, 1);
    monthWeeks[i] = computeMonthWeeks(y, m, monthStartDow[i]);
    monthStartWeek[i] = totalWeeks;
    totalWeeks += monthWeeks[i];
    if (++m > 12) {
      m = 1;
      y++;
    }
  }
  return totalWeeks;
}

/**
 * Given a week column and day-of-week, resolve to a date.
 * Returns the date (1-31) within a specific month, or 0 if out of range.
 * @param weekCol Week column index
 * @param dow Day-of-week row (0=Sunday)
 * @param firstYear First visible year
 * @param firstMonth First visible month
 * @param monthWeeks Weeks-per-month array
 * @param monthStartWeek Starting week column per month
 * @param monthDays Days-per-month array
 * @param monthStartDow Starting day-of-week per month
 * @param outYear Output — resolved year
 * @param outMonth Output — resolved month
 * @return Day of month (1-31), or 0 if out of range
 */
static int weekDowToDay(int weekCol, int dow, int firstYear, int firstMonth,
                        int* monthWeeks, int* monthStartWeek, int* monthDays,
                        int* monthStartDow, int* outYear, int* outMonth) {
  /* Find which month this week falls in */
  int y = firstYear, m = firstMonth;
  for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
    if (weekCol < monthStartWeek[i] + monthWeeks[i]) {
      int localWeek = weekCol - monthStartWeek[i];
      int dayNum = localWeek * 7 + dow - monthStartDow[i] + 1;
      if (dayNum >= 1 && dayNum <= monthDays[i]) {
        *outYear = y;
        *outMonth = m;
        return dayNum;
      }
      return 0;
    }
    if (++m > 12) {
      m = 1;
      y++;
    }
  }
  return 0;
}

/**
 * Render the History Overview contribution graph slide.
 * Draws the month headers, day grid with contribution icons, streak info,
 * selected-day sidebar, session summary, and navigation hints.
 * @param app Application state
 * @param def Slide definition
 */
static void historyOverviewRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  HistoryData* h = &app->history_data;

  /* Dimension computation */
  int monthWeeks[HISTORY_VISIBLE_MONTHS];
  int monthStartWeek[HISTORY_VISIBLE_MONTHS];
  int monthDays[HISTORY_VISIBLE_MONTHS];
  int monthStartDow[HISTORY_VISIBLE_MONTHS];
  int totalWeeks = computeGraphLayout(h->firstYear, h->firstMonth, monthWeeks,
                                      monthStartWeek, monthDays, monthStartDow);

  /* Center dialog */
  d->size = def->size;
  UpdateFloatingDialog(d, app->screen);
  int x = d->position.x;
  int y = d->position.y;
  int w = d->size.width;

  /* Clear dialog area and draw box */
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  for (int r = 0; r < d->size.height; r++)
    for (int c = 0; c < d->size.width; c++) mvprintw(y + r, x + c, " ");

  /* Hover sync */
  if (d->hovered_button >= 0) def->hovered = d->hovered_button;

  /* Draw box */
  RenderSlideBox(x, y, w, d->size.height);

  /* Title */
  mvprintw(y + 1, x + (w - 7) / 2, "HISTORY");

  /* Month headers (y+3) — only show months that have data */
  int yMonths = y + 3;
  int iconType = GetConfigIconType();
  int gridStartX = x + 2; /* after border + space */
  int dayLabelW = 3;      /* "Su " */

  /* Compute day counts for each month */
  int dailyCounts[HISTORY_VISIBLE_MONTHS][31];
  for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
    int yr = h->firstYear, mo = h->firstMonth + i;
    while (mo > 12) {
      mo -= 12;
      yr++;
    }
    HistDailyCounts(POMODORO_LOG, yr, mo, dailyCounts[i]);
  }

  /* Month headers */
  {
    int ey = h->firstYear, em = h->firstMonth;
    for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
      int mw = monthWeeks[i];
      int headerX = gridStartX + dayLabelW + monthStartWeek[i] * 3 +
                    (mw * 3 - 3) / 2; /* approximate center */
      char buf[8];
      snprintf(buf, sizeof(buf), "%s", "");
      if (mw >= 2) {
        struct tm tm = {0};
        tm.tm_year = ey - 1900;
        tm.tm_mon = em - 1;
        tm.tm_mday = 1;
        mktime(&tm);
        strftime(buf, sizeof(buf), "%b", &tm);
      }
      SetColor(COLOR_CYAN, NO_COLOR, A_BOLD);
      mvprintw(yMonths, x + 2 + dayLabelW + monthStartWeek[i] * 3, "%s", buf);
      /* Update ey/em for next month */
      if (++em > 12) {
        em = 1;
        ey++;
      }
    }
  }
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);

  /* Grid: 7 rows (Sun=0 through Sat=6), totalWeeks columns */
  int gridY = y + 4;
  const char* dowNames[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  for (int dow = 0; dow < 7; dow++) {
    int rowY = gridY + dow;

    /* Day label */
    SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
    mvprintw(rowY, gridStartX, "%s ", dowNames[dow]);

    for (int wc = 0; wc < totalWeeks; wc++) {
      int cellX = gridStartX + dayLabelW + 1 + wc * 3;

      /* Find which month this week belongs to */
      int cyear = h->firstYear, cmonth = h->firstMonth;
      int found = 0;
      for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
        if (wc >= monthStartWeek[i] && wc < monthStartWeek[i] + monthWeeks[i]) {
          found = 1;
          int localWeek = wc - monthStartWeek[i];
          int dayNum = localWeek * 7 + dow - monthStartDow[i] + 1;
          if (dayNum >= 1 && dayNum <= monthDays[i]) {
            int level = HistLevelForCount(dailyCounts[i][dayNum - 1]);
            const char* icon = HISTORY_ICONS[level];

            /* Check if this is the selected or hovered day */
            bool isSel = (h->selYear == cyear && h->selMonth == cmonth &&
                          h->selDay == dayNum);
            bool isHov = (h->hoverWeek >= 0 && h->hoverWeek == wc &&
                          h->hoverDow == dow && !isSel);

            if (isSel || isHov) {
              SetColor(COLOR_CYAN, NO_COLOR, A_BOLD);
              mvprintw(rowY, cellX - 1, "<");
              mvprintw(rowY, cellX, "%s", icon);
              mvprintw(rowY, cellX + 2, ">");
            } else {
              SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
              mvprintw(rowY, cellX, "%s", icon);
              mvprintw(rowY, cellX + 2, " ");
            }
          } else {
            mvprintw(rowY, cellX, "   ");
          }
          break;
        }
        if (++cmonth > 12) {
          cmonth = 1;
          cyear++;
        }
      }
      if (!found) mvprintw(rowY, cellX, "   ");
    }
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  }

  /* Streak line + sidebar box (y+11 area) */
  {
    int streakY = y + 11;
    int currentStreak = 0, longestStreak = 0;
    HistStreak(POMODORO_LOG, h->selYear, h->selMonth, h->selDay, &currentStreak,
               &longestStreak);

    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    mvprintw(streakY + 1, x + 2, "Current Streak: %d day%s", currentStreak,
             currentStreak == 1 ? "" : "s");

    /* Sidebar: selected day info box */
    if (h->selDay > 0) {
      int boxW = 16; /* outer width: left + 14 content + right */
      int boxX = x + w - 1 - boxW;
      int level = 0;
      int cc = 0;
      for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
        int yr = h->firstYear, mo = h->firstMonth + i;
        while (mo > 12) {
          mo -= 12;
          yr++;
        }
        if (yr == h->selYear && mo == h->selMonth) {
          cc = dailyCounts[i][h->selDay - 1];
          break;
        }
      }
      level = HistLevelForCount(cc);

      /* Box top */
      mvaddch(streakY, boxX, ACS_ULCORNER);
      for (int ci = 1; ci < boxW - 1; ci++)
        mvaddch(streakY, boxX + ci, ACS_HLINE);
      mvaddch(streakY, boxX + boxW - 1, ACS_URCORNER);

      /* Box middle: icon + date */
      static const char* monNames[] = {"Jan", "Feb", "Mar", "Apr",
                                       "May", "Jun", "Jul", "Aug",
                                       "Sep", "Oct", "Nov", "Dec"};
      char dateBuf[20];
      snprintf(dateBuf, sizeof(dateBuf), "%d %s %d", h->selDay,
               monNames[h->selMonth - 1], h->selYear);
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      mvprintw(streakY + 1, boxX + 1, "%s", HISTORY_ICONS[level]);
      SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      mvprintw(streakY + 1, boxX + 4, "%s", dateBuf);

      /* Box bottom */
      mvaddch(streakY + 1, boxX, ACS_VLINE);
      mvaddch(streakY + 1, boxX + boxW - 1, ACS_VLINE);
      mvaddch(streakY + 2, boxX, ACS_LLCORNER);
      for (int ci = 1; ci < boxW - 1; ci++)
        mvaddch(streakY + 2, boxX + ci, ACS_HLINE);
      mvaddch(streakY + 2, boxX + boxW - 1, ACS_LRCORNER);
    }
  }

  /* Session summary separator */
  int sepY = y + 11 + 2 + 1; /* y+14 = after streak + sidebar box */
  {
    mvaddch(sepY, x, ACS_LTEE);
    for (int ci = 1; ci < w - 1; ci++) mvaddch(sepY, x + ci, ACS_HLINE);
    mvaddch(sepY, x + w - 1, ACS_RTEE);
  }

  {
    int infoY = sepY + 1;
    if (h->selDay > 0) {
      int sIndices[100];
      time_t sTimes[100];
      int sDurations[100];
      int sStatuses[100];
      int scount =
        HistSessionsForDay(POMODORO_LOG, h->selYear, h->selMonth, h->selDay,
                           sIndices, sTimes, sDurations, sStatuses, 100);

      static const char* dows[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                   "Thursday", "Friday", "Saturday"};
      struct tm tm = {0};
      tm.tm_year = h->selYear - 1900;
      tm.tm_mon = h->selMonth - 1;
      tm.tm_mday = h->selDay;
      mktime(&tm);
      int dw = tm.tm_wday;

      SetColor(COLOR_CYAN, NO_COLOR, A_BOLD);
      mvprintw(infoY, x + 2, "%04d-%02d-%02d • %s", h->selYear, h->selMonth,
               h->selDay, dows[dw]);

      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      int totalMins = 0;
      for (int i = 0; i < scount; i++) totalMins += sDurations[i] / 60;
      mvprintw(infoY + 1, x + 2, "%d session%s • %d minute%s focused", scount,
               scount == 1 ? "" : "s", totalMins, totalMins == 1 ? "" : "s");
    } else {
      mvprintw(infoY, x + 2, "No day selected");
    }
  }

  /* Navigation hints — key (15/bold) + description (07/normal), clickable */
  {
    int hintY = y + d->size.height - 2;
    static const char* hint_keys[] = {"h/j/k/l", "Enter", "Tab", "q"};
    static const char* descs[] = {" Navigate", " Day", " Stats", " Close"};
    static MenuAction actions[] = {NULL, (MenuAction)HistoryOpenDayDetail,
                                   (MenuAction)HistoryOpenStatistics,
                                   (MenuAction)ClosePopup};
    int nSeg = 4;
    int keyW[4], descW[4], segW[4], totalW = 0;
    for (int i = 0; i < nSeg; i++) {
      keyW[i] = utf8DisplayWidth(hint_keys[i]);
      descW[i] = utf8DisplayWidth(descs[i]);
      segW[i] = keyW[i] + descW[i];
      totalW += segW[i];
    }
    int gapW = (w - 2 - totalW) / (nSeg - 1);
    if (gapW < 2) gapW = 2;

    int cx = x + 1;
    for (int i = 0; i < nSeg; i++) {
      int hover = (def->hovered == i);
      SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
      if (hover) attron(A_REVERSE);
      mvprintw(hintY, cx, "%s", hint_keys[i]);
      if (hover) attroff(A_REVERSE);
      SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
      if (hover) attron(A_REVERSE);
      mvprintw(hintY, cx + keyW[i], "%s", descs[i]);
      if (hover) attroff(A_REVERSE);
      if (actions[i])
        RegisterClickRegion(app, cx, hintY, segW[i], 1, REGION_SLIDE_NAV,
                            actions[i], -1, i, 0);
      cx += segW[i] + gapW;
    }
  }

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
}

/**
 * Mouse update for the overview — tracks hover on grid cells
 * and nav hint segments.
 * @param app Application state
 * @param def Slide definition
 */
static void historyOverviewUpdate(AppData* app, SlideDef* def) {
  HistoryData* h = &app->history_data;
  FloatingDialog* d = app->popup_dialog;
  if (!d || !def) return;

  int x = d->position.x;
  int y = d->position.y;
  int w = def->size.width;

  /* Check nav hint segments first */
  static const char* hint_keys[] = {"h/j/k/l", "Enter", "Tab", "q"};
  static const char* descs[] = {" Navigate", " Day", " Stats", " Close"};
  int nSeg = 4;
  int keyW[4], descW[4], segW[4], totalW = 0;
  for (int i = 0; i < nSeg; i++) {
    keyW[i] = utf8DisplayWidth(hint_keys[i]);
    descW[i] = utf8DisplayWidth(descs[i]);
    segW[i] = keyW[i] + descW[i];
    totalW += segW[i];
  }
  int gapW = (w - 2 - totalW) / (nSeg - 1);
  if (gapW < 2) gapW = 2;

  int hintY = y + d->size.height - 2;
  int cx = x + 1;
  def->hovered = -1;
  d->hovered_button = -1;
  for (int i = 0; i < nSeg; i++) {
    if (app->mouse_y == hintY && app->mouse_x >= cx &&
        app->mouse_x < cx + segW[i]) {
      def->hovered = i;
      d->hovered_button = i;
      break;
    }
    cx += segW[i] + gapW;
  }

  /* Check grid area for hover */
  int gridX = x + 2 + 3 + 1; /* border + dayLabelW + space */
  int gridY = y + 4;

  if (app->mouse_y >= gridY && app->mouse_y < gridY + 7 &&
      app->mouse_x >= gridX) {
    int col = (app->mouse_x - gridX) / 3;
    int row = app->mouse_y - gridY;
    if (col >= 0 && row >= 0 && row < 7) {
      h->hoverWeek = col;
      h->hoverDow = row;
      return;
    }
  }
  h->hoverWeek = -1;
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
 * ---------------------------------------------------------------------------
 * Preferences
 * ---------------------------------------------------------------------------
 */

/**
 * Create the main preferences dialog showing the full list of preference
 * fields with keyboard and mouse navigation.
 * @param app Application state
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreatePreferencesDialog(AppData* app) {
  (void)app;
  Dimensions size = {.width = 56, .height = 25};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem items[] = {{"Back", ClosePopup}};
  Menu menu = {.items = items,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 1};
  FloatingDialog* dialog =
    CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (dialog) {
    SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
    if (!slides) {
      FreeFloatingDialog(dialog);
      return NULL;
    }
    SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
    if (!def) {
      free(slides);
      FreeFloatingDialog(dialog);
      return NULL;
    }
    def->size.width = size.width;
    def->size.height = size.height;
    def->render = prefsSlideRender;
    def->update = prefsSlideUpdate;
    def->slide_type = SLIDE_TYPE_PREFERENCES;
    def->hovered = 0;
    slides[0] = def;
    dialog->slides = slides;
    dialog->slideCount = 1;
    dialog->currentSlide = 0;
    dialog->slide_type = SLIDE_TYPE_PREFERENCES;
    dialog->hovered_button = 0;
  }
  return dialog;
}

/**
 * Open a stepper sub-dialog for editing a single integer, float, or toggle
 * preference field.  Stores the field index in app->prefs.edit_index and a
 * snapshot of the current value in app->prefs.edit_temp for Cancel restore.
 * @param app Application state
 * @param idx Preference field index to edit
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreatePrefsStepperDialog(AppData* app, int idx) {
  bool has_preview = (idx >= 0 && idx < app->prefs.count &&
                      app->prefs.fields[idx].preview != NULL);
  Dimensions size = {.width = has_preview ? 56 : 44, .height = 8};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem mi[] = {{"Save", ClosePrefsEdit}, {"Cancel", ClosePrefsEdit}};
  Menu menu = {.items = mi,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 2};
  FloatingDialog* d = CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (d) {
    SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
    if (!slides) {
      FreeFloatingDialog(d);
      return NULL;
    }
    SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
    if (!def) {
      free(slides);
      FreeFloatingDialog(d);
      return NULL;
    }
    def->size.width = size.width;
    def->size.height = size.height;
    def->render = prefsStepperRender;
    def->slide_type = SLIDE_TYPE_PREFS_STEPPER;
    slides[0] = def;
    d->slides = slides;
    d->slideCount = 1;
    d->currentSlide = 0;
    d->slide_type = SLIDE_TYPE_PREFS_STEPPER;
    d->hovered_button = idx;
    app->prefs.edit_index = idx;
    app->prefs.edit_temp = GetPrefInt(app, idx);
  }
  return d;
}

/**
 * Open a select sub-dialog for picking from a list of named options.
 * Stores the field index in app->prefs.select_index.
 * @param app Application state
 * @param idx Preference field index to edit
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreatePrefsSelectDialog(AppData* app, int idx) {
  int cnt = app->prefs.fields[idx].option_count;
  bool has_preview = (idx >= 0 && idx < app->prefs.count &&
                      app->prefs.fields[idx].preview != NULL);
  int nw = has_preview ? 56 : 44;
  int nh = cnt + 6;
  if (nh < 9) nh = 9;
  if (nh > 20) nh = 20;
  Dimensions size = {.width = nw, .height = nh};
  Vector2D pos = {.x = 0, .y = 0};
  MenuItem mi[] = {{"Apply", ClosePrefsSelect}, {"Cancel", ClosePrefsSelect}};
  Menu menu = {.items = mi,
               .selected_item = 0,
               .focused_color = COLOR_WHITE,
               .unfocused_color = COLOR_WHITE,
               .select_style_left = "",
               .select_style_right = "",
               .item_count = 2};
  FloatingDialog* d = CreateFloatingDialog(pos, size, InitBorder(), menu, "");
  if (d) {
    SlideDef** slides = (SlideDef**)calloc(1, sizeof(SlideDef*));
    if (!slides) {
      FreeFloatingDialog(d);
      return NULL;
    }
    SlideDef* def = (SlideDef*)calloc(1, sizeof(SlideDef));
    if (!def) {
      free(slides);
      FreeFloatingDialog(d);
      return NULL;
    }
    def->size.width = size.width;
    def->size.height = size.height;
    def->render = prefsSelectRender;
    def->update = prefsSelectUpdate;
    def->slide_type = SLIDE_TYPE_PREFS_SELECT;
    def->hovered = *app->prefs.fields[idx].int_value;
    slides[0] = def;
    d->slides = slides;
    d->slideCount = 1;
    d->currentSlide = 0;
    d->slide_type = SLIDE_TYPE_PREFS_SELECT;
    d->hovered_button = *app->prefs.fields[idx].int_value;
    app->prefs.select_index = idx;
  }
  return d;
}

/**
 * Read the current value of a preference field as an integer.
 * For PREF_STEPPER_FLOAT the stored float is multiplied by 100 and rounded.
 * @param app Application state
 * @param idx Preference field index
 * @return Current integer representation of the field value
 */
int GetPrefInt(AppData* app, int idx) {
  if (app->prefs.fields[idx].type == PREF_STEPPER_FLOAT)
    return (int)(*app->prefs.fields[idx].float_value * 100.0f + 0.5f);
  return *app->prefs.fields[idx].int_value;
}

/**
 * Close the stepper sub-dialog, restoring the original value saved in
 * app->prefs.edit_temp, and return to the main preferences list.
 * @param app Application state
 */
void ClosePrefsEdit(AppData* app) {
  if (app->prefs.edit_index >= 0) {
    setPrefInt(app, app->prefs.edit_index, app->prefs.edit_temp);
    app->prefs.edit_index = -1;
  }
  SyncIconsFromIndex();
  FreeFloatingDialog(app->popup_dialog);
  app->popup_dialog = CreatePreferencesDialog(app);
}

/**
 * Close the select sub-dialog and return to the main preferences list.
 * The option value is already committed by SelectApply() before this runs.
 * @param app Application state
 */
void ClosePrefsSelect(AppData* app) {
  app->prefs.select_index = -1;
  SyncIconsFromIndex();
  FreeFloatingDialog(app->popup_dialog);
  app->popup_dialog = CreatePreferencesDialog(app);
}

/**
 * Preview callback for "Desktop Notifications" — sends a test notification.
 * @param app Application state
 */
void PrefsPreviewDesktop(AppData* app) {
  (void)app;
  Notification notif = {"Test Notification",
                        "This is a test notification from settings.", NULL};
  Notify(&notif);
}

/**
 * Preview callback for "Sound" toggle and "Sound Volume" stepper —
 * plays the default notification sound at the configured volume.
 * @param app Application state
 */
void PrefsPreviewSound(AppData* app) {
  (void)app;
  PlayAudio("./sounds/dfltnotify.mp3", NOTIFICATIONS_SOUND_VOLUME, false);
}

/**
 * Write an integer value back to a preference field, converting to float
 * for PREF_STEPPER_FLOAT fields.
 * @param app Application state
 * @param idx Preference field index
 * @param val Integer value to store
 */
static void setPrefInt(AppData* app, int idx, int val) {
  if (app->prefs.fields[idx].type == PREF_STEPPER_FLOAT)
    *app->prefs.fields[idx].float_value = val / 100.0f;
  else
    *app->prefs.fields[idx].int_value = val;
}

/**
 * Count how many preference fields are selectable (skip PREF_SECTION headers).
 * @param app Application state
 * @return Number of interactive fields
 */
static int selectablePrefCount(AppData* app) {
  int n = 0;
  for (int i = 0; i < app->prefs.count; i++)
    if (app->prefs.fields[i].type != PREF_SECTION) n++;
  return n;
}

/**
 * Format the current value of a preference field into a display buffer.
 * Toggles show [✓] Enabled / [ ] Disabled, selects show the option label,
 * and steppers show the numeric value with unit.
 * @param app Application state
 * @param buf  Output buffer
 * @param len  Buffer size
 * @param idx  Preference field index
 */
static void renderPrefValue(AppData* app, char* buf, int len, int idx) {
  PrefField* f = &app->prefs.fields[idx];
  int val = GetPrefInt(app, idx);
  if (f->type == PREF_TOGGLE) {
    snprintf(buf, len, "[%s] %s", val ? "\u2713" : " ",
             val ? "Enabled" : "Disabled");
  } else if (f->type == PREF_SELECT) {
    if (f->options && val >= 0 && val < f->option_count)
      snprintf(buf, len, "%s", f->options[val]);
    else
      snprintf(buf, len, "?");
  } else {
    const char* u = f->unit ? f->unit : "";
    if (f->type == PREF_STEPPER_FLOAT)
      snprintf(buf, len, "%d%s", val, u);
    else
      snprintf(buf, len, "%d %s", val, u);
  }
}

/**
 * Compute the display width (terminal cell count) of a UTF-8 string.
 * Multi-byte continuation bytes (0x80–0xBF) are not counted.
 * @param s  Null-terminated UTF-8 string
 * @return  Number of terminal cells occupied
 */
static int displayWidth(const char* s) {
  int n = (int)strlen(s);
  int cont = 0;
  for (const char* p = s; *p; p++) {
    if (((unsigned char)*p & 0xC0) == 0x80) cont++;
  }
  return n - cont;
}

/**
 * Render the main preferences list dialog.
 * Draws each field as a row with label, dots, current value, and an arrow.
 * Section headers are rendered in cyan bold.  The footer shows evenly-spaced
 * navigation hints with mouse click regions.
 * @param app Application state
 * @param def Slide definition (provides width, height, hovered index)
 */
static void prefsSlideRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
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

  if (d->hovered_button >= 0) def->hovered = d->hovered_button;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  RenderSlideBox(x, y, w, h);

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  int title_x = x + (w - 11) / 2;
  mvprintw(y + 1, title_x, "Preferences");

  /* Pre-compute content rows for each field */
  int total_cr = 0;
  int content_row[64];
  for (int i = 0; i < app->prefs.count && i < 64; i++) {
    content_row[i] = total_cr;
    if (app->prefs.fields[i].type == PREF_SECTION && i > 0) {
      total_cr += 2;  /* blank line + header */
    } else {
      total_cr++;
    }
  }

  int max_visible = h - 6;
  int sr = app->prefs.scroll_row;
  int end_cr = sr + max_visible;
  if (end_cr > total_cr) end_cr = total_cr;
  bool show_up = (sr > 0);
  bool show_down = (end_cr < total_cr);

  int sel = d->hovered_button;
  int cnt = 0;
  int row_avail = w - 4;

  int field_start_row = 3 + (show_up ? 1 : 0);
  int field_end_row = h - 3;
  if (show_down) field_end_row--;

  if (show_up) {
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    mvprintw(y + 3, x + 2 + row_avail - 7, "\u25b2 More");
    RegisterClickRegion(app, x + 2 + row_avail - 7, y + 3, 7, 1,
                        REGION_SLIDE_NAV, (MenuAction)PrefsScrollUp, -1, 0, 0);
  }

  int row = field_start_row;
  for (int i = 0; i < app->prefs.count; i++) {
    int cr = content_row[i];
    bool is_sec = (app->prefs.fields[i].type == PREF_SECTION);
    if (is_sec && i > 0) {
      if (cr + 1 < sr) continue;
      if (cr >= end_cr) break;
    } else {
      if (cr < sr) {
        if (!is_sec) cnt++;
        continue;
      }
      if (cr >= end_cr) break;
    }
    if (row >= field_end_row) break;

    if (is_sec) {
      if (i > 0 && cr >= sr) {
        SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
        for (int c = 0; c < row_avail; c++) mvprintw(y + row, x + 2 + c, " ");
        row++;
      }
      SetColor(COLOR_CYAN, NO_COLOR, A_BOLD);
      mvprintw(y + row, x + 2, "%s", app->prefs.fields[i].label);
      row++;
      continue;
    }

    int is_sel = (cnt == sel);
    cnt++;

    char val_buf[48];
    renderPrefValue(app, val_buf, sizeof(val_buf), i);

    int fg = is_sel ? COLOR_BLACK : COLOR_WHITE;
    int bg = is_sel ? COLOR_WHITE : NO_COLOR;
    int attr = is_sel ? A_BOLD : A_NORMAL;
    SetColor(fg, bg, attr);
    for (int c = 0; c < row_avail; c++) mvprintw(y + row, x + 2 + c, " ");

    int L = (int)strlen(app->prefs.fields[i].label);
    int V = displayWidth(val_buf);
    int fill = row_avail - 6 - L - V;
    if (fill < 0) fill = 0;
    char dots[64];
    int dot_len = fill < (int)sizeof(dots) - 1 ? fill : (int)sizeof(dots) - 1;
    memset(dots, '.', dot_len);
    dots[dot_len] = '\0';

    mvprintw(y + row, x + 2, "%c %s", is_sel ? '>' : ' ',
             app->prefs.fields[i].label);
    mvprintw(y + row, x + 4 + L + 1, "%s %s", dots, val_buf);
    mvprintw(y + row, x + 2 + row_avail - 2, " \u25b6");
    RegisterClickRegion(app, x + 2, y + row, row_avail, 1, REGION_SLIDE_NAV,
                        app->prefs.fields[i].type == PREF_TOGGLE
                          ? (MenuAction)PrefsToggle
                          : (MenuAction)PrefsEdit,
                        -1, 0, 0);
    row++;
  }

  if (show_down) {
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    mvprintw(y + h - 4, x + 2 + row_avail - 7, "\u25bc More");
    RegisterClickRegion(app, x + 2 + row_avail - 7, y + h - 4, 7, 1,
                        REGION_SLIDE_NAV, (MenuAction)PrefsScrollDown, -1, 0, 0);
  }

  /* Evenly spaced sections across the full width */
  int s1w = displayWidth("\u2191/\u2193 Navigate");
  int s2w = displayWidth("\u2190/\u2192 Change");
  int s3w = displayWidth("Space Toggle");
  int s4w = displayWidth("q Back");
  int total_cw = s1w + s2w + s3w + s4w;
  int gap_avail = row_avail - total_cw;
  int gap = gap_avail / 3;
  int gap_rem = gap_avail - gap * 3;
  int fx = x + 2;
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(y + h - 2, fx, "\u2191/\u2193 ");
  fx += displayWidth("\u2191/\u2193 ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y + h - 2, fx, "Navigate");
  fx += displayWidth("Navigate");
  fx += gap + (gap_rem-- > 0 ? 1 : 0);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(y + h - 2, fx, "\u2190/\u2192 ");
  fx += displayWidth("\u2190/\u2192 ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y + h - 2, fx, "Change");
  fx += displayWidth("Change");
  fx += gap + (gap_rem-- > 0 ? 1 : 0);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(y + h - 2, fx, "Space ");
  fx += displayWidth("Space ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y + h - 2, fx, "Toggle");
  fx += displayWidth("Toggle");
  int qx = fx + gap + (gap_rem-- > 0 ? 1 : 0);
  int qw = displayWidth("q Back");
  int qy = y + h - 2;
  bool on_q =
    (app->mouse_y == qy && app->mouse_x >= qx && app->mouse_x < qx + qw);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  if (on_q) attron(A_REVERSE);
  mvprintw(qy, qx, "q ");
  if (on_q) attroff(A_REVERSE);
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  if (on_q) attron(A_REVERSE);
  mvprintw(qy, qx + displayWidth("q "), "Back");
  if (on_q) attroff(A_REVERSE);
  RegisterClickRegion(app, qx, qy, qw, 1, REGION_SLIDE_NAV,
                      (MenuAction)ClosePopup, -1, 0, 0);
}

/**
 * Update mouse hover state for the main preferences list.
 * Maps the mouse row to the corresponding selectable field index and stores
 * it in d->hovered_button and def->hovered.
 * @param app Application state
 * @param def Slide definition
 */
static void prefsSlideUpdate(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int x = d->position.x;
  int y = d->position.y;
  int w = def->size.width;
  int h = def->size.height;
  int my = app->mouse_y;
  int mx = app->mouse_x;
  int sel_cnt = selectablePrefCount(app);
  if (my < y + 3 || my >= y + h - 2 || mx < x + 2 || mx >= x + w - 2) {
    if (d->hovered_button >= sel_cnt) d->hovered_button = sel_cnt - 1;
    if (d->hovered_button < 0) d->hovered_button = 0;
    def->hovered = d->hovered_button;
    return;
  }
  int target_row = my - y;
  int max_visible = h - 6;
  int sr = app->prefs.scroll_row;
  int end_cr = sr + max_visible;
  int total_cr = 0;
  for (int i = 0; i < app->prefs.count; i++) {
    if (app->prefs.fields[i].type == PREF_SECTION && i > 0) {
      total_cr += 2;
    } else {
      total_cr++;
    }
  }
  if (end_cr > total_cr) end_cr = total_cr;
  bool show_up = (sr > 0);
  bool show_down = (end_cr < total_cr);

  int field_start_row = 3 + (show_up ? 1 : 0);
  int field_end_row = h - 3;
  if (show_down) field_end_row--;

  if (target_row < field_start_row || target_row >= field_end_row) {
    if (d->hovered_button >= sel_cnt) d->hovered_button = sel_cnt - 1;
    if (d->hovered_button < 0) d->hovered_button = 0;
    def->hovered = d->hovered_button;
    return;
  }

  int total_cr2 = 0;
  int content_row[64];
  for (int i = 0; i < app->prefs.count && i < 64; i++) {
    content_row[i] = total_cr2;
    if (app->prefs.fields[i].type == PREF_SECTION && i > 0) {
      total_cr2 += 2;
    } else {
      total_cr2++;
    }
  }

  int row = field_start_row;
  int cnt = 0;
  for (int i = 0; i < app->prefs.count; i++) {
    int cr = content_row[i];
    bool is_sec = (app->prefs.fields[i].type == PREF_SECTION);
    if (is_sec && i > 0) {
      if (cr + 1 < sr) continue;
      if (cr >= end_cr) break;
    } else {
      if (cr < sr) {
        if (!is_sec) cnt++;
        continue;
      }
      if (cr >= end_cr) break;
    }
    if (row >= field_end_row) break;
    if (is_sec) {
      if (i > 0 && cr >= sr) row++;
      row++;
      continue;
    }
    if (row == target_row) {
      d->hovered_button = cnt;
      def->hovered = cnt;
      return;
    }
    cnt++;
    row++;
  }
  if (d->hovered_button >= sel_cnt) d->hovered_button = sel_cnt - 1;
  if (d->hovered_button < 0) d->hovered_button = 0;
  def->hovered = d->hovered_button;
}

/**
 * Render the stepper sub-dialog for editing a numeric or toggle field.
 * Shows [-] value [+] controls, a range string, and footer with
 * ←/→ Change, conditional p Preview, Enter Save, and Esc Cancel.
 * @param app Application state
 * @param def Slide definition
 */
static void prefsStepperRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int w = def->size.width;
  int h = def->size.height;
  d->size.width = w;
  d->size.height = h;
  UpdateFloatingDialog(d, app->screen);
  int x = d->position.x;
  int y = d->position.y;

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) mvprintw(y + r, x + c, " ");

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  RenderSlideBox(x, y, w, h);

  int idx = d->hovered_button;
  PrefField* f = &app->prefs.fields[idx];
  const char* label = f->label;
  int val = GetPrefInt(app, idx);
  int min = f->min;
  int max = f->max;
  int step = f->step;
  const char* unit = f->unit ? f->unit : "";

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  int title_x = x + (w - (int)strlen(label)) / 2;
  mvprintw(y + 1, title_x, "%s", label);

  char val_str[32];
  if (f->type == PREF_TOGGLE)
    snprintf(val_str, sizeof(val_str), val ? "On" : "Off");
  else if (f->type == PREF_STEPPER_FLOAT)
    snprintf(val_str, sizeof(val_str), "%d%s", val, unit);
  else
    snprintf(val_str, sizeof(val_str), "%d %s", val, unit);

  char range_str[64];
  if (f->type == PREF_TOGGLE)
    snprintf(range_str, sizeof(range_str), "Off\u2013On");
  else if (f->type == PREF_STEPPER_FLOAT)
    snprintf(range_str, sizeof(range_str), "Range: %d\u2013%d%s", min, max,
             unit);
  else
    snprintf(range_str, sizeof(range_str), "Range: %d\u2013%d %s", min, max,
             unit);

  int range_center = x + (w - displayWidth(range_str)) / 2;

  /* [ - ] value [ + ] */
  const char* minus = "[-]";
  const char* plus = "[+]";
  int val_dw = displayWidth(val_str);
  int val_start = x + (w - val_dw) / 2;
  int minus_x = val_start - (int)strlen(minus) - 2;
  int plus_x = val_start + val_dw + 2;

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  int val_row = 3;
  RegisterClickRegion(app, minus_x, y + val_row, (int)strlen(minus), 1,
                      REGION_SLIDE_NAV, (MenuAction)StepperDecrement, -1, 0, 0);
  mvprintw(y + val_row, minus_x, "%s", minus);
  SetColor(COLOR_CYAN, NO_COLOR, A_BOLD);
  mvprintw(y + val_row, val_start, "%s", val_str);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  int plus_w = (int)strlen(plus);
  RegisterClickRegion(app, plus_x, y + val_row, plus_w, 1, REGION_SLIDE_NAV,
                      (MenuAction)StepperIncrement, -1, 0, 0);
  mvprintw(y + val_row, plus_x, "%s", plus);

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(y + val_row + 1, range_center, "%s", range_str);

  /* Footer: BOLD keys, NORMAL descriptions, A_REVERSE on hover for clickable */
  int fy = y + h - 2;
  int fx = x + 2;
  bool has_preview = (f->preview != NULL);
  /* ←/→ Change */
  int s1 = fx;
  int w1 = displayWidth("\u2190/\u2192 Change");
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(fy, fx, "\u2190/\u2192 ");
  fx += displayWidth("\u2190/\u2192 ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(fy, fx, "Change");
  fx += displayWidth("Change") + 4;
  /* p Preview (conditional) */
  if (has_preview) {
    int s = fx;
    int w = displayWidth("p Preview");
    bool on = (app->mouse_y == fy && app->mouse_x >= s && app->mouse_x < s + w);
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    if (on) attron(A_REVERSE);
    mvprintw(fy, fx, "p ");
    if (on) attroff(A_REVERSE);
    fx += displayWidth("p ");
    SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
    if (on) attron(A_REVERSE);
    mvprintw(fy, fx, "Preview");
    if (on) attroff(A_REVERSE);
    fx += displayWidth("Preview") + 4;
    RegisterClickRegion(app, s, fy, w, 1, REGION_SLIDE_NAV,
                        (MenuAction)PrefsPreview, -1, 0, 0);
  }
  /* Enter Save */
  int s2 = fx;
  int w2 = displayWidth("Enter Save");
  bool on_enter =
    (app->mouse_y == fy && app->mouse_x >= s2 && app->mouse_x < s2 + w2);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  if (on_enter) attron(A_REVERSE);
  mvprintw(fy, fx, "Enter ");
  if (on_enter) attroff(A_REVERSE);
  fx += displayWidth("Enter ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  if (on_enter) attron(A_REVERSE);
  mvprintw(fy, fx, "Save");
  if (on_enter) attroff(A_REVERSE);
  RegisterClickRegion(app, s2, fy, w2, 1, REGION_SLIDE_NAV,
                      (MenuAction)StepperClose, -1, 0, 0);
  fx += displayWidth("Save") + 4;
  /* Esc Cancel */
  int s3 = fx;
  int w3 = displayWidth("Esc Cancel");
  bool on_esc =
    (app->mouse_y == fy && app->mouse_x >= s3 && app->mouse_x < s3 + w3);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  if (on_esc) attron(A_REVERSE);
  mvprintw(fy, fx, "Esc ");
  if (on_esc) attroff(A_REVERSE);
  fx += displayWidth("Esc ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  if (on_esc) attron(A_REVERSE);
  mvprintw(fy, fx, "Cancel");
  if (on_esc) attroff(A_REVERSE);
  RegisterClickRegion(app, s3, fy, w3, 1, REGION_SLIDE_NAV,
                      (MenuAction)StepperClose, -1, 0, 0);
}

/**
 * Update mouse hover state for the select sub-dialog.
 * Maps the mouse row to an option index and stores it in d->hovered_button
 * and def->hovered.
 * @param app Application state
 * @param def Slide definition
 */
static void prefsSelectUpdate(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = app->prefs.select_index;
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  int x = d->position.x;
  int y = d->position.y;
  int w = def->size.width;
  int h = def->size.height;
  int my = app->mouse_y;
  int mx = app->mouse_x;
  if (my < y + 3 || my >= y + h - 3 || mx < x + 2 || mx >= x + w - 2) return;
  int prow = my - y - 3;
  if (prow >= 0 && prow < f->option_count) {
    d->hovered_button = prow;
    def->hovered = prow;
  }
}

/**
 * Render the select sub-dialog showing all options for a PREF_SELECT field.
 * The currently selected option is highlighted with reverse video and a '>'
 * marker.  The footer shows ↑/↓ Select, conditional p Preview, Enter Apply,
 * and Esc Cancel with mouse click regions.
 * @param app Application state
 * @param def Slide definition
 */
static void prefsSelectRender(AppData* app, SlideDef* def) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int w = def->size.width;
  int h = def->size.height;
  d->size.width = w;
  d->size.height = h;
  UpdateFloatingDialog(d, app->screen);
  int x = d->position.x;
  int y = d->position.y;

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) mvprintw(y + r, x + c, " ");

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  RenderSlideBox(x, y, w, h);

  int idx = app->prefs.select_index;
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];

  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  int title_x = x + (w - (int)strlen(f->label)) / 2;
  mvprintw(y + 1, title_x, "%s", f->label);

  int row = 3;
  int sel = d->hovered_button;
  int inner_w = w - 4;
  for (int i = 0; i < f->option_count && row < h - 3; i++) {
    int is_sel = (i == sel);
    SetColor(is_sel ? COLOR_BLACK : COLOR_WHITE,
             is_sel ? COLOR_WHITE : NO_COLOR, is_sel ? A_BOLD : A_NORMAL);
    int text_x = x + (w - (int)strlen(f->options[i])) / 2;
    mvprintw(y + row, text_x - 2, "%c ", is_sel ? '>' : ' ');
    mvprintw(y + row, text_x, "%s", f->options[i]);
    RegisterClickRegion(app, x + 2, y + row, inner_w, 1, REGION_SLIDE_NAV,
                        (MenuAction)SelectApply, -1, 0, 0);
    row++;
  }

  /* Footer: BOLD keys, NORMAL descriptions, A_REVERSE on clickable */
  int fy = y + h - 2;
  int fx = x + 2;
  bool has_preview = (f->preview != NULL);
  /* ↑/↓ Select */
  int s1 = fx;
  int w1 = displayWidth("\u2191/\u2193 Select");
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  mvprintw(fy, fx, "\u2191/\u2193 ");
  fx += displayWidth("\u2191/\u2193 ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(fy, fx, "Select");
  fx += displayWidth("Select") + 4;
  /* p Preview (conditional) */
  if (has_preview) {
    int s = fx;
    int w = displayWidth("p Preview");
    bool on = (app->mouse_y == fy && app->mouse_x >= s && app->mouse_x < s + w);
    SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
    if (on) attron(A_REVERSE);
    mvprintw(fy, fx, "p ");
    if (on) attroff(A_REVERSE);
    fx += displayWidth("p ");
    SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
    if (on) attron(A_REVERSE);
    mvprintw(fy, fx, "Preview");
    if (on) attroff(A_REVERSE);
    fx += displayWidth("Preview") + 4;
    RegisterClickRegion(app, s, fy, w, 1, REGION_SLIDE_NAV,
                        (MenuAction)PrefsPreview, -1, 0, 0);
  }
  /* Enter Apply */
  int s2 = fx;
  int w2 = displayWidth("Enter Apply");
  bool on_ent =
    (app->mouse_y == fy && app->mouse_x >= s2 && app->mouse_x < s2 + w2);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  if (on_ent) attron(A_REVERSE);
  mvprintw(fy, fx, "Enter ");
  if (on_ent) attroff(A_REVERSE);
  fx += displayWidth("Enter ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  if (on_ent) attron(A_REVERSE);
  mvprintw(fy, fx, "Apply");
  if (on_ent) attroff(A_REVERSE);
  RegisterClickRegion(app, s2, fy, w2, 1, REGION_SLIDE_NAV,
                      (MenuAction)SelectApply, -1, 0, 0);
  fx += displayWidth("Apply") + 4;
  /* Esc Cancel */
  int s3 = fx;
  int w3 = displayWidth("Esc Cancel");
  bool on_esc =
    (app->mouse_y == fy && app->mouse_x >= s3 && app->mouse_x < s3 + w3);
  SetColor(COLOR_WHITE, NO_COLOR, A_BOLD);
  if (on_esc) attron(A_REVERSE);
  mvprintw(fy, fx, "Esc ");
  if (on_esc) attroff(A_REVERSE);
  fx += displayWidth("Esc ");
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  if (on_esc) attron(A_REVERSE);
  mvprintw(fy, fx, "Cancel");
  if (on_esc) attroff(A_REVERSE);
  RegisterClickRegion(app, s3, fy, w3, 1, REGION_SLIDE_NAV,
                      (MenuAction)ClosePrefsSelect, -1, 0, 0);
}
