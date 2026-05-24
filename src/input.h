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
  DEFAULT = 1 << 0, /**< Default mode for menu navigation */
  NORMAL = 1 << 1,  /**< Normal mode for text commands and navigation */
  INSERT = 1 << 2,  /**< Insert mode for text input */
  VISUAL = 1 << 3,  /**< Visual mode for text selection */
} InputMode;

/**
 * Scene type enum representing the current application view.
 * Used for routing input and determining which UI to display.
 */
typedef enum {
  MAIN_MENU,   /**< Main menu scene */
  WORK_TIME,   /**< Work session timer scene */
  SHORT_PAUSE, /**< Short break timer scene */
  LONG_PAUSE,  /**< Long break timer scene */
  NOTES,       /**< Notes/text editor scene */
  HELP,        /**< Help screen scene */
  CONTINUE,    /**< Continue/pause scene */
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
  char buffer[256]; /**< Character buffer for input text */
  int len;          /**< Current length of text in buffer */
  int cursor;       /**< Current cursor position (0 to len) */
  int max_len;      /**< Maximum buffer length (typically 255) */
  bool is_task;     /**< true for task [ ], false for note - */
  int
    pending_parent_id; /**< ID of parent note for pending insertion (-1 for root) */
  int insert_after_id; /**< Note ID to insert after (-1 for none) */
  struct {
    int start; /**< Start position of visual selection */
    int end;   /**< End position of visual selection */
  } selection; /**< Visual mode selection range */
};

/**
 * Struct to map a key to a function with mode and scene filters.
 * Used for defining keyboard shortcuts in the application.
 */
struct KeyFunction {
  int key;                      /**< The key code */
  void (*action)(AppData* app); /**< Function to execute when key's pressed */
  int modes;       /**< Bitmask of input modes where this key is active */
  int scene_types; /**< Bitmask of scene types where this key is active */
};

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
void ProcessKeyInput(AppData* app, int key);

/**
 * Check if the given key is assigned to the specified action function.
 * @param key The key code to check
 * @param action Function pointer to compare against
 * @return 1 if key is assigned to action, 0 otherwise
 */
int IsKeyAssignedToAction(int key, void (*action)(AppData*));

/**
 * Handle all user input based on current app state.
 * Reads input from terminal and routes to appropriate handler.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInputs(AppData* app);

/**
 * Handle mouse events. In DEFAULT mode: hover updates menu selection +
 * switches panel focus on REQUEST_MOUSE_POSITION, clicks execute actions or
 * switch panels. In non-DEFAULT mode: movement is ignored, first click goes
 * directly to DEFAULT mode.
 * @param app Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
void HandleMouseEvent(AppData* app, MEVENT* event);

/**
 * Handle input in DEFAULT mode (menu navigation, pomodoro control).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleDefaultMode(AppData* app, int key);

/**
 * Handle input in NORMAL mode (text navigation, note editing).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleNormalMode(AppData* app, int key);

/**
 * Handle input in INSERT mode (text input).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInsertMode(AppData* app, int key);

/**
 * Handle input in VISUAL mode (text selection).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleVisualMode(AppData* app, int key);

/**
 * Handle input in while POPUP is active.
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return true if input is consumed, or false if not popup active
 */
bool HandlePopupInput(AppData* app, int key);

/**
 * ---------------------------------------------------------------------------
 * InputState Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new InputState with default values.
 * @return Pointer to the created InputState, or NULL on allocation failure
 */
InputState* InputStateCreate(void);

/**
 * Destroy an InputState and free its memory.
 * @param input Pointer to the InputState pointer to free
 */
void InputStateDestroy(InputState** input);

/**
 * Clear the InputState contents, resetting cursor and buffer.
 * @param s Pointer to the InputState to clear
 */
void InputStateClear(InputState* s);

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
void InputSetMode(Panel* panel, InputMode mode);

/**
 * Switch to INSERT mode from current position.
 * @param app Pointer to the application data
 */
void SwitchToInsertMode(AppData* app); /* 'i' key */

/**
 * Switch to INSERT mode after current cursor position.
 * @param app Pointer to the application data
 */
void SwitchToInsertModeAppend(AppData* app); /* 'a' key */

/**
 * Enter VISUAL mode for text selection.
 * @param app Pointer to the application data
 */
void SwitchToVisualMode(AppData* app); /* 'v' key */

/**
 * Exit to NORMAL mode (ESC key handler).
 * @param app Pointer to the application data
 */
void SwitchToNormalMode(AppData* app); /* ESC key */

/**
 * ---------------------------------------------------------------------------
 * Editor Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move cursor one position to the left.
 * @param app Pointer to the application data
 */
void InputCursorLeft(AppData* app);

/**
 * Move cursor one position to the right.
 * @param app Pointer to the application data
 */
void InputCursorRight(AppData* app);

/**
 * Delete character before cursor (backspace).
 * @param app Pointer to the application data
 */
void InputBackspace(AppData* app);

/**
 * Delete character at cursor (in NORMAL mode).
 * @param app Pointer to the application data
 */
void InputDeleteChar(AppData* app);

/**
 * Delete character(s) in visual selection.
 * @param app Pointer to the application data
 */
void InputVisualDelete(AppData* app);

