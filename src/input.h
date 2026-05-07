#ifndef INPUT_H_
#define INPUT_H_

#include <ncurses.h>

#include "error.h"
#include "tomato.h"
#include "ui.h"

/* Struct to map a key to a function */
typedef struct {
  int key;
  void (*action)(AppData* app);
  int modes;
  int scene_types;
} KeyFunction;

/* Defining some ASCII Keys */
#define BACKSPACE 127
#define ESC 27
#define ENTER 10
#define CTRLC 3
#define CTRLP 16
#define CTRLS 19
#define CTRLX 24
#define CTRLD 4
#define CTRLR 18
#define CTRLF 6
#define CTRLW 23
#define CTRLT 20

/* Function to process key input */
void ProcessKeyInput(AppData* app, int key);

/* Handle user input and app state */
ErrorType HandleInputs(AppData* app);

ErrorType HandleNormalMode(AppData* app, int key);
ErrorType HandleInsertMode(AppData* app, int key);
ErrorType HandleVisualMode(AppData* app, int key);

/* New action functions for keybindings */
void InputCursorLeft(AppData* app);
void InputCursorRight(AppData* app);
void InputBackspace(AppData* app);
void InputDeleteChar(AppData* app);
void InputVisualDelete(AppData* app);
void InputCommit(AppData* app);
void InputESC(AppData* app);
void InputInsertChar(AppData* app);
void InputSwitchToInsertFromVisual(AppData* app);

/* Check if the given key is assigned to the specified action function */
int IsKeyAssignedToAction(int key, void (*action)(AppData*));

/* Switch to next panel */
void NextPanel(AppData* app);

/* Select next menu item */
void SelectNextItem(AppData* app);

/* Select previous menu item */
void SelectPreviousItem(AppData* app);

/* Toggle pause */
void TogglePause(AppData* app);

/* Change the input mode */
void ChangeMode(AppData* app);

/* Vim-like mode switching */
void SwitchToInsertMode(AppData* app); /* 'i' key */
void SwitchToVisualMode(AppData* app); /* 'v' key */
void SwitchToNormalMode(AppData* app); /* ESC key */

/* Update animation mode */
void ChangeDebugAnimation(AppData* app, int step);

/* Quit the program */
void QuitApp(AppData* app);

/* Quit the program forcefully */
void ForcefullyQuitApp(AppData* app);

/* Close the popup dialog */
void ClosePopup(AppData* app);

/* Start pomodoro cycle */
void StartPomodoro(AppData* app);

/* Open the reset pomodoro menu */
void OpenResetMenu(AppData* app);

/* Reset pomodoro step */
void ResetPomodoroStep(AppData* app);

/* Reset pomodoro cycle */
void ResetPomodoroCycle(AppData* app);

/* Skip pomodoro step */
void SkipPomodoroStep(AppData* app);

/* Forcefully skip pomodoro step */
void ForcefullySkipPomodoroStep(AppData* app);

/* Function to execute the action of the selected menu item */
void ExecuteMenuAction(AppData* app);

/* Notes keybinding functions */
void NoteDownApp(AppData* app);
void NoteUpApp(AppData* app);
void ToggleTaskAtNotes(AppData* app);
void DeleteNoteAtNotes(AppData* app);
void AddNewNote(AppData* app);     /* Add task with [ ] prefix */
void AddNewNoteItem(AppData* app); /* Add note with - prefix */

/* Input buffer for INSERT mode (accessible from other files) */
extern char input_buffer[];
extern int input_len; /* Actual length of input */
extern int
  input_cursor_pos;      /* Cursor position in input buffer (0 to input_len) */
extern int visual_start; /* Start position for VISUAL mode selection */
extern int input_mode_type; /* 0 = task, 1 = note */

/* Scene types bitmask for pomodoro-related scenes */
#define POMODORO_SCENES \
  (SCENE_WORK_TIME | SCENE_SHORT_PAUSE | SCENE_LONG_PAUSE | SCENE_MAIN_MENU)
#define ALL_SCENES (POMODORO_SCENES | SCENE_NOTES | SCENE_HELP | SCENE_CONTINUE)

