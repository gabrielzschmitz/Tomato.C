#include "bar.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "tomato.h"

/* Function to create and allocate a StatusBarModule */
StatusBarModule* CreateStatusBarModule(StatusBarModulePosition position,
                                       char* content, int fg_color,
                                       int bg_color, ModuleUpdate update) {
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

/* Function to add a StatusBarModule to the end of the linked list by values */
void AddStatusBarModule(StatusBar* status_bar, StatusBarModulePosition position,
                        ModuleUpdate update) {
  if (status_bar == NULL) return;

  char* default_content = (char*)"default_content";
  int default_fg_color = COLOR_WHITE;
  int default_bg_color = COLOR_BLACK;

  StatusBarModule* new_widget = CreateStatusBarModule(
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

/* Function to free a StatusBarModule */
void FreeStatusBarModule(StatusBarModule* widget) {
  if (widget == NULL) return;
  if (widget->content != NULL) free(widget->content);
  free(widget);
}

/* Function to free a linked list of StatusBarModule modules */
void FreeStatusBarModules(StatusBarModule* module) {
  StatusBarModule* current = module;
  StatusBarModule* next;

  while (current) {
    next = current->next;
    FreeStatusBarModule(current);
    current = next;
  }
}

/* Function to create and allocate a StatusBar */
StatusBar* CreateStatusBar(StatusBarPosition position) {
  StatusBar* bar = (StatusBar*)malloc(sizeof(StatusBar));
  if (bar == NULL) return NULL;

  bar->position = position;
  bar->left_modules = NULL;
  bar->center_modules = NULL;
  bar->right_modules = NULL;

  return bar;
}

/* Function to free a StatusBar */
void FreeStatusBar(StatusBar* bar) {
  if (bar == NULL) return;

  FreeStatusBarModules(bar->left_modules);
  FreeStatusBarModules(bar->center_modules);
  FreeStatusBarModules(bar->right_modules);

  free(bar);
}

void RenderStatusBarModule(const StatusBarModule* module, int start_y,
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

void RenderStatusBar(const StatusBar* status_bar, const Screen* screen) {
  if (status_bar == NULL || screen == NULL) return;

  int bar_width = screen->size.width;
  int bar_y = (status_bar->position == TOP) ? 0 : screen->size.height - 1;

  /* Fill background */
  SetColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
  for (int i = 0; i < bar_width; i++) mvprintw(bar_y, i, " ");

  /* Render LEFT modules */
  StatusBarModule* current = status_bar->left_modules;
  int x_offset = 0;
  while (current != NULL) {
    RenderStatusBarModule(current, bar_y, x_offset,
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
    RenderStatusBarModule(current, bar_y, x_offset, bar_width - x_offset);
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
    RenderStatusBarModule(current, bar_y, x_offset + STATUS_BAR_SPACING,
                          bar_width - x_offset + STATUS_BAR_SPACING);
    x_offset += current->content_length + STATUS_BAR_SPACING;
    current = current->next;
  }
}

/* Update status bar module */
void UpdateStatusBarModule(AppData* app, StatusBarModule* module,
                           Panel* current_panel) {
  if (module && module->update) module->update(app, module, current_panel);
}

/* Update status bar */
void UpdateStatusBar(AppData* app, StatusBar* status_bar,
                     Panel* current_panel) {
  if (status_bar == NULL || current_panel == NULL) return;

  StatusBarModule* current;

  /* Update LEFT modules */
  current = status_bar->left_modules;
  while (current != NULL) {
    UpdateStatusBarModule(app, current, current_panel);
    current = current->next;
  }

  /* Update CENTER modules */
  current = status_bar->center_modules;
  while (current != NULL) {
    UpdateStatusBarModule(app, current, current_panel);
    current = current->next;
  }

  /* Update RIGHT modules */
  current = status_bar->right_modules;
  while (current != NULL) {
    UpdateStatusBarModule(app, current, current_panel);
    current = current->next;
  }
}

/* Inverts the order of a linked list of StatusBarModule */
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

/* Input mode module to status bar */
void InputModeModule(AppData* app, StatusBarModule* module, Panel* panel) {
  (void)app;
  if (module == NULL || panel == NULL) return;

  IconType icon_type = GetConfigIconType();
  char* mode = (char*)"NORMAL";
  char* icon = (char*)"";
  int color = COLOR_BLUE;

  switch (panel->mode) {
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

/* Real-time module for status bar */
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

/* Current scene module for status bar */
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

/* Current status module for status bar */
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
