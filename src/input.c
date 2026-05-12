#include "input.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "draw.h"
#include "error.h"
#include "notes.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/**
 * ---------------------------------------------------------------------------
 * Input Dispatching
 * ---------------------------------------------------------------------------
 */

/**
 * Process a single key input and dispatch to appropriate handler.
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 */
void ProcessKeyInput(AppData* app, int key) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  int effective_mode = current_mode;

  /* Only switch to DEFAULT if mode is NORMAL and input is empty */
  if (current_mode == NORMAL && (!input || input->len == 0))
    effective_mode = DEFAULT;

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

/**
 * Check if the given key is assigned to the specified action function.
 * @param key The key code to check
 * @param action Function pointer to compare against
 * @return 1 if key is assigned to action, 0 otherwise
 */
int IsKeyAssignedToAction(int key, void (*action)(AppData*)) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++)
    if (keys[i].key == key && keys[i].action == action) return 1;
  return 0;
}

/**
 * Handle all user input based on current app state.
 * Reads input from terminal and routes to appropriate handler.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInputs(AppData* app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  if (app->user_input != -1) app->last_input = app->user_input;
  app->user_input = getch();

  int key = app->user_input;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;

  /* Dispatch to mode-specific handler */
  switch (current_mode) {
    case DEFAULT:
      status = HandleDefaultMode(app, key);
      break;
    case NORMAL:
      status = HandleNormalMode(app, key);
      break;
    case INSERT:
      status = HandleInsertMode(app, key);
      break;
    case VISUAL:
      status = HandleVisualMode(app, key);
      break;
  }
  flushinp();

  return status;
}

/**
 * Handle input in DEFAULT mode (menu navigation, pomodoro control).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleDefaultMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* First handle popup input if popup is active */
  if (HandlePopupInput(app, key)) {
    return status; /* popup key was handled */
  }

  if (!ValidateAndRenderScreenSize(app)) {
    if (IsKeyAssignedToAction(key, QuitApp))
      ProcessKeyInput(app, key);
    else
      app->user_input = -1;
  } else if (!app->block_input)
    ProcessKeyInput(app, key);

  return status;
}

/**
 * Handle input in NORMAL mode (text navigation, note editing).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleNormalMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  Panel* panel = &app->screen->panels[app->screen->current_panel];
  InputState* input = panel->input;

  /* If Panel.input exists with content, process editing keys */
  if (input && input->len > 0) {
    ProcessKeyInput(app, key);
    return status;
  }

  /* If input is empty in NORMAL mode, fall through to DEFAULT handling */
  if (!app->block_input) ProcessKeyInput(app, key);

  return status;
}

/**
 * Handle input in INSERT mode (text input).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInsertMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/**
 * Handle input in VISUAL mode (text selection).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleVisualMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/**
 * Handle input in while POPUP is active.
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return true if input is consumed, or false if not popup active
 */
bool HandlePopupInput(AppData* app, int key) {
  if (app->popup_dialog == NULL) return 0;

  /* When popup is active, only use keys bound to ALL_SCENES to avoid
   * scene-specific keys (like ToggleTaskAtNotes) intercepting popup input */
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
        keys[i].scene_types == ALL_SCENES) {
      keys[i].action(app);
      return true; /* handled */
    }
  }
  /* For other keys (like q, ESC), let them pass through */
  return false;
}

/**
 * ---------------------------------------------------------------------------
 * InputState Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new InputState with default values.
 * @return Pointer to the created InputState, or NULL on allocation failure
 */
InputState* InputStateCreate(void) {
  InputState* s = (InputState*)malloc(sizeof(InputState));
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->max_len = sizeof(s->buffer) - 1;
    s->is_task = true;
    s->pending_parent_id = -1;
    s->insert_after_id = -1;
    s->selection.start = 0;
    s->selection.end = 0;
  }
  return s;
}

/**
 * Destroy an InputState and free its memory.
 * @param input Pointer to the InputState pointer to free
 */
