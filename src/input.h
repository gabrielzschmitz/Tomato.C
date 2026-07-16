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
#define CTRLH 8
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
  MAIN_MENU,        /**< Main menu scene */
  WORK_TIME,        /**< Work session timer scene */
  SHORT_PAUSE,      /**< Short break timer scene */
  LONG_PAUSE,       /**< Long break timer scene */
  NOTES,            /**< Notes/text editor scene */
  NOTES_TRANSITION, /**< Notes page transition animation scene */
  HELP,             /**< Help screen scene */
  CONTINUE,         /**< Continue/pause scene */
  NOISE,            /**< White noise control dialog scene */
  HISTORY_OVERVIEW, /**< History contribution graph overview */
  HISTORY_DAY,      /**< History single-day session details */
  HISTORY_STATS,    /**< History statistics popup */
  PREFERENCES,      /**< Preferences dialog */
  PREFS_STEPPER,    /**< Preferences stepper sub-dialog */
  PREFS_SELECT,     /**< Preferences select sub-dialog */
} SceneType;

/* Scene type bitmasks for key binding filters */
#define SCENE_MAIN_MENU (1 << MAIN_MENU)
#define SCENE_WORK_TIME (1 << WORK_TIME)
#define SCENE_SHORT_PAUSE (1 << SHORT_PAUSE)
#define SCENE_LONG_PAUSE (1 << LONG_PAUSE)
#define SCENE_NOTES (1 << NOTES)
#define SCENE_NOTES_TRANSITION (1 << NOTES_TRANSITION)
#define SCENE_HELP (1 << HELP)
#define SCENE_CONTINUE (1 << CONTINUE)
#define SCENE_NOISE (1 << NOISE)
#define SCENE_HISTORY_OVERVIEW (1 << HISTORY_OVERVIEW)
#define SCENE_HISTORY_DAY (1 << HISTORY_DAY)
#define SCENE_HISTORY_STATS (1 << HISTORY_STATS)
#define SCENE_PREFERENCES (1 << PREFERENCES)
#define SCENE_PREFS_STEPPER (1 << PREFS_STEPPER)
#define SCENE_PREFS_SELECT (1 << PREFS_SELECT)

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
  int modes;         /**< Bitmask of input modes where this key is active */
  int scene_types;   /**< Bitmask of scene types where this key is active */
  const char* group; /**< Help section name for grouping, e.g. "General" */
  const char* desc;  /**< Human-readable description for help display */
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
 * Select the previous/left button in a slide dialog's control bar.
 * For SLIDE_TYPE_CONTINUE: cycles hovered index toward 0.
 * @param app Application state
 */
void SelectPrevButton(AppData* app);

/**
 * Select the next/right button in a slide dialog's control bar.
 * For SLIDE_TYPE_CONTINUE: cycles hovered index toward end.
 * @param app Application state
 */
void SelectNextButton(AppData* app);

/**
 * Execute the currently hovered button's action in a slide dialog.
 * For SLIDE_TYPE_CONTINUE: invokes def->controls->buttons[hovered].action.
 * @param app Application state
 */
void ExecuteButtonAction(AppData* app);

/**
 * Handle all user input based on current app state.
 * Reads input from terminal and routes to appropriate handler.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInputs(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * Mouse Handlers
 * ---------------------------------------------------------------------------
 */

/**
 * Handle mouse events. In DEFAULT mode: hover updates menu selection +
 * switches panel focus on REQUEST_MOUSE_POSITION, clicks execute actions or
 * switch panels. In non-DEFAULT mode: movement is ignored, first click goes
 * directly to DEFAULT mode.
 * @param app Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleMouseEvent(AppData* app, MEVENT* event);

/**
 * ---------------------------------------------------------------------------
 * Keyboard Handlers
 * ---------------------------------------------------------------------------
 */

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
 * Open the white noise control dialog.
 * @param app Pointer to the application data
 */
void OpenNoiseMenu(AppData* app);

/**
 * Close the white noise dialog.
 * @param app Pointer to the application data
 */
void NoiseClose(AppData* app);

/**
 * Select the previous noise track.
 * @param app Pointer to the application data
 */
void NoiseSelectPrev(AppData* app);

/**
 * Select the next noise track.
 * @param app Pointer to the application data
 */
void NoiseSelectNext(AppData* app);

/**
 * Toggle play/stop for the currently selected noise track.
 * @param app Pointer to the application data
 */
void NoiseTogglePlay(AppData* app);

/**
 * Increase the volume of the currently selected noise track or master.
 * @param app Pointer to the application data
 */
void NoiseVolumeUp(AppData* app);

/**
 * Decrease the volume of the currently selected noise track or master.
 * @param app Pointer to the application data
 */
void NoiseVolumeDown(AppData* app);

/**
 * Reset all noise tracks to default state.
 * @param app Pointer to the application data
 */
void NoiseResetAll(AppData* app);

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
 * Return to main menu from a pomodoro session.
 * Saves the session to log and pauses the timer.
 * @param app Pointer to the application data
 */
void ReturnToMainMenu(AppData* app);

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
 * Navigate to the previous slide in a welcome dialog.
 * @param app Pointer to the application data
 */
void GoPrevSlide(AppData* app);

/**
 * Navigate to the next slide in a welcome dialog.
 * @param app Pointer to the application data
 */
void GoNextSlide(AppData* app);

