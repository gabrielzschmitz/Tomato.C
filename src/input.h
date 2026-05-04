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

/* Scene types bitmask for pomodoro-related scenes */
#define POMODORO_SCENES \
  (SCENE_WORK_TIME | SCENE_SHORT_PAUSE | SCENE_LONG_PAUSE | SCENE_MAIN_MENU)
#define ALL_SCENES (POMODORO_SCENES | SCENE_NOTES | SCENE_HELP | SCENE_CONTINUE)

/* Struct to map a key to a function */
static const KeyFunction keys[] = {
  {' ', NextPanel, NORMAL, ALL_SCENES},
  {KEY_DOWN, SelectNextItem, NORMAL, ALL_SCENES},
  {KEY_UP, SelectPreviousItem, NORMAL, ALL_SCENES},
  {'j', SelectNextItem, NORMAL, ALL_SCENES},
  {'k', SelectPreviousItem, NORMAL, ALL_SCENES},
  {'l', SelectNextItem, NORMAL, ALL_SCENES},
  {'h', SelectPreviousItem, NORMAL, ALL_SCENES},
  {'s', SkipPomodoroStep, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {'p', TogglePause, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {CTRLR, OpenResetMenu, NORMAL | INSERT | VISUAL, POMODORO_SCENES},
  {'q', QuitApp, NORMAL, ALL_SCENES},
  {ESC, QuitApp, NORMAL | VISUAL, ALL_SCENES},
  {'m', ChangeMode, NORMAL | INSERT | VISUAL, ALL_SCENES},
  {ENTER, ExecuteMenuAction, NORMAL, ALL_SCENES},
};

#endif /* INPUT_H_ */