void InputStateDestroy(InputState** input) {
  if (input && *input) {
    free(*input);
    *input = NULL;
  }
}

/**
 * Clear the InputState contents, resetting cursor and buffer.
 * @param s Pointer to the InputState to clear
 */
void InputStateClear(InputState* s) {
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->selection.start = 0;
    s->selection.end = 0;
  }
}

/**
 * ---------------------------------------------------------------------------
 * Mode Management
 * ---------------------------------------------------------------------------
 */

/**
 * Set the input mode of a panel, updating the panel's mode field.
 * Centralized mode transition that handles mode switching logic.
 * @param panel Pointer to the panel to update
 * @param mode The new input mode (DEFAULT, NORMAL, INSERT, VISUAL)
 */
void InputSetMode(Panel* panel, InputMode mode) {
  panel->mode = mode;

  if (panel->input) {
    int max = (mode == INSERT)
                ? panel->input->len
                : (panel->input->len > 0 ? panel->input->len - 1 : 0);
    if (panel->input->cursor > max) {
      panel->input->cursor = max;
    }

    if (mode == INSERT) {
      echo();
      curs_set(1);
    } else {
      noecho();
      curs_set(0);
    }

    if (mode != VISUAL) {
      panel->input->selection.start = 0;
      panel->input->selection.end = 0;
    }
  }
}

/**
 * Switch to INSERT mode from current position.
 * @param app Pointer to the application data
 */
void SwitchToInsertMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Switch to INSERT mode after current cursor position.
 * @param app Pointer to the application data
 */
void SwitchToInsertModeAppend(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor < input->len) input->cursor++;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Enter VISUAL mode for text selection.
 * @param app Pointer to the application data
 */
void SwitchToVisualMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  app->screen->panels[app->screen->current_panel].mode = VISUAL;
  if (input) input->selection.start = input->cursor;
}

/**
 * Exit to NORMAL mode (ESC key handler).
 * @param app Pointer to the application data
 */
void SwitchToNormalMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
  /* Clear input state to prevent quit popup from appearing */
  app->user_input = -1;
  app->last_input = -1;
}

/**
 * ---------------------------------------------------------------------------
 * Editor Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move cursor one position to the left.
 * @param app Pointer to the application data
 */
void InputCursorLeft(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor > 0) input->cursor--;
}

/**
 * Move cursor one position to the right.
 * @param app Pointer to the application data
 */
void InputCursorRight(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  int mode = app->screen->panels[app->screen->current_panel].mode;
  if (mode == INSERT) {
    if (input->cursor < input->len) input->cursor++;
  } else {
    if (input->len > 0 && input->cursor < input->len - 1) input->cursor++;
  }
}

/**
 * Delete character before cursor (backspace).
 * @param app Pointer to the application data
 */
void InputBackspace(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  if (input->cursor > 0) {
    for (int i = input->cursor - 1; i < input->len - 1; i++)
      input->buffer[i] = input->buffer[i + 1];
    input->cursor--;
    input->len--;
    input->buffer[input->len] = '\0';
  }
}

/**
 * Delete character at cursor (in NORMAL mode).
 * @param app Pointer to the application data
 */
void InputDeleteChar(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  SaveNotesToHistory(app->notes, input->cursor);

  if (input->cursor < input->len) {
    for (int i = input->cursor; i < input->len - 1; i++)
      input->buffer[i] = input->buffer[i + 1];
    input->len--;
    input->buffer[input->len] = '\0';
    int mode = app->screen->panels[app->screen->current_panel].mode;
    int max_cursor =
      (mode == INSERT) ? input->len : (input->len > 0 ? input->len - 1 : 0);
    if (input->cursor > max_cursor) input->cursor = max_cursor;
  }
  if (input->len == 0) {
    input->cursor = 0;
    input->buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  }
}

/**
 * Delete character(s) in visual selection.
 * @param app Pointer to the application data
 */
