#include "input.h"

#include <ncurses.h>
#include <string.h>

#include "config.h"
#include "draw.h"
#include "notes.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* Function to process key input */
void ProcessKeyInput(AppData* app, int key) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  int effective_mode = current_mode;

  /* When in NORMAL mode with input, use NORMAL_WITH_INPUT for key matching */
  if (current_mode == NORMAL && input_len > 0)
    effective_mode = NORMAL_WITH_INPUT;

  /* Handle printable characters in INSERT mode via special entry */
  if (effective_mode == INSERT && key >= ' ' && key <= '~') {
    for (size_t i = 0; i < numKeyFunctions; i++) {
      if (keys[i].key == -1 && (keys[i].modes & effective_mode) &&
          (keys[i].scene_types & (1 << current_scene))) {
        keys[i].action(app);
        return;
      }
    }
  }

  /* Normal lookup */
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key && (keys[i].modes & effective_mode) &&
        (keys[i].scene_types & (1 << current_scene))) {
      keys[i].action(app);
      return;
    }
  }
}

/* Check if the given key is assigned to the specified action function */
int IsKeyAssignedToAction(int key, void (*action)(AppData*)) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++)
    if (keys[i].key == key && keys[i].action == action) return 1;
  return 0;
}

/* Handle user input and app state */
ErrorType HandleInputs(AppData* app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  if (app->user_input != -1) app->last_input = app->user_input;
  app->user_input = getch();

  int key = app->user_input;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;

  /* If popup is showing, handle popup navigation keys directly */
  if (app->popup_dialog != NULL) {
    if (key == KEY_UP || key == 'k' || key == KEY_LEFT || key == 'h') {
      ChangeSelectedItem(&app->popup_dialog->menu, -1);
      flushinp();
      return status;
    } else if (key == KEY_DOWN || key == 'j' || key == KEY_RIGHT ||
               key == 'l') {
      ChangeSelectedItem(&app->popup_dialog->menu, 1);
      flushinp();
      return status;
    } else if (key == KEY_ENTER || key == '\n' || key == '\r') {
      ExecuteMenuAction(app);
      flushinp();
      return status;
    }
    /* For other keys (like q, ESC), let them pass through to ProcessKeyInput */
  }

  /* Dispatch to mode-specific handler */
  if (current_mode == INSERT || current_mode == VISUAL) {
    ProcessKeyInput(app, key);
    flushinp();
    return status;
  } else if (current_mode == NORMAL) {
    status = HandleNormalMode(app, key);
  }

  flushinp();

  return status;
}

/* Handle NORMAL mode - process key bindings */
ErrorType HandleNormalMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* Handle input buffer editing if there's content */
  if (input_len > 0) {
    ProcessKeyInput(app, key);
    return status;
  }

  if (!CheckScreenSize(app)) {
    if (IsKeyAssignedToAction(key, QuitApp))
      ProcessKeyInput(app, key);
    else
      app->user_input = -1;
  } else if (!app->block_input)
    ProcessKeyInput(app, key);

  return status;
}

/* Handle INSERT mode - now handled via keys[] */
ErrorType HandleInsertMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/* Handle VISUAL mode - now handled via keys[] */
ErrorType HandleVisualMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/* Buffer for INSERT mode text input */
char input_buffer[256];
int input_len = 0;        /* Actual length of input */
int input_cursor_pos = 0; /* Cursor position in input buffer (0 to input_len) */
int visual_start = 0;     /* Start position for VISUAL mode selection */
int input_mode_type = 0;  /* 0 = task, 1 = note */

/* New action functions for keybindings */
void InputCursorLeft(AppData* app) {
  (void)app;
  if (input_cursor_pos > 0) input_cursor_pos--;
}

void InputCursorRight(AppData* app) {
  (void)app;
  if (input_cursor_pos < input_len) input_cursor_pos++;
}

void InputBackspace(AppData* app) {
  (void)app;
  if (input_cursor_pos > 0) {
    for (int i = input_cursor_pos - 1; i < input_len - 1; i++)
      input_buffer[i] = input_buffer[i + 1];
    input_cursor_pos--;
    input_len--;
    input_buffer[input_len] = '\0';
  }
}

void InputDeleteChar(AppData* app) {
  (void)app;
  if (input_cursor_pos < input_len) {
    for (int i = input_cursor_pos; i < input_len - 1; i++)
      input_buffer[i] = input_buffer[i + 1];
    input_len--;
    input_buffer[input_len] = '\0';
    if (input_cursor_pos > input_len) input_cursor_pos = input_len;
  }
  if (input_len == 0) {
    input_cursor_pos = 0;
    input_buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  }
}

