#include "input.h"
#include "draw.h"

#include <ncurses.h>

/* Function to process key input */
void ProcessKeyInput(AppData* app, int key) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key &&
        (keys[i].modes &
         app->screen->panels[app->screen->current_panel].mode)) {
      keys[i].action(app);
      break;
    }
  }
}

/* Handle user input and app state */
ErrorType HandleInputs(AppData* app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  if (app->user_input != -1) app->last_input = app->user_input;
  app->user_input = getch();

  if (!CheckScreenSize(app)) {
    if (IsKeyAssignedToAction(app->user_input, QuitApp))
      ProcessKeyInput(app, app->user_input);
    else app->user_input = -1;
  } else if (!app->block_input) ProcessKeyInput(app, app->user_input);

  flushinp();

  return status;
}

/* Check if the given key is assigned to the specified action function */
int IsKeyAssignedToAction(int key, void (*action)(AppData*)) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++)
    if (keys[i].key == key && keys[i].action == action) return 1;
  return 0;
}

/* Switch to next panel */
void NextPanel(AppData* app) {
  app->screen->current_panel = (app->screen->current_panel + 1) % MAX_PANELS;
}

/* Select next menu item */
void SelectNextItem(AppData* app) {
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], 1);
}

/* Select previous menu item */
void SelectPreviousItem(AppData* app) {
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], -1);
}

/* Toggle pause */
void TogglePause(AppData* app) {
  app->is_paused = !app->is_paused;
}

/* Change the input mode */
void ChangeMode(AppData* app) {
  int* mode = &app->screen->panels[app->screen->current_panel].mode;

  if (*mode == VISUAL) *mode = NORMAL;
  else *mode <<= 1;
}

/* Update animation mode */
void ChangeDebugAnimation(AppData* app, int step) {
  int* present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}

/* Quit the program */
void QuitApp(AppData* app) {
  if (app->user_input == app->last_input)
    app->running = false;
}

/* Quit the program forcefully */
void ForcefullyQuitApp(AppData* app) {
  app->running = false;
}

/* Function to execute the action of the selected menu item */
void ExecuteMenuAction(AppData* app) {
  if (app->current_menu == -1) return;

  Menu* current_menu = app->menus[app->current_menu];
  int selected_index = current_menu->selected_item;
  MenuAction action = current_menu->items[selected_index].action;

  if (action) 
    action(app);
}