void InputVisualDelete(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  SaveNotesToHistory(app->notes, input->cursor);

  int start_sel = (input->selection.start < input->cursor)
                    ? input->selection.start
                    : input->cursor;
  int end_sel = (input->selection.start < input->cursor)
                  ? input->cursor
                  : input->selection.start;
  end_sel++;
  int sel_len = end_sel - start_sel;
  if (sel_len > 0 && start_sel < input->len) {
    for (int i = start_sel; i < input->len - sel_len; i++)
      input->buffer[i] = input->buffer[i + sel_len];
    input->len -= sel_len;
    input->buffer[input->len] = '\0';
    input->cursor = start_sel;
    input->selection.start = start_sel;
    int mode = app->screen->panels[app->screen->current_panel].mode;
    int max_cursor =
      (mode == INSERT) ? input->len : (input->len > 0 ? input->len - 1 : 0);
    if (input->cursor > max_cursor) input->cursor = max_cursor;
  }
  if (input->len == 0) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  } else {
    if (input->cursor > 0 && input->cursor > input->len - 1)
      input->cursor = (input->len > 0) ? input->len - 1 : 0;
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    refresh();
  }
}

/**
 * Commit current input (return/enter key).
 * Finalizes text entry or confirms selection.
 * @param app Pointer to the application data
 */
void InputCommit(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input || input->len == 0) {
    if (app->notes->count > 0 && app->notes->current_id < 0)
      app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
    app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
    return;
  }

  input->buffer[input->len] = '\0';
  NoteState state;
  if (app->notes->current_id >= 0) {
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    state = note ? note->state : (input->is_task ? NOTE_UNDONE : NOTE_PLAIN);
  } else
    state = input->is_task ? NOTE_UNDONE : NOTE_PLAIN;

  if (app->notes->current_id >= 0) {
    app->notes->last_affected_id = app->notes->current_id;
    UpdateNote(app->notes, app->notes->current_id, input->buffer, state);
  } else if (input->insert_after_id >= 0) {
    app->notes->last_affected_id = -1;
    AddNoteAfter(app->notes, input->insert_after_id, input->buffer, state);
    app->notes->last_affected_id = app->notes->current_id;
  } else if (input->pending_parent_id >= 0) {
    app->notes->last_affected_id = input->pending_parent_id;
    AddChildNote(app->notes, input->pending_parent_id, input->buffer, state);
    app->notes->last_affected_id = app->notes->current_id;
  } else {
    app->notes->last_affected_id = -1;
    AddNote(app->notes, input->buffer, state);
    app->notes->last_affected_id = app->notes->current_id;
  }
  input->len = 0;
  input->cursor = 0;
  input->buffer[0] = '\0';
  input->is_task = true;
  input->pending_parent_id = -1;
  input->insert_after_id = -1;
  app->screen->panels[app->screen->current_panel].mode = DEFAULT;
  app->user_input = -1;
  app->last_input = -1;
  move(LINES - 1, 0);
  clrtoeol();
  refresh();
}

/**
 * Handle escape key - exit to DEFAULT mode or close dialog.
 * @param app Pointer to the application data
 */
void InputESC(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == INSERT) {
    if (input && input->cursor > 0 && input->cursor > input->len - 1)
      input->cursor = (input->len > 0) ? input->len - 1 : 0;
    if (!input || input->len == 0) {
      if (input) {
        input->len = 0;
        input->cursor = 0;
        input->buffer[0] = '\0';
        input->pending_parent_id = -1;
      }
      if (app->notes->count > 0)
        app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
      app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    } else {
      app->screen->panels[app->screen->current_panel].mode = NORMAL;
    }
    noecho();
    curs_set(0);
    app->user_input = -1;
    app->last_input = -1;
  } else if (current_mode == VISUAL) {
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  } else if (current_mode == NORMAL) {
    if (input) {
      input->len = 0;
      input->cursor = 0;
      input->buffer[0] = '\0';
    }
    app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  }
}