/* Struct to map a key to a function */
static const KeyFunction keys[] = {
  /* NORMAL mode - editing keys (when input_len > 0) */
  {'h', InputCursorLeft, NORMAL, SCENE_NOTES},
  {'l', InputCursorRight, NORMAL, SCENE_NOTES},
  {'x', InputDeleteChar, NORMAL, SCENE_NOTES},
  {'i', SwitchToInsertMode, NORMAL, SCENE_NOTES},
  {'v', SwitchToVisualMode, NORMAL, SCENE_NOTES},
  {ESC, InputESC, NORMAL, SCENE_NOTES},

  /* NORMAL mode - navigation keys (when input_len == 0) */
  {'j', NoteDownApp, NORMAL, SCENE_NOTES},
  {'k', NoteUpApp, NORMAL, SCENE_NOTES},
  {KEY_DOWN, NoteDownApp, NORMAL, SCENE_NOTES},
  {KEY_UP, NoteUpApp, NORMAL, SCENE_NOTES},
  {ENTER, ToggleTaskAtNotes, NORMAL, SCENE_NOTES},
  {'d', DeleteNoteAtNotes, NORMAL, SCENE_NOTES},
  {'a', AddNewNote, NORMAL, SCENE_NOTES},
  {'A', AddNewNoteItem, NORMAL, SCENE_NOTES},

  /* INSERT mode keys */
  {KEY_LEFT, InputCursorLeft, INSERT, SCENE_NOTES},
  {KEY_RIGHT, InputCursorRight, INSERT, SCENE_NOTES},
  {KEY_BACKSPACE, InputBackspace, INSERT, SCENE_NOTES},
  {BACKSPACE, InputBackspace, INSERT, SCENE_NOTES},
  {ENTER, InputCommit, INSERT, SCENE_NOTES},
  {'\r', InputCommit, INSERT, SCENE_NOTES},
  {KEY_ENTER, InputCommit, INSERT, SCENE_NOTES},
  {ESC, InputESC, INSERT, SCENE_NOTES},
  {-1, InputInsertChar, INSERT, SCENE_NOTES}, /* printable chars */

  /* VISUAL mode keys */
  {'h', InputCursorLeft, VISUAL, SCENE_NOTES},
  {'l', InputCursorRight, VISUAL, SCENE_NOTES},
  {'x', InputVisualDelete, VISUAL, SCENE_NOTES},
  {'a', InputSwitchToInsertFromVisual, VISUAL, SCENE_NOTES},
  {ENTER, InputCommit, VISUAL, SCENE_NOTES},
  {'\r', InputCommit, VISUAL, SCENE_NOTES},
  {KEY_ENTER, InputCommit, VISUAL, SCENE_NOTES},
  {ESC, InputESC, VISUAL, SCENE_NOTES},

  /* Mode switching */
  {'i', SwitchToInsertMode, VISUAL, SCENE_NOTES},
  {'v', SwitchToVisualMode, INSERT, SCENE_NOTES},
  {ESC, InputESC, INSERT | VISUAL, SCENE_NOTES},

  /* General keybindings - only for NORMAL mode */
  {' ', NextPanel, NORMAL, ALL_SCENES},
  {KEY_DOWN, SelectNextItem, NORMAL, POMODORO_SCENES},
  {KEY_UP, SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {KEY_RIGHT, SelectNextItem, NORMAL, POMODORO_SCENES},
  {KEY_LEFT, SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'j', SelectNextItem, NORMAL, POMODORO_SCENES},
  {'k', SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'l', SelectNextItem, NORMAL, POMODORO_SCENES},
  {'h', SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'s', SkipPomodoroStep, NORMAL, POMODORO_SCENES},
  {'p', TogglePause, NORMAL, POMODORO_SCENES},
  {CTRLR, OpenResetMenu, NORMAL, POMODORO_SCENES},
  {'q', QuitApp, NORMAL, ALL_SCENES},
  {ESC, QuitApp, NORMAL, ALL_SCENES},
  {ENTER, ExecuteMenuAction, NORMAL, POMODORO_SCENES},
};

#endif /* INPUT_H_ */
