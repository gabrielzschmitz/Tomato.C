#ifndef INPUT_H_
#define INPUT_H_

#include <ncurses.h>
#include <stdbool.h>

#include "error.h"
#include "tomato.h"
#include "ui.h"

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

/* Input state for text input (vim-like modes) */
typedef struct InputState {
  char buffer[256];
  int len;
  int cursor;
  int max_len;
  bool is_task; /* true for task [ ], false for note - */
  struct {
    int start;
    int end;
  } selection;
} InputState;

/* InputState management */
InputState* InputStateCreate(void);
void InputStateDestroy(InputState** input);
void InputStateClear(InputState* s);

/* Centralized mode transition */
void InputSetMode(Panel* panel, InputMode mode);

/* Function to process key input */
void ProcessKeyInput(AppData* app, int key);

/* Handle user input and app state */
ErrorType HandleInputs(AppData* app);

ErrorType HandleDefaultMode(AppData* app, int key);
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

/* Vim-like mode switching */
void SwitchToInsertMode(AppData* app); /* 'i' key */
void SwitchToInsertModeAppend(AppData* app); /* 'a' key */
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
void AddNewTask(AppData* app); /* Add task with [ ] prefix */
void AddNewNote(AppData* app); /* Add note with - prefix */
void EditCurrentNote(AppData* app); /* Edit selected node, NORMAL mode */

/* Popup navigation wrappers */
void ChangeSelectedItemLeft(AppData* app);
void ChangeSelectedItemRight(AppData* app);

#endif /* INPUT_H_ */
