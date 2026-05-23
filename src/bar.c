#include "bar.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "notes.h"
#include "tomato.h"

/* PRIVATE BAR FUNCTIONS */
/* StatusBar Lifecycle */
static StatusBarModule* createStatusBarModule(StatusBarModulePosition position,
                                              char* content, int fg_color,
                                              int bg_color,
                                              ModuleUpdate update);
static void freeStatusBarModule(StatusBarModule* widget);
static void freeStatusBarModules(StatusBarModule* module);
/* Rendering/Updating */
static void renderStatusBarModule(const StatusBarModule* module, int start_y,
                                  int start_x, int max_width);
static void updateStatusBarModule(AppData* app, StatusBarModule* module,
                                  Panel* current_panel);

/**
 * ---------------------------------------------------------------------------
 * StatusBar Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create and allocate a new StatusBar.
 * @param position Position of the status bar (TOP or BOTTOM)
 * @return Pointer to the created status bar, or NULL on allocation failure
 */
StatusBar* CreateStatusBar(StatusBarPosition position) {
  StatusBar* bar = (StatusBar*)malloc(sizeof(StatusBar));
  if (bar == NULL) return NULL;

  bar->position = position;
  bar->left_modules = NULL;
  bar->center_modules = NULL;
  bar->right_modules = NULL;

  return bar;
}

/**
 * Free a StatusBar and all its modules.
 * @param bar Pointer to the status bar to free
 */
void FreeStatusBar(StatusBar* bar) {
  if (bar == NULL) return;

  freeStatusBarModules(bar->left_modules);
  freeStatusBarModules(bar->center_modules);
  freeStatusBarModules(bar->right_modules);

  free(bar);
}

/**
 * Add a new StatusBarModule to the end of the linked list by position.
 * @param status_bar Pointer to the status bar
 * @param position Position for the new module
 * @param update Function pointer for the module update logic
 */
void AddStatusBarModule(StatusBar* status_bar, StatusBarModulePosition position,
                        ModuleUpdate update) {
  if (status_bar == NULL) return;

  char* default_content = (char*)"default_content";
  int default_fg_color = COLOR_WHITE;
  int default_bg_color = COLOR_BLACK;

  StatusBarModule* new_widget = createStatusBarModule(
    position, default_content, default_fg_color, default_bg_color, update);
  if (new_widget == NULL) return;

  StatusBarModule** modules_root;
  switch (position) {
    case LEFT:
      modules_root = &status_bar->left_modules;
      break;
    case RIGHT:
      modules_root = &status_bar->right_modules;
      break;
    case CENTER:
      modules_root = &status_bar->center_modules;
      break;
    default:
      return;
  }

  if (*modules_root == NULL) {
    *modules_root = new_widget;
  } else {
    StatusBarModule* current = *modules_root;
    while (current->next != NULL) {
      new_widget->id++;
      current = current->next;
    }
    new_widget->id++;
    current->next = new_widget;
  }
}

/**
 * Create and allocate a new StatusBarModule.
 * @param position Position of the module within the bar
 * @param content Initial content string
 * @param fg_color Foreground color
 * @param bg_color Background color
 * @param update Function pointer for module updates
 * @return Pointer to the created module, or NULL on allocation failure
 */
static StatusBarModule* createStatusBarModule(StatusBarModulePosition position,
                                              char* content, int fg_color,
                                              int bg_color,
                                              ModuleUpdate update) {
  StatusBarModule* new_widget =
    (StatusBarModule*)malloc(sizeof(StatusBarModule));
  if (new_widget == NULL) return NULL;

  new_widget->fg_color = fg_color;
  new_widget->bg_color = bg_color;
  new_widget->update = update;
  new_widget->position = position;
  new_widget->next = NULL;
  new_widget->id = 0;

  if (content != NULL) {
    new_widget->content_length = strlen(content);
    new_widget->content = (char*)malloc(new_widget->content_length + 1);
    if (new_widget->content == NULL) {
      free(new_widget);
      return NULL;
    }
    strncpy(new_widget->content, content, new_widget->content_length);
    new_widget->content[new_widget->content_length] = '\0';
  } else {
    new_widget->content = NULL;
    new_widget->content_length = 0;
  }

  return new_widget;
}

/**
 * Free a single StatusBarModule and its content.
 * @param widget Pointer to the module to free
 */
static void freeStatusBarModule(StatusBarModule* widget) {
  if (widget == NULL) return;
  if (widget->content != NULL) free(widget->content);
  free(widget);
}

