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

/* Buffer for INSERT mode text input */
char input_buffer[256];
int input_len = 0;        /* Actual length of input */
int input_cursor_pos = 0; /* Cursor position in input buffer (0 to input_len) */
int visual_start = 0;     /* Start position for VISUAL mode selection */
int input_mode_type = 0;  /* 0 = task, 1 = note */

/* Function to process key input */
void ProcessKeyInput(AppData* app, int key) {
  size_t numKeyFunctions = sizeof(keys) / sizeof(keys[0]);
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key &&
        (keys[i].modes &
         app->screen->panels[app->screen->current_panel].mode) &&
        (keys[i].scene_types & (1 << current_scene))) {
      keys[i].action(app);
      break;
    }
  }
}

/* Handle INSERT mode - capture all input for text entry */
ErrorType HandleInsertMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  if (key == KEY_ENTER || key == '\n' || key == '\r') {
    /* Finish input - switch back to NORMAL and add the note/task */
    input_buffer[input_len] = '\0';
    if (input_len > 0) {
      if (input_mode_type == 0)
        AddNote(app->notes, input_buffer, true);
      else
        AddNote(app->notes, input_buffer, false);
      /* Select the newly added note */
      app->notes->current = app->notes->tail;
    } else
      app->notes->current = app->notes->tail;
    input_len = 0;
    input_cursor_pos = 0;
    input_buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    /* Clear input state to prevent triggering quit popup */
    app->user_input = -1;
    app->last_input = -1;
    /* Clear the input line */
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  } else if (key == ESC) { /* ESC to go to NORMAL mode - keep input */
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    /* Clear input state to prevent quit popup */
    app->user_input = -1;
    app->last_input = -1;
    noecho();
    curs_set(0);
    refresh();
  } else if (key == KEY_LEFT && input_cursor_pos > 0) {
    /* Move cursor left */
    input_cursor_pos--;
  } else if (key == KEY_RIGHT && input_cursor_pos < input_len) {
    /* Move cursor right */
    input_cursor_pos++;
  } else if (key == KEY_BACKSPACE || key == BACKSPACE) {
    if (input_cursor_pos > 0) {
      /* Delete character before cursor and shift left */
      for (int i = input_cursor_pos - 1; i < input_len - 1; i++)
        input_buffer[i] = input_buffer[i + 1];
      input_cursor_pos--;
      input_len--;
      input_buffer[input_len] = '\0';
    }
  } else if (key >= ' ' && key <= '~' &&
             input_len <
               (int)sizeof(input_buffer) - 1) { /* Printable characters */
    /* Insert character at cursor position - shift right */
    for (int i = input_len; i > input_cursor_pos; i--)
      input_buffer[i] = input_buffer[i - 1];
    input_buffer[input_cursor_pos] = key;
    input_cursor_pos++;
    input_len++;
    input_buffer[input_len] = '\0';
  }

  return status;
}