/**
 * Continue a previous unfinished pomodoro session.
 * Resumes the saved scene and hides the menu.
 * @param app Pointer to the application data
 */
void ContinuePreviousSession(AppData* app);

/**
 * Abandon a previous unfinished pomodoro session.
 * Resets pomodoro data to defaults and removes the
 * uncompleted log entry so the popup won't reappear.
 * @param app Pointer to the application data
 */
void AbandonPreviousSession(AppData* app);

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
 * Navigate to the next page of notes.
 * @param app Pointer to the application data
 */
void NotesNextPage(AppData* app);

/**
 * Navigate to the previous page of notes.
 * @param app Pointer to the application data
 */
void NotesPrevPage(AppData* app);

/**
 * Quit notes scene and return to previous scene.
 * @param app Pointer to the application data
 */
void QuitAppNotes(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * History Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Open the History Overview popup (contribution graph).
 * Called from menu action or via CTRL+H keybind.
 * @param app Application state
 */
void OpenHistoryPopup(AppData* app);

/**
 * Close current history view and re-open the Overview popup.
 * @param app Application state
 */
void HistoryCloseToOverview(AppData* app);

/**
 * Switch from overview to day-detail popup.
 * @param app Application state
 */
void HistoryOpenDayDetail(AppData* app);

/**
 * Switch from overview to statistics popup.
 * @param app Application state
 */
void HistoryOpenStatistics(AppData* app);

/**
 * Move cursor left (overview).
 * @param app Application state
 */
void HistoryCursorLeft(AppData* app);

/**
 * Move cursor right (overview).
 * @param app Application state
 */
void HistoryCursorRight(AppData* app);

/**
 * Move cursor up (overview).
 * @param app Application state
 */
void HistoryCursorUp(AppData* app);

/**
 * Move cursor down (overview).
 * @param app Application state
 */
void HistoryCursorDown(AppData* app);

/**
 * Scroll session list up (day detail).
 * @param app Application state
 */
void HistoryScrollUp(AppData* app);

/**
 * Scroll session list down (day detail).
 * @param app Application state
 */
void HistoryScrollDown(AppData* app);

/**
 * ---------------------------------------------------------------------------
 * Preferences Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Open the preferences dialog from the main menu.
 * @param app Application state
 */
void OpenPreferencesMenu(AppData* app);

/**
 * Open the help popup filtered by the current context scene.
 * Saves the active popup (if any) before opening help, and restores it on close.
 * @param app Application state
 */
void OpenHelp(AppData* app);

/**
 * Open the help popup showing ALL keybindings (full reference).
 * Called from the main menu "help menu" entry.
 * @param app Application state
 */
void OpenHelpMenu(AppData* app);

/**
 * Scroll the help content up (reveal earlier sections).
 * @param app Application state
 */
void HelpScrollUp(AppData* app);

/**
 * Scroll the help content down (reveal later sections).
 * @param app Application state
 */
void HelpScrollDown(AppData* app);

/**
 * Select the previous setting in the preferences dialog.
 * @param app Application state
 */
void PrefsSelectPrev(AppData* app);

/**
 * Select the next setting in the preferences dialog.
 * @param app Application state
 */
void PrefsSelectNext(AppData* app);

/**
 * Decrease the value of the selected setting.
 * @param app Application state
 */
void PrefsValueDown(AppData* app);

/**
 * Increase the value of the selected setting.
 * @param app Application state
 */
void PrefsValueUp(AppData* app);

/**
 * Toggle a boolean setting.
 * @param app Application state
 */
void PrefsToggle(AppData* app);

/**
 * Open the edit sub-dialog for the selected setting (stepper/selector).
 * @param app Application state
 */
void PrefsEdit(AppData* app);

/**
 * Go back from the preferences dialog.
 * @param app Application state
 */
void PrefsBack(AppData* app);

/**
 * Preview the currently selected setting. For sound/notification fields: plays
 * the example audio/test notification. Not bound to keyboard, only accessible
 * via mouse hover in stepper/select popups.
 * @param app Application state
 */
void PrefsPreview(AppData* app);

/**
 * Scroll the preferences list up (reveal earlier items).
 * @param app Application state
 */
void PrefsScrollUp(AppData* app);

/**
 * Scroll the preferences list down (reveal later items).
 * @param app Application state
 */
void PrefsScrollDown(AppData* app);

/**
 * Decrement the value in a preferences stepper sub-dialog.
 * Used for numeric settings (stepper INT/FLOAT).
 * @param app Application state
 */
void StepperDecrement(AppData* app);

/**
 * Increment the value in a preferences stepper sub-dialog.
 * Used for numeric settings (stepper INT/FLOAT).
 * @param app Application state
 */
void StepperIncrement(AppData* app);

/**
 * Close the stepper sub-dialog and return to the main preferences dialog.
 * @param app Application state
 */
void StepperClose(AppData* app);

/**
 * Apply the selected value and close the option selector sub-dialog.
 * @param app Application state
 */
void SelectApply(AppData* app);

/**
 * Cancel selection and return to the main preferences dialog.
 * @param app Application state
 */
void SelectCancel(AppData* app);

/**
 * Navigate to the previous option in a preferences select sub-dialog.
 * @param app Application state
 */
void SelectPrevOption(AppData* app);

/**
 * Navigate to the next option in a preferences select sub-dialog.
 * @param app Application state
 */
void SelectNextOption(AppData* app);

#endif /* INPUT_H_ */
