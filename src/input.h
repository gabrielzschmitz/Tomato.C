#ifndef INPUT_H_
#define INPUT_H_

#include <ncurses.h>
#include <stdbool.h>

#include "error.h"

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

typedef struct AppData AppData;
typedef struct Panel Panel;
typedef struct InputState InputState;
typedef struct KeyFunction KeyFunction;

/**
 * Input mode enum for the text editor (vim-like modes).
 * Determines how keyboard input is interpreted.
 */
typedef enum {
  DEFAULT = 1 << 0, /* Default mode for menu navigation */
  NORMAL = 1 << 1,  /* Normal mode for text commands and navigation */
  INSERT = 1 << 2,  /* Insert mode for text input */
  VISUAL = 1 << 3,  /* Visual mode for text selection */
} InputMode;

/**
 * Scene type enum representing the current application view.
 * Used for routing input and determining which UI to display.
 */
typedef enum {
  MAIN_MENU,   /* Main menu scene */
  WORK_TIME,   /* Work session timer scene */
  SHORT_PAUSE, /* Short break timer scene */
  LONG_PAUSE,  /* Long break timer scene */
  NOTES,       /* Notes/text editor scene */
  HELP,        /* Help screen scene */
  CONTINUE,    /* Continue/pause scene */
} SceneType;

/* Scene type bitmasks for key binding filters */
#define SCENE_MAIN_MENU (1 << MAIN_MENU)
#define SCENE_WORK_TIME (1 << WORK_TIME)
#define SCENE_SHORT_PAUSE (1 << SHORT_PAUSE)
#define SCENE_LONG_PAUSE (1 << LONG_PAUSE)
#define SCENE_NOTES (1 << NOTES)
#define SCENE_HELP (1 << HELP)
#define SCENE_CONTINUE (1 << CONTINUE)

/**
 * Input state for text input in vim-like modes.
 * Manages text buffer, cursor position, and selection for the text editor.
 */
struct InputState {
  char buffer[256]; /* Character buffer for input text */
  int len;          /* Current length of text in buffer */
  int cursor;       /* Current cursor position (0 to len) */
  int max_len;      /* Maximum buffer length (typically 255) */
  bool is_task;     /* true for task [ ], false for note - */
  int
    pending_parent_id; /* ID of parent note for pending insertion (-1 for root) */
  int insert_after_id; /* Note ID to insert after (-1 for none) */
  struct {
    int start; /* Start position of visual selection */
    int end;   /* End position of visual selection */
  } selection; /* Visual mode selection range */
};

/**
 * Struct to map a key to a function with mode and scene filters.
 * Used for defining keyboard shortcuts in the application.
 */
struct KeyFunction {
  int key;                      /* The key code */
  void (*action)(AppData* app); /* Function to execute when key's pressed */
  int modes;       /* Bitmask of input modes where this key is active */
  int scene_types; /* Bitmask of scene types where this key is active */
};

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
void SwitchToInsertMode(AppData* app);       /* 'i' key */
void SwitchToInsertModeAppend(AppData* app); /* 'a' key */
void SwitchToVisualMode(AppData* app);       /* 'v' key */
void SwitchToNormalMode(AppData* app);       /* ESC key */

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
void AddNewTask(AppData* app);      /* Add task with [ ] prefix */
void AddNewNote(AppData* app);      /* Add note with - prefix */
void AddSubtask(AppData* app);      /* Add subtask under selected node */
void AddSubnote(AppData* app);      /* Add subnote under selected node */
void EditCurrentNote(AppData* app); /* Edit selected node, NORMAL mode */

/* Move mode functions */
void ToggleMoveMode(AppData* app);
void ExitMoveMode(AppData* app);
void MoveNoteUpWrapper(AppData* app);
void MoveNoteDownWrapper(AppData* app);
void PromoteNoteWrapper(AppData* app);
void DemoteNoteWrapper(AppData* app);
void QuitAppNotes(AppData* app);

/* Popup navigation wrappers */
void ChangeSelectedItemLeft(AppData* app);
void ChangeSelectedItemRight(AppData* app);

#endif /* INPUT_H_ */