ErrorType HandleVisualMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* Handle ENTER to commit note in VISUAL mode */
  if (key == KEY_ENTER || key == '\n' || key == '\r') {
    input_buffer[input_len] = '\0';
    if (input_len > 0) {
      if (input_mode_type == 0)
        AddNote(app->notes, input_buffer, true);
      else
        AddNote(app->notes, input_buffer, false);
      /* Select the newly added note */
      app->notes->current = app->notes->tail;
    } else {
      /* Cancel commit - select last note on the list */
      app->notes->current = app->notes->tail;
    }
    input_len = 0;
    input_cursor_pos = 0;
    visual_start = 0;
    input_buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
    return status;
  }

  /* Handle cursor movement and delete in VISUAL mode */
  if (input_len > 0) {
    if (key == 'h' && input_cursor_pos > 0) {
      input_cursor_pos--;
      return status;
    } else if (key == 'l' && input_cursor_pos < input_len) {
      input_cursor_pos++;
      return status;
    } else if (key == 'x') {
      /* Delete all selected characters (from visual_start to cursor) */
      int start_sel =
        (visual_start < input_cursor_pos) ? visual_start : input_cursor_pos;
      int end_sel =
        (visual_start < input_cursor_pos) ? input_cursor_pos : visual_start;
      int sel_len = end_sel - start_sel;
      if (sel_len > 0 && start_sel < input_len) {
        /* Shift characters after selection left */
        for (int i = start_sel; i < input_len - sel_len; i++) {
          input_buffer[i] = input_buffer[i + sel_len];
        }
        input_len -= sel_len;
        input_buffer[input_len] = '\0';
        input_cursor_pos = start_sel;
        visual_start = start_sel;
      }
      app->screen->panels[app->screen->current_panel].mode = NORMAL;
      noecho();
      curs_set(0);
      refresh();
      return status;
    } else if (key == 'i' || key == 'a') {
      /* Go back to INSERT mode */
      app->screen->panels[app->screen->current_panel].mode = INSERT;
      noecho();
      curs_set(1);
      refresh();
      return status;
    } else if (key == ESC) { /* ESC - go back to NORMAL */
      app->screen->panels[app->screen->current_panel].mode = NORMAL;
      noecho();
      curs_set(0);
      refresh();
      return status;
    }
    /* Block all other keys */
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

/* Handle NORMAL mode - process key bindings */
ErrorType HandleNormalMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* Handle input buffer navigation if there's content */
  if (input_len > 0) {
    /* Only allow specific keys while editing - block note list interaction */
    if (key == 'h' && input_cursor_pos > 0) {
      input_cursor_pos--;
      return status;
    } else if (key == 'l' && input_cursor_pos < input_len) {
      input_cursor_pos++;
      return status;
    } else if (key == 'x') {
      /* Delete character at cursor */
      if (input_cursor_pos < input_len) {
        for (int i = input_cursor_pos; i < input_len - 1; i++)
          input_buffer[i] = input_buffer[i + 1];
        input_len--;
        input_buffer[input_len] = '\0';
        /* If cursor is past end, move back one */
        if (input_cursor_pos > input_len) input_cursor_pos = input_len;
      }
      return status;
    } else if (key == 'i' || key == 'a') {
      /* Go back to INSERT mode */
      app->screen->panels[app->screen->current_panel].mode = INSERT;
      noecho();
      curs_set(1);
      refresh();
      return status;
    } else if (key == 'v') {
      /* Go to VISUAL mode */
      app->screen->panels[app->screen->current_panel].mode = VISUAL;
      visual_start = input_cursor_pos;
      noecho();
      curs_set(0);
      refresh();
      return status;
    } else if (key == ESC) { /* ESC - go back to INSERT */
      app->screen->panels[app->screen->current_panel].mode = INSERT;
      noecho();
      curs_set(1);
      refresh();
      return status;
    } else if (key == KEY_ENTER || key == '\n' || key == '\r') {
      /* Commit note in NORMAL mode */
      input_buffer[input_len] = '\0';
      if (input_len > 0) {
        if (input_mode_type == 0)
          AddNote(app->notes, input_buffer, true);
        else
          AddNote(app->notes, input_buffer, false);
      } else
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
      return status;
    }
    /* Block all other keys while editing */
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
  if (current_mode == INSERT) {
    status = HandleInsertMode(app, key);
    flushinp();
    return status;
  } else if (current_mode == VISUAL)
    status = HandleVisualMode(app, key);
  else if (current_mode == NORMAL)
    status = HandleNormalMode(app, key);

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

/* Switch to INSERT mode (vim 'i' key) */
void SwitchToInsertMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/* Switch to VISUAL mode (vim 'v' key) */
void SwitchToVisualMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = VISUAL;
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

/* Notes keybinding functions */
void ToggleTaskAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) {
    ExecuteMenuAction(app);
    return;
  }
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