void InputVisualDelete(AppData* app) {
  int start_sel =
    (visual_start < input_cursor_pos) ? visual_start : input_cursor_pos;
  int end_sel =
    (visual_start < input_cursor_pos) ? input_cursor_pos : visual_start;
  end_sel++;
  int sel_len = end_sel - start_sel;
  if (sel_len > 0 && start_sel < input_len) {
    for (int i = start_sel; i < input_len - sel_len; i++)
      input_buffer[i] = input_buffer[i + sel_len];
    input_len -= sel_len;
    input_buffer[input_len] = '\0';
    input_cursor_pos = start_sel;
    visual_start = start_sel;
  }
  if (input_len == 0) {
    input_len = 0;
    input_cursor_pos = 0;
    input_buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  } else {
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    refresh();
  }
}

void InputCommit(AppData* app) {
  /* Commit if there's text, otherwise cancel (select last note) */
  if (input_len == 0) {
    app->notes->current = app->notes->tail;
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
    return;
  }

  input_buffer[input_len] = '\0';
  if (input_mode_type == 0)
    AddNote(app->notes, input_buffer, true);
  else
    AddNote(app->notes, input_buffer, false);
  app->notes->current = app->notes->tail;
  input_len = 0;
  input_cursor_pos = 0;
  input_buffer[0] = '\0';
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
  app->user_input = -1;
  app->last_input = -1;
  move(LINES - 1, 0);
  clrtoeol();
  refresh();
}

void InputESC(AppData* app) {
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == INSERT) {
    if (input_len == 0) {
      input_len = 0;
      input_cursor_pos = 0;
      input_buffer[0] = '\0';
      app->notes->current = app->notes->tail;
    }
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    app->user_input = -1;
    app->last_input = -1;
  } else if (current_mode == VISUAL) {
    /* Switch to NORMAL, select last note */
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    app->notes->current = app->notes->tail;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  } else if (current_mode == NORMAL) {
    /* Clear input state, select last note */
    input_len = 0;
    input_cursor_pos = 0;
    input_buffer[0] = '\0';
    app->notes->current = app->notes->tail;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  }
}

void InputInsertChar(AppData* app) {
  int key = app->user_input;
  if (key >= ' ' && key <= '~' && input_len < (int)sizeof(input_buffer) - 1) {
    for (int i = input_len; i > input_cursor_pos; i--)
      input_buffer[i] = input_buffer[i - 1];
    input_buffer[input_cursor_pos] = key;
    input_cursor_pos++;
    input_len++;
    input_buffer[input_len] = '\0';
  }
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
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
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

/* Switch to INSERT mode (vim 'i' key) */
void SwitchToInsertMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/* Switch to VISUAL mode (vim 'v' key) */
void SwitchToVisualMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = VISUAL;
  visual_start = input_cursor_pos;
}

/* Switch to NORMAL mode (ESC key) */
void SwitchToNormalMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
  /* Clear input state to prevent quit popup from appearing */
  app->user_input = -1;
  app->last_input = -1;
}

/* Update animation mode */
void ChangeDebugAnimation(AppData* app, int step) {
  if (app->popup_dialog != NULL) return;
  int* present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}

/* Quit the program - requires double press of same key */
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
void OpenResetMenu(AppData* app) {
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
  RenderResetMenu(app);
}

/* Reset pomodoro step */
void ResetPomodoroStep(AppData* app) {
  app->pomodoro_data.current_step_time = 0;
}

/* Reset pomodoro cycle */
void ResetPomodoroCycle(AppData* app) { StartPomodoro(app); }

/* Skip pomodoro step */
void SkipPomodoroStep(AppData* app) {
  if (app->user_input != app->last_input) return;
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;

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
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;

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

/* Notes keybinding functions */
void ToggleTaskAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  ToggleTask(app->notes);
}

void DeleteNoteAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  DeleteNote(app->notes);
}

void AddNewNote(AppData* app) {
  if (app->popup_dialog != NULL) return;

  /* Clear selection - unselect all notes */
  app->notes->current = NULL;

  /* Switch to INSERT mode - HandleInputs will handle the text input */
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  input_len = 0;
  input_cursor_pos = 0;
  input_buffer[0] = '\0';
  input_mode_type = 0; /* Task */

  /* Show prompt */
  echo();
  curs_set(1);
  mvprintw(LINES - 1, 0, "New Task: ");
  clrtoeol();
  refresh();
}

void AddNewNoteItem(AppData* app) {
  if (app->popup_dialog != NULL) return;

  /* Clear selection - unselect all notes */
  app->notes->current = NULL;

  /* Switch to INSERT mode - HandleInputs will handle the text input */
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  input_len = 0;
  input_cursor_pos = 0;
  input_buffer[0] = '\0';
  input_mode_type = 1; /* Note */

  /* Show prompt */
  echo();
  curs_set(1);
  mvprintw(LINES - 1, 0, "New Note: ");
  clrtoeol();
  refresh();
}

void InputSwitchToInsertFromVisual(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  noecho();
  curs_set(1);
  refresh();
}