/**
 * Insert a printable character at cursor position.
 * @param app Pointer to the application data
 */
void InputInsertChar(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  int key = app->user_input;
  if (key >= ' ' && key <= '~' && input->len < input->max_len) {
    for (int i = input->len; i > input->cursor; i--)
      input->buffer[i] = input->buffer[i - 1];
    input->buffer[input->cursor] = key;
    input->cursor++;
    input->len++;
    input->buffer[input->len] = '\0';
  }
}

/**
 * Switch from VISUAL mode to INSERT mode, keeping selection.
 * @param app Pointer to the application data
 */
void InputSwitchToInsertFromVisual(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  noecho();
  curs_set(1);
  refresh();
}

/**
 * ---------------------------------------------------------------------------
 * App Control Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Switch focus to the next panel.
 * @param app Pointer to the application data
 */
void NextPanel(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->current_panel = (app->screen->current_panel + 1) % MAX_PANELS;
  app->current_menu =
    app->screen->panels[app->screen->current_panel].menu_index;
}

/**
 * Toggle pomodoro timer pause state.
 * @param app Pointer to the application data
 */
void TogglePause(AppData* app) {
  if (app->popup_dialog != NULL) return;
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
  app->is_paused = !app->is_paused;
}

/**
 * Update animation mode for debugging (step through frames).
 * @param app Pointer to the application data
 * @param step Number of frames to advance (can be negative)
 */
void ChangeDebugAnimation(AppData* app, int step) {
  if (app->popup_dialog != NULL) return;
  int* present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}

/**
 * Quit the program with confirmation dialog.
 * @param app Pointer to the application data
 */
void QuitApp(AppData* app) {
  if (app->notes && app->notes->is_move_mode) return;
  if (app->user_input == app->last_input) app->running = false;
}

/**
 * Quit the program immediately without confirmation.
 * @param app Pointer to the application data
 */
void ForcefullyQuitApp(AppData* app) { app->running = false; }

/**
 * Start a new pomodoro cycle from the current scene.
 * @param app Pointer to the application data
 */
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

/**
 * Open the reset pomodoro menu dialog.
 * @param app Pointer to the application data
 */
void OpenResetMenu(AppData* app) {
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
  RenderResetMenu(app);
}

/**
 * Reset the current pomodoro step (time only, not cycle).
 * @param app Pointer to the application data
 */
void ResetPomodoroStep(AppData* app) {
  app->pomodoro_data.current_step_time = 0;
}

/**
 * Reset the entire pomodoro cycle (all steps and progress).
 * @param app Pointer to the application data
 */
void ResetPomodoroCycle(AppData* app) { StartPomodoro(app); }

/**
 * Skip the current pomodoro step (with confirmation).
 * @param app Pointer to the application data
 */
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

/**
 * Skip the current pomodoro step without confirmation.
 * @param app Pointer to the application data
 */
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

/**
 * ---------------------------------------------------------------------------
 * Navigation Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Select the next item in the current menu.
 * @param app Pointer to the application data
 */
void SelectNextItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], 1);
}

/**
 * Select the previous item in the current menu.
 * @param app Pointer to the application data
 */
void SelectPreviousItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], -1);
}

/**
 * Execute the action of the currently selected menu item.
 * @param app Pointer to the application data
 */
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

/**
 * Close the currently open popup dialog.
 * @param app Pointer to the application data
 */
void ClosePopup(AppData* app) {
  if (app->popup_dialog != NULL) FreeFloatingDialog(app->popup_dialog);

  app->popup_dialog = NULL;
}

/**
 * Navigate popup left/up (previous item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemLeft(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    flushinp();
  } else
    SelectPreviousItem(app);
}

/**
 * Navigate popup right/down (next item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemRight(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    flushinp();
  } else
    SelectNextItem(app);
}

/**
 * ---------------------------------------------------------------------------
 * Notes Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move selected note up in the list.
 * @param app Pointer to the application data
 */
void NoteUpApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  NoteUp(app->notes);
}