/**
 * Free a linked list of StatusBarModule modules.
 * @param module Pointer to the first module in the list
 */
static void freeStatusBarModules(StatusBarModule* module) {
  StatusBarModule* current = module;
  StatusBarModule* next;

  while (current) {
    next = current->next;
    freeStatusBarModule(current);
    current = next;
  }
}

/**
 * Render a single status bar module at the specified coordinates.
 * @param module Pointer to the module to render
 * @param start_y Starting y coordinate
 * @param start_x Starting x coordinate
 * @param max_width Maximum width for the module
 */
static void renderStatusBarModule(const StatusBarModule* module, int start_y,
                                  int start_x, int max_width) {
  if (module == NULL || module->content == NULL) return;

  if (GetConfigIconType() == NERD_ICONS) {
    /* Calculate the number of characters that fit within max_width */
    int byte_count = 0;
    int char_count = UTF16CharFitWidth(module->content, max_width, &byte_count);

    char buffer[byte_count + 1];
    strncpy(buffer, module->content, byte_count);
    buffer[byte_count] = '\0';

    SetColor(module->fg_color, module->bg_color, A_BOLD);
    if (DEBUG) mvprintw(start_y - 1, start_x, "%d", module->id);
    mvprintw(start_y, start_x, "%s", buffer);
  } else {
    int byte_count = 0;
    int char_count = UTF16CharFitWidth(module->content, max_width, &byte_count);

    char buffer[byte_count + 1];
    strncpy(buffer, module->content, byte_count);
    buffer[byte_count] = '\0';

    SetColor(module->fg_color, module->bg_color, A_BOLD);
    mvprintw(start_y, start_x, "%s", buffer);
  }
}

/**
 * ---------------------------------------------------------------------------
 * Rendering/Updating
 * ---------------------------------------------------------------------------
 */

/**
 * Render the entire status bar with all its modules.
 * @param status_bar Pointer to the status bar to render
 * @param screen Pointer to the screen for dimensions
 * @param has_error_line If true, render status bar one line above bottom
 */
void RenderStatusBar(const StatusBar* status_bar, const Screen* screen,
                     bool has_error_line) {
  if (status_bar == NULL || screen == NULL) return;

  int bar_width = screen->size.width;
  int bar_y;
  if (status_bar->position == TOP) {
    bar_y = 0;
  } else if (has_error_line) {
    bar_y = screen->size.height - 2;
  } else {
    bar_y = screen->size.height - 1;
  }

  /* Fill background */
  SetColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
  for (int i = 0; i < bar_width; i++) mvprintw(bar_y, i, " ");

  /* Render LEFT modules */
  StatusBarModule* current = status_bar->left_modules;
  int x_offset = 0;
  while (current != NULL) {
    renderStatusBarModule(current, bar_y, x_offset,
                          bar_width - x_offset + STATUS_BAR_SPACING);
    x_offset += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }

  /* Render CENTER modules */
  int total_center_width = 0;
  current = status_bar->center_modules;
  while (current != NULL) {
    total_center_width += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }
  if (total_center_width > 1) total_center_width -= 1;
  x_offset = (bar_width - total_center_width) / 2;
  current = status_bar->center_modules;
  while (current != NULL) {
    renderStatusBarModule(current, bar_y, x_offset, bar_width - x_offset);
    x_offset += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }

  /* Render RIGHT modules */
  int total_right_width = 0;
  current = status_bar->right_modules;
  while (current != NULL) {
    total_right_width += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }
  x_offset = bar_width - total_right_width;
  current = status_bar->right_modules;
  while (current != NULL) {
    renderStatusBarModule(current, bar_y, x_offset + STATUS_BAR_SPACING,
                          bar_width - x_offset + STATUS_BAR_SPACING);
    x_offset += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }
}

/**
 * Update all modules in the status bar.
 * @param app Pointer to the application data
 * @param status_bar Pointer to the status bar to update
 * @param current_panel Pointer to the current panel
 */
void UpdateStatusBar(AppData* app, StatusBar* status_bar,
                     Panel* current_panel) {
  if (status_bar == NULL || current_panel == NULL) return;

  StatusBarModule* current;

  /* Update LEFT modules */
  current = status_bar->left_modules;
  while (current != NULL) {
    updateStatusBarModule(app, current, current_panel);
    current = current->next;
  }

  /* Update CENTER modules */
  current = status_bar->center_modules;
  while (current != NULL) {
    updateStatusBarModule(app, current, current_panel);
    current = current->next;
  }

  /* Update RIGHT modules */
  current = status_bar->right_modules;
  while (current != NULL) {
    updateStatusBarModule(app, current, current_panel);
    current = current->next;
  }
}