/**
 * Commit current input (return/enter key).
 * Finalizes text entry or confirms selection.
 * @param app Pointer to the application data
 */
void InputCommit(AppData* app);

/**
 * Handle escape key - exit to DEFAULT mode or close dialog.
 * @param app Pointer to the application data
 */
void InputESC(AppData* app);

/**
 * Insert a printable character at cursor position.
 * @param app Pointer to the application data
 */
void InputInsertChar(AppData* app);

/**
 * Switch from VISUAL mode to INSERT mode, keeping selection.
 * @param app Pointer to the application data
 */
void InputSwitchToInsertFromVisual(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * App Control Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Switch focus to the next panel.
 * @param app Pointer to the application data
 */
void NextPanel(AppData* app);

/**
 * Toggle pomodoro timer pause state.
 * @param app Pointer to the application data
 */
void TogglePause(AppData* app);

/**
 * Update animation mode for debugging (step through frames).
 * @param app Pointer to the application data
 * @param step Number of frames to advance (can be negative)
 */
void ChangeDebugAnimation(AppData* app, int step);

/**
 * Quit the program with confirmation dialog.
 * @param app Pointer to the application data
 */
void QuitApp(AppData* app);

/**
 * Quit the program immediately without confirmation.
 * @param app Pointer to the application data
 */
void ForcefullyQuitApp(AppData* app);

/**
 * Start a new pomodoro cycle from the current scene.
 * @param app Pointer to the application data
 */
void StartPomodoro(AppData* app);

/**
 * Open the reset pomodoro menu dialog.
 * @param app Pointer to the application data
 */
void OpenResetMenu(AppData* app);

/**
 * Reset the current pomodoro step (time only, not cycle).
 * @param app Pointer to the application data
 */
void ResetPomodoroStep(AppData* app);

/**
 * Reset the entire pomodoro cycle (all steps and progress).
 * @param app Pointer to the application data
 */
void ResetPomodoroCycle(AppData* app);

/**
 * Skip the current pomodoro step (with confirmation).
 * @param app Pointer to the application data
 */
void SkipPomodoroStep(AppData* app);

/**
 * Skip the current pomodoro step without confirmation.
 * @param app Pointer to the application data
 */
void ForcefullySkipPomodoroStep(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * Navigation Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Select the next item in the current menu.
 * @param app Pointer to the application data
 */
void SelectNextItem(AppData* app);

/**
 * Select the previous item in the current menu.
 * @param app Pointer to the application data
 */
void SelectPreviousItem(AppData* app);

/**
 * Execute the action of the currently selected menu item.
 * @param app Pointer to the application data
 */
void ExecuteMenuAction(AppData* app);

/**
 * Close the currently open popup dialog.
 * @param app Pointer to the application data
 */
void ClosePopup(AppData* app);

/**
 * Navigate popup left/up (previous item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemLeft(AppData* app);

/**
 * Navigate popup right/down (next item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemRight(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * Notes Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move selected note down in the list.
 * @param app Pointer to the application data
 */
void NoteDownApp(AppData* app);

/**
 * Move selected note up in the list.
 * @param app Pointer to the application data
 */
void NoteUpApp(AppData* app);

/**
 * Toggle the selected note between done and undone.
 * @param app Pointer to the application data
 */
void ToggleTaskAtNotes(AppData* app);

/**
 * Delete the currently selected note.
 * @param app Pointer to the application data
 */
void DeleteNoteAtNotes(AppData* app);

/**
 * Add a new task with [ ] prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewTask(AppData* app); /* Add task with [ ] prefix */

/**
 * Add a new note with - prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewNote(AppData* app); /* Add note with - prefix */

/**
 * Add a subtask under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubtask(AppData* app); /* Add subtask under selected node */

/**
 * Add a subnote under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubnote(AppData* app); /* Add subnote under selected node */

/**
 * Edit the selected note content (NORMAL mode).
 * Switches to INSERT mode with existing content loaded.
 * @param app Pointer to the application data
 */
void EditCurrentNote(AppData* app); /* Edit selected node, NORMAL mode */

/**
 * Undo last note operation.
 * @param app Pointer to the application data
 */
void UndoNotes(AppData* app);

/**
 * Redo last undone note operation.
 * @param app Pointer to the application data
 */
void RedoNotes(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * Move Mode Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Toggle move mode for reorganizing notes.
 * @param app Pointer to the application data
 */
void ToggleMoveMode(AppData* app);

/**
 * Exit move mode and return to normal interaction.
 * @param app Pointer to the application data
 */
void ExitMoveMode(AppData* app);

/**
 * Move the selected note up one position.
 * @param app Pointer to the application data
 */
void MoveNoteUpWrapper(AppData* app);

/**
 * Move the selected note down one position.
 * @param app Pointer to the application data
 */
void MoveNoteDownWrapper(AppData* app);

/**
 * Promote note (move to parent level, decrease depth).
 * @param app Pointer to the application data
 */
void PromoteNoteWrapper(AppData* app);

/**
 * Demote note (move to child of previous sibling, increase depth).
 * @param app Pointer to the application data
 */
void DemoteNoteWrapper(AppData* app);

/**
 * Quit notes scene and return to previous scene.
 * @param app Pointer to the application data
 */
void QuitAppNotes(AppData* app);

#endif /* INPUT_H_ */
