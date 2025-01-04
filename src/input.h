#ifndef INPUT_H_
#define INPUT_H_

#include "error.h"
#include "tomato.h"
#include "ui.h"

/* Struct to map a key to a function */
typedef struct {
  int key;
  void (*action)(AppData* app);
  int modes;
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

/* Function to execute the action of the selected menu item */
void ExecuteMenuAction(AppData* app);

/* Struct to map a key to a function */
static const KeyFunction keys[] = {
  {' ', NextPanel, NORMAL},
  {'j', SelectNextItem, NORMAL},
  {'k', SelectPreviousItem, NORMAL},
  {'l', SelectNextItem, NORMAL},
  {'h', SelectPreviousItem, NORMAL},
  {'p', TogglePause, NORMAL},
  {'q', QuitApp, NORMAL},
  {ESC, QuitApp, NORMAL | VISUAL},
  {'m', ChangeMode, NORMAL | INSERT | VISUAL},
  {ENTER, ExecuteMenuAction, NORMAL},
};

#endif /* INPUT_H_ */