/**
 * Update a single status bar module with the latest app data.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param current_panel Pointer to the current panel
 */
static void updateStatusBarModule(AppData* app, StatusBarModule* module,
                                  Panel* current_panel) {
  if (module && module->update) module->update(app, module, current_panel);
}

/**
 * ---------------------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------------------
 */

/**
 * Inverts the order of a linked list of StatusBarModule.
 * Used for right-aligned modules that need to be rendered in reverse order.
 * @param module Pointer to the first module in the list
 * @return Pointer to the new first module (was last)
 */
StatusBarModule* InvertModulesOrder(StatusBarModule* module) {
  StatusBarModule* prev = NULL;
  StatusBarModule* current = module;
  StatusBarModule* next = NULL;

  while (current != NULL) {
    next = current->next; /* Store next node */
    current->next = prev; /* Reverse current node's pointer */
    prev = current;       /* Move pointers one position ahead */
    current = next;
  }

  return prev; /* New head of the inverted list */
}

/**
 * ---------------------------------------------------------------------------
 * Module Callbacks
 * ---------------------------------------------------------------------------
 */

/**
 * Input mode module update function.
 * Displays the current input mode (NORMAL, INSERT, VISUAL) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void InputModeModule(AppData* app, StatusBarModule* module, Panel* panel) {
  (void)app;
  if (module == NULL || panel == NULL) return;

  IconType icon_type = GetConfigIconType();
  char* mode = (char*)"NORMAL";
  char* icon = (char*)"";
  int color = COLOR_BLUE;

  switch (panel->mode) {
    case DEFAULT:
      if (app->notes && app->notes->is_move_mode) {
        icon = (char*)DEFAULT_MODE_ICONS[icon_type];
        color = COLOR_YELLOW;
        mode = (char*)"V-LINE";
      } else {
        mode = (char*)"DEFAULT";
        icon = (char*)DEFAULT_MODE_ICONS[icon_type];
        color = COLOR_MAGENTA;
      }
      break;
    case NORMAL:
      mode = (char*)"NORMAL";
      icon = (char*)NORMAL_MODE_ICONS[icon_type];
      color = COLOR_BLUE;
      break;
    case INSERT:
      mode = (char*)"INSERT";
      icon = (char*)INSERT_MODE_ICONS[icon_type];
      color = COLOR_GREEN;
      break;
    case VISUAL:
      mode = (char*)"VISUAL";
      icon = (char*)VISUAL_MODE_ICONS[icon_type];
      color = COLOR_YELLOW;
      break;
    default:
      mode = (char*)"UNKNOWN";
      icon = (char*)"?";
      color = COLOR_RED;
      break;
  }

  module->bg_color = color;
  module->fg_color = COLOR_BLACK;

  int required_length = 0;
  if (strlen(icon) < 1) {
    required_length = snprintf(NULL, 0, "%s", mode) + 1;
    snprintf(module->content, required_length, "%s", mode);
  } else {
    required_length = snprintf(NULL, 0, "%s %s", icon, mode) + 1;
    snprintf(module->content, required_length, "%s %s", icon, mode);
  }
  module->content_length = UTF16CharCount(module->content);
}

/**
 * Real-time module update function.
 * Displays the current time in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void RealTimeModule(AppData* app, StatusBarModule* module, Panel* panel) {
  (void)app;
  if (module == NULL || panel == NULL) return;

  IconType icon_type = GetConfigIconType();
  const char* icon = (char*)REAL_TIME_MODULE_ICONS[icon_type];

  int color = COLOR_CYAN;
  char current_time[6]; /* "HH:MM" + null terminator */
  GetCurrentTime(current_time, sizeof(current_time));

  module->bg_color = color;
  module->fg_color = COLOR_BLACK;

  if (strlen(icon) < 1) {
    int required_length = snprintf(NULL, 0, "%s", current_time) + 1;
    snprintf(module->content, required_length, "%s", current_time);
  } else {
    int required_length = snprintf(NULL, 0, "%s %s", icon, current_time) + 1;
    snprintf(module->content, required_length, "%s %s", icon, current_time);
  }
  module->content_length = UTF16CharCount(module->content);
}