/**
 * Move selected note down in the list.
 * @param app Pointer to the application data
 */
void NoteDownApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  NoteDown(app->notes);
}

/**
 * Toggle the selected note between done and undone.
 * @param app Pointer to the application data
 */
void ToggleTaskAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  ToggleTask(app->notes);
}

/**
 * Delete the currently selected note.
 * @param app Pointer to the application data
 */
void DeleteNoteAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  DeleteNote(app->notes);
}

/**
 * Add a new task with [ ] prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewTask(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  int insert_after = app->notes->current_id;
  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = true;
    input->pending_parent_id = -1;
    input->insert_after_id = insert_after;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a new note with - prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewNote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  int insert_after = app->notes->current_id;
  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = false;
    input->pending_parent_id = -1;
    input->insert_after_id = insert_after;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a subtask under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubtask(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->current_id < 0) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  NoteItem* parent = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      parent = app->notes->items[i];
      break;
    }
  }
  if (!parent) return;
  if (parent->depth >= MAX_NOTE_DEPTH) return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = true;
    input->pending_parent_id = parent->id;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a subnote under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubnote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->current_id < 0) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  NoteItem* parent = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      parent = app->notes->items[i];
      break;
    }
  }
  if (!parent) return;
  if (parent->depth >= MAX_NOTE_DEPTH) return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = false;
    input->pending_parent_id = parent->id;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Edit the selected note content (NORMAL mode).
 * Switches to INSERT mode with existing content loaded.
 * @param app Pointer to the application data
 */
void EditCurrentNote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes == NULL || app->notes->current_id < 0) return;

  NoteItem* note = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      note = app->notes->items[i];
      break;
    }
  }
  if (!note) return;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  char* text = GapBufferToString(note->text);
  if (!text) return;

  input->len = 0;
  input->cursor = 0;
  input->buffer[0] = '\0';
  strncpy(input->buffer, text, 255);
  input->buffer[255] = '\0';
  input->len = strlen(input->buffer);
  input->cursor = input->len > 0 ? input->len - 1 : 0;
  input->is_task = (note->state != NOTE_PLAIN);

  free(text);
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
}

/**
 * Undo last note operation.
 * @param app Pointer to the application data
 */
void UndoNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (!HistoryCanUndo(app->notes->history)) return;

  /* Don't allow undo in Move Mode */
  if (app->notes->is_move_mode) return;

  /* Check if in NORMAL mode - only allow undo if action was on current note */
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == NORMAL) {
    void* snapshot = HistoryPop(app->notes->history, true);
    NotesData* snap = (NotesData*)snapshot;
    if (snap && snap->last_affected_id != app->notes->current_id) {
      /* Not allowed - push back and return */
      HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, true);
      return;
    }
    /* Allowed - push it back for the normal undo flow */
    HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, true);
  }

  NotesData* current = CloneNotesData(app->notes);
  void* prev = HistoryPop(app->notes->history, true);
  HistoryPush(app->notes->history, current, FreeClonedNotesData, true);
  RestoreNotesData(app->notes, prev);

  /* Force stay in DEFAULT mode, not Move Mode */
  app->notes->is_move_mode = false;

  /* Restore input buffer and cursor to match restored note */
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && app->notes->current_id >= 0) {
    NotesData* snap = (NotesData*)prev;
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    if (note) {
      char* text = GapBufferToString(note->text);
      if (text) {
        int len = strlen(text);
        if (len >= input->max_len) len = input->max_len - 1;
        memcpy(input->buffer, text, len);
        input->buffer[len] = '\0';
        input->len = len;
        /* Restore cursor - clamp to valid range */
        if (snap && snap->saved_cursor >= 0) {
          input->cursor = snap->saved_cursor;
        } else {
          input->cursor = len;
        }
        /* Clamp to valid range for current mode */
        int max_cursor =
          (app->screen->panels[app->screen->current_panel].mode == INSERT)
            ? len
            : (len > 0 ? len - 1 : 0);
        if (input->cursor > max_cursor) input->cursor = max_cursor;
        if (input->cursor < 0) input->cursor = 0;
        free(text);
      }
    }
  }

  FreeClonedNotesData(prev);
}

