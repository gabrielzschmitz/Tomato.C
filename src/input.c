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

/* Function to process key input */
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

/* Handle popup input - only when popup is active */
int HandlePopupInput(AppData* app, int key) {
  if (app->popup_dialog == NULL) return 0;

  /* When popup is active, only use keys bound to ALL_SCENES to avoid
   * scene-specific keys (like ToggleTaskAtNotes) intercepting popup input */
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
        keys[i].scene_types == ALL_SCENES) {
      keys[i].action(app);
      return 1; /* handled */
    }
  }
  /* For other keys (like q, ESC), let them pass through */
  return 0;
}

/* Handle DEFAULT mode - navigation, no text input */
ErrorType HandleDefaultMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* First handle popup input if popup is active */
  if (HandlePopupInput(app, key)) {
    return status; /* popup key was handled */
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

/* Handle NORMAL mode - text input editing */
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

/* InputState management */
InputState* InputStateCreate(void) {
  InputState* s = (InputState*)malloc(sizeof(InputState));
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->max_len = sizeof(s->buffer) - 1;
    s->is_task = true;
    s->selection.start = 0;
    s->selection.end = 0;
  }
  return s;
}

void InputStateDestroy(InputState** input) {
  if (input && *input) {
    free(*input);
    *input = NULL;
  }
}

void InputStateClear(InputState* s) {
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->selection.start = 0;
    s->selection.end = 0;
  }
}

/* Centralized mode transition */
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

/* Action functions for keybindings */
void InputCursorLeft(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor > 0) input->cursor--;
}

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

void InputDeleteChar(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
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

void InputVisualDelete(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
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
    if (input->cursor > 0 && input->cursor > input->len - 1) {
      input->cursor = (input->len > 0) ? input->len - 1 : 0;
    }
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    refresh();
  }
}

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
  if (app->notes->current_id >= 0)
    UpdateNote(app->notes, app->notes->current_id, input->buffer, state);
  else
    AddNote(app->notes, input->buffer, state);
  input->len = 0;
  input->cursor = 0;
  input->buffer[0] = '\0';
  input->is_task = true;
  app->screen->panels[app->screen->current_panel].mode = DEFAULT;
  app->user_input = -1;
  app->last_input = -1;
  move(LINES - 1, 0);
  clrtoeol();
  refresh();
}

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
      }
      if (app->notes->count > 0)
        app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
      app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    } else
      app->screen->panels[app->screen->current_panel].mode = NORMAL;
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

/* Switch to INSERT mode (vim 'i' key) */
void SwitchToInsertMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/* Switch to INSERT mode at cursor+1 (vim 'a' key) */
void SwitchToInsertModeAppend(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor < input->len) input->cursor++;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/* Switch to VISUAL mode (vim 'v' key) */
void SwitchToVisualMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  app->screen->panels[app->screen->current_panel].mode = VISUAL;
  if (input) input->selection.start = input->cursor;
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

void AddNewTask(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = true;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

void AddNewNote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = false;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

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

void InputSwitchToInsertFromVisual(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  noecho();
  curs_set(1);
  refresh();
}

/* Wrapper for popup navigation - change selected item left */
void ChangeSelectedItemLeft(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    flushinp();
  } else
    SelectPreviousItem(app);
}

/* Wrapper for popup navigation - change selected item right */
void ChangeSelectedItemRight(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    flushinp();
  } else
    SelectNextItem(app);
}