/**
 * Scene module update function.
 * Displays the current scene type (WORK, PAUSE, etc.) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void SceneModule(AppData* app, StatusBarModule* module, Panel* panel) {
  if (module == NULL || panel == NULL) return;

  int color = COLOR_BLACK;
  char* content = (char*)"";
  IconType icon_type = GetConfigIconType();
  const char* icon;

  Panel current_panel = app->screen->panels[app->screen->current_panel];
  switch (current_panel.scene_history->present) {
    case MAIN_MENU:
      icon = (char*)MAIN_MENU_ICONS[icon_type];
      color = COLOR_MAGENTA;
      content = (char*)"MAIN MENU";
      break;
    case WORK_TIME:
      icon = (char*)WORK_ICONS[icon_type];
      color = COLOR_RED;
      content = (char*)"WORK TIME";
      break;
    case SHORT_PAUSE:
      icon = (char*)SHORT_PAUSE_ICONS[icon_type];
      color = COLOR_BLUE;
      content = (char*)"SHORT PAUSE";
      break;
    case LONG_PAUSE:
      icon = (char*)LONG_PAUSE_ICONS[icon_type];
      color = COLOR_CYAN;
      content = (char*)"LONG PAUSE";
      break;
    case NOTES:
      icon = (char*)NOTES_ICONS[icon_type];
      color = COLOR_YELLOW;
      content = (char*)"NOTES";
      break;
    case HELP:
      icon = (char*)HELP_ICONS[icon_type];
      color = COLOR_RED;
      content = (char*)"HELP";
      break;
    case CONTINUE:
      icon = (char*)CONTINUE_ICONS[icon_type];
      color = COLOR_WHITE;
      content = (char*)"CONTINUE";
      break;
    default:
      icon = (char*)"?";
      color = COLOR_RED;
      break;
  }

  module->bg_color = color;
  module->fg_color = COLOR_BLACK;

  if (strlen(icon) < 1) {
    int required_length = snprintf(NULL, 0, "%s", content) + 1;
    snprintf(module->content, required_length, "%s", content);
  } else {
    int required_length = snprintf(NULL, 0, "%s %s", icon, content) + 1;
    snprintf(module->content, required_length, "%s %s", icon, content);
  }
  module->content_length = UTF16CharCount(module->content);
}

/**
 * Current status module update function.
 * Displays the pomodoro status (time remaining, cycle count) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void CurrentStatusModule(AppData* app, StatusBarModule* module, Panel* panel) {
  if (module == NULL || panel == NULL) return;

  /* Access the Pomodoro state from the app */
  PomodoroData pomodoro = app->pomodoro_data;
  int current_step = pomodoro.current_step;
  int current_cycle = pomodoro.current_cycle + 1;
  int total_cycles = pomodoro.total_cycles;

  /* Set default values for the icon and content */
  int color = COLOR_BLACK;
  char content[50];
  IconType icon_type = GetConfigIconType();
  const char* icon;
  icon = "?";
  color = COLOR_WHITE;

  switch (current_step) {
    case WORK_TIME:
      snprintf(content, 50, "%02d/%02d", current_cycle, total_cycles);
      icon = (char*)WORK_ICONS[icon_type];
      color = COLOR_MAGENTA;
      break;
    case SHORT_PAUSE:
      snprintf(content, 50, "%02d/%02d", current_cycle, total_cycles);
      icon = (char*)SHORT_PAUSE_ICONS[icon_type];
      color = COLOR_BLUE;
      break;
    case LONG_PAUSE:
      snprintf(content, 50, "%02d/%02d", current_cycle, total_cycles);
      icon = (char*)LONG_PAUSE_ICONS[icon_type];
      color = COLOR_CYAN;
      break;
    default:
      snprintf(content, 50, "IDLE");
      icon = (char*)IDLE_ICONS[icon_type];
      color = COLOR_YELLOW;
      break;
  }

  module->bg_color = color;
  module->fg_color = COLOR_BLACK;

  if (strlen(icon) < 1) {
    int required_length = snprintf(NULL, 0, "%s", content) + 1;
    snprintf(module->content, required_length, "%s", content);
  } else {
    int required_length =
      snprintf(NULL, 0, "%d %s %s", current_step, icon, content) + 1;
    snprintf(module->content, required_length, "%s %s", icon, content);
  }

  module->content_length = UTF16CharCount(module->content);
}

