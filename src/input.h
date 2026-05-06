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

/* Mode-specific handlers */
ErrorType HandleInsertMode(AppData* app, int key);
ErrorType HandleVisualMode(AppData* app, int key);
ErrorType HandleNormalMode(AppData* app, int key);

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
extern int input_pos;
extern int input_mode_type; /* 0 = task, 1 = note */

/* Scene types bitmask for pomodoro-related scenes */
#define POMODORO_SCENES \
  (SCENE_WORK_TIME | SCENE_SHORT_PAUSE | SCENE_LONG_PAUSE | SCENE_MAIN_MENU)
#define NOTES_SCENES (SCENE_NOTES | SCENE_HELP | SCENE_CONTINUE)
#define ALL_SCENES (POMODORO_SCENES | NOTES_SCENES)

/* Struct to map a key to a function */
static const KeyFunction keys[] = {
  /* NOTES panel vim-like keybindings (checked first for NOTES scene) */
  {'j', NoteDownApp, NORMAL, SCENE_NOTES},
  {'k', NoteUpApp, NORMAL, SCENE_NOTES},
  {KEY_DOWN, NoteDownApp, NORMAL, SCENE_NOTES},
  {KEY_UP, NoteUpApp, NORMAL, SCENE_NOTES},
  {ENTER, ToggleTaskAtNotes, NORMAL, SCENE_NOTES},
  {'d', DeleteNoteAtNotes, NORMAL, SCENE_NOTES},
  {'a', AddNewNote, NORMAL, SCENE_NOTES},     /* Add task */
  {'A', AddNewNoteItem, NORMAL, SCENE_NOTES}, /* Add note */

  /* General keybindings */
  {' ', NextPanel, NORMAL, ALL_SCENES},
  {KEY_DOWN, SelectNextItem, NORMAL, POMODORO_SCENES},
  {KEY_UP, SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {KEY_RIGHT, SelectNextItem, NORMAL, POMODORO_SCENES},
  {KEY_LEFT, SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'j', SelectNextItem, NORMAL, POMODORO_SCENES},
  {'k', SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'l', SelectNextItem, NORMAL, POMODORO_SCENES},
  {'h', SelectPreviousItem, NORMAL, POMODORO_SCENES},
  {'s', SkipPomodoroStep, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {'p', TogglePause, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {CTRLR, OpenResetMenu, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {'q', QuitApp, NORMAL, ALL_SCENES},
  {ESC, QuitApp, NORMAL, ALL_SCENES},
  // {'m', ChangeMode, NORMAL | INSERT | VISUAL, ALL_SCENES},
  {ENTER, ExecuteMenuAction, NORMAL, POMODORO_SCENES},
};

#endif /* INPUT_H_ */