/**
 * Redo last undone note operation.
 * @param app Pointer to the application data
 */
void RedoNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (!HistoryCanRedo(app->notes->history)) return;

  /* Don't allow redo in Move Mode */
  if (app->notes->is_move_mode) return;

  /* Check if in NORMAL mode - only allow redo if action was on current note */
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == NORMAL) {
    void* snapshot = HistoryPop(app->notes->history, false);
    NotesData* snap = (NotesData*)snapshot;
    if (snap && snap->last_affected_id != app->notes->current_id) {
      HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, false);
      return;
    }
    HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, false);
  }

  NotesData* current = CloneNotesData(app->notes);
  void* next = HistoryPop(app->notes->history, false);
  HistoryPush(app->notes->history, current, FreeClonedNotesData, false);
  RestoreNotesData(app->notes, next);

  /* Force stay in DEFAULT mode, not Move Mode */
  app->notes->is_move_mode = false;

  /* Restore input buffer and cursor to match restored note */
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && app->notes->current_id >= 0) {
    NotesData* snap = (NotesData*)next;
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    if (note) {
      char* text = GapBufferToString(note->text);
      if (text) {
        int len = strlen(text);
        if (len >= input->max_len) len = input->max_len - 1;
        memcpy(input->buffer, text, len);
        input->buffer[len] = '\0';
        input->len = len;
        /* Restore cursor - clamp to valid range */
        if (snap && snap->saved_cursor >= 0) {
          input->cursor = snap->saved_cursor;
        } else {
          input->cursor = len;
        }
        /* Clamp to valid range for current mode */
        int max_cursor =
          (app->screen->panels[app->screen->current_panel].mode == INSERT)
            ? len
            : (len > 0 ? len - 1 : 0);
        if (input->cursor > max_cursor) input->cursor = max_cursor;
        if (input->cursor < 0) input->cursor = 0;
        free(text);
      }
    }
  }

  FreeClonedNotesData(next);
}

/**
 * ---------------------------------------------------------------------------
 * Move Mode Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Toggle move mode for reorganizing notes.
 * @param app Pointer to the application data
 */
void ToggleMoveMode(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  app->notes->is_move_mode = !app->notes->is_move_mode;
}

/**
 * Exit move mode and return to normal interaction.
 * @param app Pointer to the application data
 */
void ExitMoveMode(AppData* app) {
  if (!app || !app->notes) return;
  if (app->notes->is_move_mode) {
    app->notes->is_move_mode = false;
  } else {
    if (app->user_input == app->last_input) app->running = false;
  }
  app->user_input = -1;
  app->last_input = -1;
}

/**
 * Move the selected note up one position.
 * @param app Pointer to the application data
 */
void MoveNoteUpWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode)
    MoveNoteUp(app->notes);
  else
    NoteUp(app->notes);
}

/**
 * Move the selected note down one position.
 * @param app Pointer to the application data
 */
void MoveNoteDownWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode)
    MoveNoteDown(app->notes);
  else
    NoteDown(app->notes);
}

/**
 * Promote note (move to parent level, decrease depth).
 * @param app Pointer to the application data
 */
void PromoteNoteWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) PromoteNote(app->notes);
}

/**
 * Demote note (move to child of previous sibling, increase depth).
 * @param app Pointer to the application data
 */
void DemoteNoteWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) DemoteNote(app->notes);
}

/**
 * Quit notes scene and return to previous scene.
 * @param app Pointer to the application data
 */
void QuitAppNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (app->notes->is_move_mode) {
    app->notes->is_move_mode = false;
    app->user_input = -1;
    app->last_input = -1;
    return;
  }
  if (app->user_input == app->last_input) app->running = false;
}