/**
 * Line and column module update function for NOTES scene.
 * Displays the current cursor line and column in the notes editor.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void LineColumnModule(AppData* app, StatusBarModule* module, Panel* panel) {
  if (module == NULL || panel == NULL) return;
  (void)panel;

  module->fg_color = COLOR_BLACK;
  module->bg_color = COLOR_GREEN;

  /* Only show in NOTES scene */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(SCENE_NOTES & (1 << current_scene))) {
    module->content[0] = '\0';
    module->content_length = 0;
    return;
  }

  int line = 0;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  int col = (input && (input->len > 0 || current_mode == INSERT))
              ? input->cursor + 1
              : 0;
  bool show_col = false;

  if (input && input->len > 0)
    show_col = true;
  else {
    if (current_mode == INSERT || current_mode == VISUAL) show_col = true;
  }

  if (app->notes != NULL) {
    if (app->notes->count > 0) {
      if (app->notes->current_id < 0) {
        int input_lines = 0;
        int input_indent = 0;
        if (current_mode == INSERT && input && input->insert_after_id >= 0) {
          for (int i = 0; i < app->notes->count; i++) {
            if (app->notes->items[i]->id == input->insert_after_id) {
              input_indent = app->notes->items[i]->depth * 2;
              break;
            }
          }
        }
        if (current_mode == INSERT && input && input->len > 0) {
          const char* input_prefix = input->is_task ? "[ ] " : " - ";
          int input_prefix_len = (int)strlen(input_prefix);
          int input_wrap_width =
            app->notes->render_width - input_prefix_len - input_indent;
          if (input_wrap_width <= 0) input_wrap_width = 1;
          input_lines = GetNoteLinesFromText(input->buffer, input_wrap_width);
        }
        if (input_lines > 1)
          line = app->notes->total_lines + input_lines;
        else
          line = app->notes->total_lines + 1;
      } else {
        line = 0;
        for (int i = 0; i < app->notes->count; i++) {
          const char* prefix;
          switch (app->notes->items[i]->state) {
            case NOTE_DONE:
            case NOTE_UNDONE:
              prefix = "[ ] ";
              break;
            case NOTE_PLAIN:
            default:
              prefix = " - ";
              break;
          }
          int prefix_len = (int)strlen(prefix);
          int depth = app->notes->items[i]->depth;
          int indent = depth * 2;
          int item_wrap_width = app->notes->render_width - prefix_len - indent;
          if (item_wrap_width <= 0) item_wrap_width = 1;
          if (app->notes->items[i]->id == app->notes->current_id) {
            line += GetNoteLines(app->notes->items[i], item_wrap_width);
            break;
          }
          line += GetNoteLines(app->notes->items[i], item_wrap_width);
        }
      }
    } else if (current_mode == INSERT) {
      int input_lines = 0;
      int input_indent = 0;
      if (input && input->insert_after_id >= 0) {
        for (int i = 0; i < app->notes->count; i++) {
          if (app->notes->items[i]->id == input->insert_after_id) {
            input_indent = app->notes->items[i]->depth * 2;
            break;
          }
        }
      }
      if (input && input->len > 0) {
        const char* input_prefix = input->is_task ? "[ ] " : " - ";
        int input_prefix_len = (int)strlen(input_prefix);
        int input_wrap_width =
          app->notes->render_width - input_prefix_len - input_indent;
        if (input_wrap_width <= 0) input_wrap_width = 1;
        input_lines = GetNoteLinesFromText(input->buffer, input_wrap_width);
      }
      if (input_lines > 1)
        line = input_lines;
      else
        line = 1;
    }
  }

  int required_length;
  IconType icon_type = GetConfigIconType();
  const char* icon = (char*)LINE_COLUMN_MODULE_ICONS[icon_type];
  if (show_col) {
    if (strlen(icon) < 1) {
      required_length = snprintf(NULL, 0, "Ln %d, Col %d", line, col) + 1;
      snprintf(module->content, required_length, "Ln %d, Col %d", line, col);
    } else {
      required_length =
        snprintf(NULL, 0, "%s Ln %d, Col %d", icon, line, col) + 1;
      snprintf(module->content, required_length, "%s Ln %d, Col %d", icon, line,
               col);
    }
  } else {
    if (strlen(icon) < 1) {
      required_length = snprintf(NULL, 0, "Ln %d", line) + 1;
      snprintf(module->content, required_length, "Ln %d", line);
    } else {
      required_length = snprintf(NULL, 0, "%s Ln %d", icon, line) + 1;
      snprintf(module->content, required_length, "%s Ln %d", icon, line);
    }
  }
  module->content_length = UTF16CharCount(module->content);
}
