#include "input.h"

#include <ncurses.h>

#include "draw.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

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
    else
      app->user_input = -1;
  } else if (!app->block_input)
    ProcessKeyInput(app, app->user_input);

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
  if (app->popup_dialog != NULL) return;
  app->screen->current_panel = (app->screen->current_panel + 1) % MAX_PANELS;
  app->current_menu =
    app->screen->panels[app->screen->current_panel].menu_index;
}

/* Select next menu item */
void SelectNextItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], 1);
}

/* Select previous menu item */
void SelectPreviousItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], -1);
}

/* Toggle pause */
void TogglePause(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->is_paused = !app->is_paused;
}

/* Change the input mode */
void ChangeMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  int* mode = &app->screen->panels[app->screen->current_panel].mode;

  if (*mode == VISUAL)
    *mode = NORMAL;
  else
    *mode <<= 1;
}

/* Update animation mode */
void ChangeDebugAnimation(AppData* app, int step) {
  if (app->popup_dialog != NULL) return;
  int* present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}

/* Quit the program */
void QuitApp(AppData* app) {
  if (app->user_input == app->last_input) app->running = false;
}

/* Quit the program forcefully */
void ForcefullyQuitApp(AppData* app) { app->running = false; }

/* Close the popup dialog */
void ClosePopup(AppData* app) {
  if (app->popup_dialog != NULL) FreeFloatingDialog(app->popup_dialog);

  app->popup_dialog = NULL;
}

/* Start pomodoro cycle */
void StartPomodoro(AppData* app) {
  ExecuteHistory(app->screen->panels[0].scene_history, WORK_TIME);
  app->screen->panels[0].menu_index = -1;
  app->pomodoro_data.current_step = WORK_TIME;
  app->pomodoro_data.current_cycle = 0;
  app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
  app->pomodoro_data.current_step_time = 0;

  Notification notification = {
    .title = " Work!",
    .description = "You need to focus",
    .audio_path = "./sounds/dfltnotify.mp3",
  };
  Notify(&notification);
}

/* Open the reset pomodoro menu */
void OpenResetMenu(AppData* app) { RenderResetMenu(app); }

/* Reset pomodoro step */
void ResetPomodoroStep(AppData* app) {
  app->pomodoro_data.current_step_time = 0;
}

/* Reset pomodoro cycle */
void ResetPomodoroCycle(AppData* app) { StartPomodoro(app); }

/* Skip pomodoro step */
void SkipPomodoroStep(AppData* app) {
  if (app->user_input != app->last_input) return;
  ResetInput(app);
  ClosePopup(app);

  int step = app->pomodoro_data.current_step;
  int duration;

  switch (step) {
    case WORK_TIME:
      duration = app->pomodoro_data.work_time;
      break;
    case SHORT_PAUSE:
      duration = app->pomodoro_data.short_pause_time;
      break;
    case LONG_PAUSE:
      duration = app->pomodoro_data.long_pause_time;
      break;
    default:
      return;
  }

  app->pomodoro_data.current_step_time = duration * 60;
}

/* Forcefully skip pomodoro step */
void ForcefullySkipPomodoroStep(AppData* app) {
  int step = app->pomodoro_data.current_step;
  int duration;

  switch (step) {
    case WORK_TIME:
      duration = app->pomodoro_data.work_time;
      break;
    case SHORT_PAUSE:
      duration = app->pomodoro_data.short_pause_time;
      break;
    case LONG_PAUSE:
      duration = app->pomodoro_data.long_pause_time;
      break;
    default:
      return;
  }

  app->pomodoro_data.current_step_time = duration * 60;
}

/* Function to execute the action of the selected menu item */
void ExecuteMenuAction(AppData* app) {
  if (app->popup_dialog != NULL) {
    Menu* current_menu = &app->popup_dialog->menu;
    int selected_index = current_menu->selected_item;
    MenuAction action = current_menu->items[selected_index].action;

    if (action) action(app);
    ResetInput(app);
    ClosePopup(app);
    return;
  }
  if (app->current_menu == -1) return;

  Menu* current_menu = app->menus[app->current_menu];
  int selected_index = current_menu->selected_item;
  MenuAction action = current_menu->items[selected_index].action;

  if (action) action(app);
  ResetInput(app);
}
