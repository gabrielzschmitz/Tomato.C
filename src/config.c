#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "external/toml-c.h"
#include "util.h"

/* toml-c.h poisons strdup — restore the system declaration for our use */
#undef strdup

/** Path to the system-wide TOML configuration file. */
const char* SYSTEM_CONFIG_PATH = "/etc/tomato/config.toml";
/**
 * Default user-config directory name (appended to $HOME).
 * The full config path becomes $HOME/.config/tomato/config.toml.
 */
const char* DEFAULT_USER_CONFIG_DIR = ".config";

/**
 * Compile-time default keybinding array.
 *
 * Every (key, action, mode, scene) tuple is listed here with first-match
 * priority — earlier entries override later ones for the same key+scene.
 * When a TOML config provides keybindings this array is replaced entirely.
 */
const KeyFunction DEFAULT_KEYS[] = {
  /* NORMAL mode - editing keys (when Panel.input != NULL) */
  {'h', InputCursorLeft, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Move cursor left"},
  {'l', InputCursorRight, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Move cursor right"},
  {KEY_LEFT, InputCursorLeft, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Move cursor left"},
  {KEY_RIGHT, InputCursorRight, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Move cursor right"},
  {'x', InputDeleteChar, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Delete character"},
  {ESC, InputESC, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Exit to DEFAULT mode"},
  {CTRLC, InputESC, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Exit to DEFAULT mode"},
  {ENTER, InputCommit, NORMAL, SCENE_NOTES, "Notes (Normal)", "Confirm input"},
  {'\r', InputCommit, NORMAL, SCENE_NOTES, "Notes (Normal)", "Confirm input"},
  {KEY_ENTER, InputCommit, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Confirm input"},
  {'i', SwitchToInsertMode, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Switch to INSERT mode"},
  {'a', SwitchToInsertModeAppend, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Append after cursor"},
  {'v', SwitchToVisualMode, NORMAL, SCENE_NOTES, "Notes (Normal)",
   "Enter VISUAL mode"},
  {'u', UndoNotes, NORMAL, SCENE_NOTES, "Notes (Normal)", "Undo last change"},
  {CTRLR, RedoNotes, NORMAL, SCENE_NOTES, "Notes (Normal)", "Redo last undo"},

  /* INSERT mode keys */
  {KEY_LEFT, InputCursorLeft, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Move cursor left"},
  {KEY_RIGHT, InputCursorRight, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Move cursor right"},
  {KEY_BACKSPACE, InputBackspace, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Delete character before cursor"},
  {BACKSPACE, InputBackspace, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Delete character before cursor"},
  {ENTER, InputCommit, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Commit text input"},
  {'\r', InputCommit, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Commit text input"},
  {KEY_ENTER, InputCommit, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Commit text input"},
  {ESC, InputESC, INSERT, SCENE_NOTES, "Notes (Insert)", "Exit to NORMAL mode"},
  {CTRLC, InputESC, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Exit to NORMAL mode"},
  {'v', SwitchToVisualMode, INSERT, SCENE_NOTES, "Notes (Insert)",
   "Enter VISUAL mode"},
  {-1, InputInsertChar, INSERT, SCENE_NOTES, NULL, NULL}, /* printable chars */

  /* VISUAL mode keys */
  {'h', InputCursorLeft, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Extend selection left"},
  {'l', InputCursorRight, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Extend selection right"},
  {KEY_LEFT, InputCursorLeft, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Extend selection left"},
  {KEY_RIGHT, InputCursorRight, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Extend selection right"},
  {'x', InputVisualDelete, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Delete selection"},
  {ENTER, InputCommit, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Confirm selection"},
  {'\r', InputCommit, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Confirm selection"},
  {KEY_ENTER, InputCommit, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Confirm selection"},
  {ESC, InputESC, VISUAL, SCENE_NOTES, "Notes (Visual)", "Exit to NORMAL mode"},
  {CTRLC, InputESC, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Exit to NORMAL mode"},
  {'a', InputSwitchToInsertFromVisual, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Insert after selection"},
  {'i', SwitchToInsertMode, VISUAL, SCENE_NOTES, "Notes (Visual)",
   "Switch to INSERT mode"},

  /* Scene-specific keybindings - Before ALL_SCENES keys */
  {'V', ToggleMoveMode, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Toggle move mode"},
  {'j', MoveNoteDownWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Move note down"},
  {'k', MoveNoteUpWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Move note up"},
  {KEY_DOWN, MoveNoteDownWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Move note down"},
  {KEY_UP, MoveNoteUpWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Move note up"},
  {'h', NotesPrevPage, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Previous page"},
  {KEY_LEFT, NotesPrevPage, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Previous page"},
  {'l', NotesNextPage, DEFAULT, SCENE_NOTES, "Notes (Default)", "Next page"},
  {KEY_RIGHT, NotesNextPage, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Next page"},
  {'h', PromoteNoteWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Promote note"},
  {KEY_LEFT, PromoteNoteWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Promote note"},
  {'l', DemoteNoteWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Demote note"},
  {KEY_RIGHT, DemoteNoteWrapper, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Demote note"},
  {'d', DeleteNoteAtNotes, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Delete note"},
  {'t', AddNewTask, DEFAULT, SCENE_NOTES, "Notes (Default)", "Add new task"},
  {'n', AddNewNote, DEFAULT, SCENE_NOTES, "Notes (Default)", "Add new note"},
  {'T', AddSubtask, DEFAULT, SCENE_NOTES, "Notes (Default)", "Add subtask"},
  {'N', AddSubnote, DEFAULT, SCENE_NOTES, "Notes (Default)", "Add subnote"},
  {'e', EditCurrentNote, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Edit current note"},
  {ENTER, ExitMoveMode, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Exit move mode"},
  {ENTER, ToggleTaskAtNotes, DEFAULT, SCENE_NOTES, "Notes (Default)",
   "Toggle task done"},
  {'u', UndoNotes, DEFAULT, SCENE_NOTES, "Notes (Default)", "Undo last change"},
  {CTRLR, RedoNotes, DEFAULT, SCENE_NOTES, "Notes (Default)", "Redo last undo"},
  {CTRLC, QuitAppNotes, DEFAULT, SCENE_NOTES, "Notes (Default)", "Quit notes"},
  {ESC, QuitAppNotes, DEFAULT, SCENE_NOTES, "Notes (Default)", "Quit notes"},
  {'q', QuitAppNotes, DEFAULT, SCENE_NOTES, "Notes (Default)", "Quit notes"},
  {KEY_DOWN, SelectNextItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select next item"},
  {KEY_UP, SelectPreviousItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select previous item"},
  {KEY_RIGHT, SelectNextItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select next item"},
  {KEY_LEFT, SelectPreviousItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select previous item"},
  {'j', SelectNextItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select next item"},
  {'k', SelectPreviousItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select previous item"},
  {'l', SelectNextItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select next item"},
  {'h', SelectPreviousItem, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Select previous item"},
  {ENTER, ExecuteMenuAction, DEFAULT, SCENE_MAIN_MENU, "Main Menu",
   "Execute menu action"},

  /* Preferences dialog — navigation */
  {KEY_UP, PrefsSelectPrev, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Select previous setting"},
  {'k', PrefsSelectPrev, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Select previous setting"},
  {KEY_DOWN, PrefsSelectNext, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Select next setting"},
  {'j', PrefsSelectNext, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Select next setting"},
  {'u', PrefsScrollUp, DEFAULT, SCENE_PREFERENCES, "Preferences", "Scroll up"},
  {'d', PrefsScrollDown, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Scroll down"},
  {KEY_LEFT, PrefsValueDown, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Decrease value"},
  {'h', PrefsValueDown, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Decrease value"},
  {KEY_RIGHT, PrefsValueUp, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Increase value"},
  {'l', PrefsValueUp, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Increase value"},
  {' ', PrefsToggle, DEFAULT, SCENE_PREFERENCES, "Preferences", "Toggle value"},
  {ENTER, PrefsEdit, DEFAULT, SCENE_PREFERENCES, "Preferences", "Edit value"},
  {'\r', PrefsEdit, DEFAULT, SCENE_PREFERENCES, "Preferences", "Edit value"},
  {KEY_ENTER, PrefsEdit, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Edit value"},
  {'q', PrefsBack, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Close preferences"},
  {ESC, PrefsBack, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Close preferences"},
  {CTRLC, PrefsBack, DEFAULT, SCENE_PREFERENCES, "Preferences",
   "Close preferences"},

  /* Preferences stepper sub-dialog */
  {'h', StepperDecrement, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Decrement value"},
  {KEY_LEFT, StepperDecrement, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Decrement value"},
  {'l', StepperIncrement, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Increment value"},
  {KEY_RIGHT, StepperIncrement, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Increment value"},
  {'p', PrefsPreview, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Preview setting"},
  {ENTER, StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Confirm and close"},
  {'\r', StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Confirm and close"},
  {KEY_ENTER, StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Confirm and close"},
  {'q', StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Cancel and close"},
  {ESC, StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Cancel and close"},
  {CTRLC, StepperClose, DEFAULT, SCENE_PREFS_STEPPER, "Preferences",
   "Cancel and close"},

  /* Preferences select sub-dialog */
  {'k', SelectPrevOption, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Select previous option"},
  {KEY_UP, SelectPrevOption, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Select previous option"},
  {'j', SelectNextOption, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Select next option"},
  {KEY_DOWN, SelectNextOption, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Select next option"},
  {'p', PrefsPreview, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Preview setting"},
  {ENTER, SelectApply, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Apply selection"},
  {'\r', SelectApply, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Apply selection"},
  {KEY_ENTER, SelectApply, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Apply selection"},
  {'q', SelectCancel, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Cancel selection"},
  {ESC, SelectCancel, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Cancel selection"},
  {CTRLC, SelectCancel, DEFAULT, SCENE_PREFS_SELECT, "Preferences",
   "Cancel selection"},

  /* General keybindings - DEFAULT mode, ALL_SCENES */
  {KEY_UP, ChangeSelectedItemLeft, DEFAULT, ALL_SCENES, "General",
   "Select previous item"},
  {'k', ChangeSelectedItemLeft, DEFAULT, ALL_SCENES, "General",
   "Select previous item"},
  {KEY_LEFT, ChangeSelectedItemLeft, DEFAULT, ALL_SCENES, "General",
   "Select previous item"},
  {'h', ChangeSelectedItemLeft, DEFAULT, ALL_SCENES, "General",
   "Select previous item"},
  {KEY_DOWN, ChangeSelectedItemRight, DEFAULT, ALL_SCENES, "General",
   "Select next item"},
  {'j', ChangeSelectedItemRight, DEFAULT, ALL_SCENES, "General",
   "Select next item"},
  {KEY_RIGHT, ChangeSelectedItemRight, DEFAULT, ALL_SCENES, "General",
   "Select next item"},
  {'l', ChangeSelectedItemRight, DEFAULT, ALL_SCENES, "General",
   "Select next item"},
  {ENTER, ExecuteMenuAction, DEFAULT, ALL_SCENES, "General",
   "Execute selected action"},
  {KEY_ENTER, ExecuteMenuAction, DEFAULT, ALL_SCENES, "General",
   "Execute selected action"},
  {'\n', ExecuteMenuAction, DEFAULT, ALL_SCENES, "General",
   "Execute selected action"},
  {'\r', ExecuteMenuAction, DEFAULT, ALL_SCENES, "General",
   "Execute selected action"},
  {' ', NextPanel, DEFAULT, ALL_SCENES, "General", "Switch panel focus"},
  {'s', SkipPomodoroStep, DEFAULT, POMODORO_SCENES, "Pomodoro",
   "Skip current step"},
  {'p', TogglePause, DEFAULT, POMODORO_SCENES, "Pomodoro",
   "Toggle pause/resume"},
  {CTRLR, OpenResetMenu, DEFAULT, POMODORO_SCENES, "Pomodoro",
   "Open reset menu"},
  {'q', QuitApp, DEFAULT, ALL_SCENES, "General", "Quit application"},
  {ESC, QuitApp, DEFAULT, ALL_SCENES, "General", "Quit application"},
  {CTRLC, QuitApp, DEFAULT, ALL_SCENES, "General", "Quit application"},

  /* Slide Navigation */
  {KEY_LEFT, GoPrevSlide, DEFAULT, ALL_SCENES, "General", "Previous slide"},
  {'h', GoPrevSlide, DEFAULT, ALL_SCENES, "General", "Previous slide"},
  {KEY_RIGHT, GoNextSlide, DEFAULT, ALL_SCENES, "General", "Next slide"},
  {'l', GoNextSlide, DEFAULT, ALL_SCENES, "General", "Next slide"},
  {ENTER, ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},
  {'\r', ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},
  {KEY_ENTER, ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},
  {'q', ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},
  {ESC, ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},
  {CTRLC, ClosePopup, DEFAULT, ALL_SCENES, "General", "Close dialog"},

  /* Continue dialog — button navigation */
  {KEY_LEFT, SelectPrevButton, DEFAULT, ALL_SCENES, "General",
   "Select previous button"},
  {'h', SelectPrevButton, DEFAULT, ALL_SCENES, "General",
   "Select previous button"},
  {KEY_RIGHT, SelectNextButton, DEFAULT, ALL_SCENES, "General",
   "Select next button"},
  {'l', SelectNextButton, DEFAULT, ALL_SCENES, "General", "Select next button"},
  {ENTER, ExecuteButtonAction, DEFAULT, ALL_SCENES, "General",
   "Execute button action"},
  {'\r', ExecuteButtonAction, DEFAULT, ALL_SCENES, "General",
   "Execute button action"},
  {KEY_ENTER, ExecuteButtonAction, DEFAULT, ALL_SCENES, "General",
   "Execute button action"},

  /* White noise dialog controls */
  {CTRLW, OpenNoiseMenu, DEFAULT, ALL_SCENES, "General", "Open white noise"},
  {'w', OpenNoiseMenu, DEFAULT, ALL_SCENES, "General", "Open white noise"},
  {'q', NoiseClose, DEFAULT, SCENE_NOISE, "Noise", "Close noise dialog"},
  {ESC, NoiseClose, DEFAULT, SCENE_NOISE, "Noise", "Close noise dialog"},
  {CTRLC, NoiseClose, DEFAULT, SCENE_NOISE, "Noise", "Close noise dialog"},
  {'k', NoiseSelectPrev, DEFAULT, SCENE_NOISE, "Noise",
   "Select previous track"},
  {KEY_UP, NoiseSelectPrev, DEFAULT, SCENE_NOISE, "Noise",
   "Select previous track"},
  {'j', NoiseSelectNext, DEFAULT, SCENE_NOISE, "Noise", "Select next track"},
  {KEY_DOWN, NoiseSelectNext, DEFAULT, SCENE_NOISE, "Noise",
   "Select next track"},
  {' ', NoiseTogglePlay, DEFAULT, SCENE_NOISE, "Noise", "Toggle play/stop"},
  {'l', NoiseVolumeUp, DEFAULT, SCENE_NOISE, "Noise", "Increase volume"},
  {KEY_RIGHT, NoiseVolumeUp, DEFAULT, SCENE_NOISE, "Noise", "Increase volume"},
  {'+', NoiseVolumeUp, DEFAULT, SCENE_NOISE, "Noise", "Increase volume"},
  {'=', NoiseVolumeUp, DEFAULT, SCENE_NOISE, "Noise", "Increase volume"},
  {'h', NoiseVolumeDown, DEFAULT, SCENE_NOISE, "Noise", "Decrease volume"},
  {KEY_LEFT, NoiseVolumeDown, DEFAULT, SCENE_NOISE, "Noise", "Decrease volume"},
  {'-', NoiseVolumeDown, DEFAULT, SCENE_NOISE, "Noise", "Decrease volume"},
  {'_', NoiseVolumeDown, DEFAULT, SCENE_NOISE, "Noise", "Decrease volume"},
  {'r', NoiseResetAll, DEFAULT, SCENE_NOISE, "Noise", "Reset all tracks"},
  {'R', NoiseResetAll, DEFAULT, SCENE_NOISE, "Noise", "Reset all tracks"},

  /* Continue dialog — keyboard navigation */
  {'l', SelectNextButton, DEFAULT, SCENE_CONTINUE, "Continue",
   "Select next button"},
  {'h', SelectPrevButton, DEFAULT, SCENE_CONTINUE, "Continue",
   "Select previous button"},
  {KEY_RIGHT, SelectNextButton, DEFAULT, SCENE_CONTINUE, "Continue",
   "Select next button"},
  {KEY_LEFT, SelectPrevButton, DEFAULT, SCENE_CONTINUE, "Continue",
   "Select previous button"},
  {ENTER, ExecuteButtonAction, DEFAULT, SCENE_CONTINUE, "Continue",
   "Execute button action"},
  {'\r', ExecuteButtonAction, DEFAULT, SCENE_CONTINUE, "Continue",
   "Execute button action"},
  {KEY_ENTER, ExecuteButtonAction, DEFAULT, SCENE_CONTINUE, "Continue",
   "Execute button action"},
  {'q', ClosePopup, DEFAULT, SCENE_CONTINUE, "Continue", "Close dialog"},
  {ESC, ClosePopup, DEFAULT, SCENE_CONTINUE, "Continue", "Close dialog"},
  {CTRLC, ClosePopup, DEFAULT, SCENE_CONTINUE, "Continue", "Close dialog"},

  /* History Overview */
  {CTRLH, OpenHistoryPopup, DEFAULT, ALL_SCENES, "General", "Open history"},
  {'h', HistoryCursorLeft, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor left"},
  {'l', HistoryCursorRight, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor right"},
  {'j', HistoryCursorDown, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor down"},
  {'k', HistoryCursorUp, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor up"},
  {KEY_LEFT, HistoryCursorLeft, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor left"},
  {KEY_RIGHT, HistoryCursorRight, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor right"},
  {KEY_DOWN, HistoryCursorDown, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor down"},
  {KEY_UP, HistoryCursorUp, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Move cursor up"},
  {ENTER, HistoryOpenDayDetail, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Open day detail"},
  {'\r', HistoryOpenDayDetail, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Open day detail"},
  {'\t', HistoryOpenStatistics, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Open statistics"},
  {'q', ClosePopup, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Close history"},
  {ESC, ClosePopup, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Close history"},
  {CTRLC, ClosePopup, DEFAULT, SCENE_HISTORY_OVERVIEW, "History",
   "Close history"},

  /* History Day Detail */
  {'j', HistoryScrollDown, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Scroll session list down"},
  {'k', HistoryScrollUp, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Scroll session list up"},
  {KEY_DOWN, HistoryScrollDown, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Scroll session list down"},
  {KEY_UP, HistoryScrollUp, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Scroll session list up"},
  {ENTER, HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Back to overview"},
  {'\r', HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Back to overview"},
  {CTRLC, HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Back to overview"},
  {ESC, HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Back to overview"},
  {'q', HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_DAY, "History",
   "Back to overview"},

  /* History Statistics */
  {'\t', HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_STATS, "History",
   "Back to overview"},
  {CTRLC, HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_STATS, "History",
   "Back to overview"},
  {ESC, HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_STATS, "History",
   "Back to overview"},
  {'q', HistoryCloseToOverview, DEFAULT, SCENE_HISTORY_STATS, "History",
   "Back to overview"},

  /* Open help — bound to ? and F1, works everywhere */
  {'?', OpenHelp, DEFAULT, ALL_SCENES, "General", "Open context help"},
  {KEY_F(1), OpenHelp, DEFAULT, ALL_SCENES, "General", "Open context help"},

  /* Help dialog — navigation (self-bound, not shown in help content) */
  {KEY_UP, HelpScrollUp, DEFAULT, SCENE_HELP, NULL, NULL},
  {'k', HelpScrollUp, DEFAULT, SCENE_HELP, NULL, NULL},
  {KEY_DOWN, HelpScrollDown, DEFAULT, SCENE_HELP, NULL, NULL},
  {'j', HelpScrollDown, DEFAULT, SCENE_HELP, NULL, NULL},
  {KEY_PPAGE, HelpScrollUp, DEFAULT, SCENE_HELP, NULL, NULL},
  {KEY_NPAGE, HelpScrollDown, DEFAULT, SCENE_HELP, NULL, NULL},
  {'q', ClosePopup, DEFAULT, SCENE_HELP, NULL, NULL},
  {ESC, ClosePopup, DEFAULT, SCENE_HELP, NULL, NULL},
  {CTRLC, ClosePopup, DEFAULT, SCENE_HELP, NULL, NULL},
};

/** Number of entries in DEFAULT_KEYS[]. */
const size_t DEFAULT_KEYS_COUNT =
  sizeof(DEFAULT_KEYS) / sizeof(DEFAULT_KEYS[0]);

/** Global runtime configuration singleton. */
Config g_config;

/* ── Key / action / mode / scene string-to-enum tables ── */

/**
 * Maps TOML key names to ncurses/input keycodes.
 * Used internally by readKeybindings().
 */
static const struct {
  const char* name; /**< TOML key name */
  int key;          /**< ncurses key constant */
} keycode_map[] = {
  {"KEY_LEFT", KEY_LEFT},
  {"KEY_RIGHT", KEY_RIGHT},
  {"KEY_UP", KEY_UP},
  {"KEY_DOWN", KEY_DOWN},
  {"KEY_ENTER", KEY_ENTER},
  {"KEY_BACKSPACE", KEY_BACKSPACE},
  {"BACKSPACE", BACKSPACE},
  {"ENTER", ENTER},
  {"ESC", ESC},
  {"CTRLC", CTRLC},
  {"CTRLR", CTRLR},
  {"CTRLW", CTRLW},
  {"CTRLH", CTRLH},
  {"SPACE", ' '},
};

/**
 * Maps TOML action names to function pointers.
 * Every entry must match a KeyFunction action used in DEFAULT_KEYS[].
 */
static const struct {
  const char* name;         /**< TOML action name */
  void (*action)(AppData*); /**< callback pointer */
} action_map[] = {
  {"InputCursorLeft", InputCursorLeft},
  {"InputCursorRight", InputCursorRight},
  {"InputDeleteChar", InputDeleteChar},
  {"InputESC", InputESC},
  {"InputCommit", InputCommit},
  {"SwitchToInsertMode", SwitchToInsertMode},
  {"SwitchToInsertModeAppend", SwitchToInsertModeAppend},
  {"SwitchToVisualMode", SwitchToVisualMode},
  {"UndoNotes", UndoNotes},
  {"RedoNotes", RedoNotes},
  {"InputBackspace", InputBackspace},
  {"InputInsertChar", InputInsertChar},
  {"InputVisualDelete", InputVisualDelete},
  {"InputSwitchToInsertFromVisual", InputSwitchToInsertFromVisual},
  {"ToggleMoveMode", ToggleMoveMode},
  {"ExitMoveMode", ExitMoveMode},
  {"MoveNoteDownWrapper", MoveNoteDownWrapper},
  {"MoveNoteUpWrapper", MoveNoteUpWrapper},
  {"PromoteNoteWrapper", PromoteNoteWrapper},
  {"DemoteNoteWrapper", DemoteNoteWrapper},
  {"DeleteNoteAtNotes", DeleteNoteAtNotes},
  {"AddNewTask", AddNewTask},
  {"AddNewNote", AddNewNote},
  {"AddSubtask", AddSubtask},
  {"AddSubnote", AddSubnote},
  {"EditCurrentNote", EditCurrentNote},
  {"ToggleTaskAtNotes", ToggleTaskAtNotes},
  {"QuitAppNotes", QuitAppNotes},
  {"SelectNextItem", SelectNextItem},
  {"SelectPreviousItem", SelectPreviousItem},
  {"ExecuteMenuAction", ExecuteMenuAction},
  {"ChangeSelectedItemLeft", ChangeSelectedItemLeft},
  {"ChangeSelectedItemRight", ChangeSelectedItemRight},
  {"SkipPomodoroStep", SkipPomodoroStep},
  {"TogglePause", TogglePause},
  {"OpenResetMenu", OpenResetMenu},
  {"QuitApp", QuitApp},
  {"GoPrevSlide", GoPrevSlide},
  {"GoNextSlide", GoNextSlide},
  {"ClosePopup", ClosePopup},
  {"SelectPrevButton", SelectPrevButton},
  {"SelectNextButton", SelectNextButton},
  {"ExecuteButtonAction", ExecuteButtonAction},
  {"OpenNoiseMenu", OpenNoiseMenu},
  {"NoiseClose", NoiseClose},
  {"NoiseSelectPrev", NoiseSelectPrev},
  {"NoiseSelectNext", NoiseSelectNext},
  {"NoiseTogglePlay", NoiseTogglePlay},
  {"NoiseVolumeUp", NoiseVolumeUp},
  {"NoiseVolumeDown", NoiseVolumeDown},
  {"NoiseResetAll", NoiseResetAll},
  {"OpenHistoryPopup", OpenHistoryPopup},
  {"HistoryCursorLeft", HistoryCursorLeft},
  {"HistoryCursorRight", HistoryCursorRight},
  {"HistoryCursorDown", HistoryCursorDown},
  {"HistoryCursorUp", HistoryCursorUp},
  {"HistoryOpenDayDetail", HistoryOpenDayDetail},
  {"HistoryOpenStatistics", HistoryOpenStatistics},
  {"HistoryScrollDown", HistoryScrollDown},
  {"HistoryScrollUp", HistoryScrollUp},
  {"HistoryCloseToOverview", HistoryCloseToOverview},
  {"NextPanel", NextPanel},
  {"OpenPreferencesMenu", OpenPreferencesMenu},
  {"PrefsSelectPrev", PrefsSelectPrev},
  {"PrefsSelectNext", PrefsSelectNext},
  {"PrefsValueDown", PrefsValueDown},
  {"PrefsValueUp", PrefsValueUp},
  {"PrefsToggle", PrefsToggle},
  {"PrefsEdit", PrefsEdit},
  {"PrefsBack", PrefsBack},
  {"PrefsPreview", PrefsPreview},
  {"PrefsScrollUp", PrefsScrollUp},
  {"PrefsScrollDown", PrefsScrollDown},
  {"StepperDecrement", StepperDecrement},
  {"StepperIncrement", StepperIncrement},
  {"StepperClose", StepperClose},
  {"SelectApply", SelectApply},
  {"SelectCancel", SelectCancel},
  {"SelectPrevOption", SelectPrevOption},
  {"SelectNextOption", SelectNextOption},
  {"OpenHelp", OpenHelp},
  {"OpenHelpMenu", OpenHelpMenu},
};

/**
 * Maps TOML mode names to input mode constants.
 */
static const struct {
  const char* name; /**< TOML mode name */
  int mode;         /**< input mode constant */
} mode_map[] = {
  {"DEFAULT", DEFAULT},
  {"NORMAL", NORMAL},
  {"INSERT", INSERT},
  {"VISUAL", VISUAL},
};

/**
 * Maps TOML scene names to scene-type bitmask constants.
 */
static const struct {
  const char* name; /**< TOML scene name */
  int scene;        /**< scene-type bitmask */
} scene_map[] = {
  {"SCENE_MAIN_MENU", SCENE_MAIN_MENU},
  {"SCENE_WORK_TIME", SCENE_WORK_TIME},
  {"SCENE_SHORT_PAUSE", SCENE_SHORT_PAUSE},
  {"SCENE_LONG_PAUSE", SCENE_LONG_PAUSE},
  {"SCENE_NOTES", SCENE_NOTES},
  {"SCENE_HELP", SCENE_HELP},
  {"SCENE_CONTINUE", SCENE_CONTINUE},
  {"SCENE_NOISE", SCENE_NOISE},
  {"SCENE_HISTORY_OVERVIEW", SCENE_HISTORY_OVERVIEW},
  {"SCENE_HISTORY_DAY", SCENE_HISTORY_DAY},
  {"SCENE_HISTORY_STATS", SCENE_HISTORY_STATS},
  {"SCENE_PREFERENCES", SCENE_PREFERENCES},
  {"SCENE_PREFS_STEPPER", SCENE_PREFS_STEPPER},
  {"SCENE_PREFS_SELECT", SCENE_PREFS_SELECT},
  {"ALL_SCENES", ALL_SCENES},
  {"POMODORO_SCENES", POMODORO_SCENES},
};

/**
 * Convert a TOML key name string to an ncurses keycode.
 * Single printable characters map to their ASCII value; named constants
 * (KEY_LEFT, ESC, …) are looked up in keycode_map[].
 * @param s TOML key name
 * @return keycode, or 0 on failure
 */
static int keycodeFromString(const char* s) {
  if (!s || !*s) return 0;
  if (strlen(s) == 1 && s[0] >= 32) return (unsigned char)s[0];
  for (size_t i = 0; i < sizeof(keycode_map) / sizeof(keycode_map[0]); i++)
    if (strcmp(s, keycode_map[i].name) == 0) return keycode_map[i].key;
  return 0;
}

/**
 * Convert a TOML action name to a KeyFunction callback.
 * @param s TOML action name
 * @return function pointer, or NULL on failure
 */
static void (*actionFromString(const char* s))(AppData*) {
  for (size_t i = 0; i < sizeof(action_map) / sizeof(action_map[0]); i++)
    if (strcmp(s, action_map[i].name) == 0) return action_map[i].action;
  return NULL;
}

/**
 * Convert a TOML mode name to an input-mode constant.
 * @param s TOML mode name (e.g. "DEFAULT", "NORMAL")
 * @return mode constant, or 0 on failure
 */
static int modeFromString(const char* s) {
  for (size_t i = 0; i < sizeof(mode_map) / sizeof(mode_map[0]); i++)
    if (strcmp(s, mode_map[i].name) == 0) return mode_map[i].mode;
  return 0;
}

/**
 * Convert a TOML scene name to a scene-type bitmask.
 * @param s TOML scene name (e.g. "SCENE_NOTES", "ALL_SCENES")
 * @return scene-type bitmask, or 0 on failure
 */
static int sceneFromString(const char* s) {
  for (size_t i = 0; i < sizeof(scene_map) / sizeof(scene_map[0]); i++)
    if (strcmp(s, scene_map[i].name) == 0) return scene_map[i].scene;
  return 0;
}

/**
 * Initialise every g_config field to its built-in default value.
 *
 * This is called before any TOML file is loaded, so TOML values only need
 * to override the subset the user actually sets.
 */
static void setDefaults(void) {
  g_config.visual.animations = 1;
  g_config.visual.icons = "nerd-icons";
  g_config.visual.bg_transparency = 1;
  g_config.visual.status_bar_spacing = 1;
  g_config.visual.status_bar_position = 0;
  g_config.visual.unfocused_panel_color = 7;
  g_config.visual.focused_panel_color = 1;

  g_config.visual.ui.icons.noise.rain[0] = "󰖖";
  g_config.visual.ui.icons.noise.rain[1] = "☔";
  g_config.visual.ui.icons.noise.rain[2] = "R";
  g_config.visual.ui.icons.noise.fire[0] = "󰈸";
  g_config.visual.ui.icons.noise.fire[1] = "🔥";
  g_config.visual.ui.icons.noise.fire[2] = "F";
  g_config.visual.ui.icons.noise.wind[0] = "󰖝";
  g_config.visual.ui.icons.noise.wind[1] = "🍃";
  g_config.visual.ui.icons.noise.wind[2] = "W";
  g_config.visual.ui.icons.noise.thunder[0] = "󱐋";
  g_config.visual.ui.icons.noise.thunder[1] = "⚡";
  g_config.visual.ui.icons.noise.thunder[2] = "T";

  g_config.visual.ui.icons.pomodoro.main_menu[0] = "󰍜";
  g_config.visual.ui.icons.pomodoro.main_menu[1] = "🧾";
  g_config.visual.ui.icons.pomodoro.main_menu[2] = "";
  g_config.visual.ui.icons.pomodoro.work[0] = "";
  g_config.visual.ui.icons.pomodoro.work[1] = "🍅";
  g_config.visual.ui.icons.pomodoro.work[2] = "";
  g_config.visual.ui.icons.pomodoro.short_pause[0] = "";
  g_config.visual.ui.icons.pomodoro.short_pause[1] = "☕";
  g_config.visual.ui.icons.pomodoro.short_pause[2] = "";
  g_config.visual.ui.icons.pomodoro.long_pause[0] = "";
  g_config.visual.ui.icons.pomodoro.long_pause[1] = "🌴";
  g_config.visual.ui.icons.pomodoro.long_pause[2] = "";
  g_config.visual.ui.icons.pomodoro.notes[0] = "";
  g_config.visual.ui.icons.pomodoro.notes[1] = "📝";
  g_config.visual.ui.icons.pomodoro.notes[2] = "";
  g_config.visual.ui.icons.pomodoro.help[0] = "";
  g_config.visual.ui.icons.pomodoro.help[1] = "⁉️";
  g_config.visual.ui.icons.pomodoro.help[2] = "";
  g_config.visual.ui.icons.pomodoro.continue_icon[0] = "";
  g_config.visual.ui.icons.pomodoro.continue_icon[1] = "⏯️";
  g_config.visual.ui.icons.pomodoro.continue_icon[2] = "";
  g_config.visual.ui.icons.pomodoro.idle[0] = "";
  g_config.visual.ui.icons.pomodoro.idle[1] = "🌙";
  g_config.visual.ui.icons.pomodoro.idle[2] = "";

  g_config.visual.ui.icons.input.default_mode[0] = "";
  g_config.visual.ui.icons.input.default_mode[1] = "🌐";
  g_config.visual.ui.icons.input.default_mode[2] = "";
  g_config.visual.ui.icons.input.normal_mode[0] = "";
  g_config.visual.ui.icons.input.normal_mode[1] = "🧭";
  g_config.visual.ui.icons.input.normal_mode[2] = "";
  g_config.visual.ui.icons.input.insert_mode[0] = "";
  g_config.visual.ui.icons.input.insert_mode[1] = "✏ ";
  g_config.visual.ui.icons.input.insert_mode[2] = "";
  g_config.visual.ui.icons.input.visual_mode[0] = "󰕢";
  g_config.visual.ui.icons.input.visual_mode[1] = "🔲";
  g_config.visual.ui.icons.input.visual_mode[2] = "";
  g_config.visual.ui.icons.input.real_time_module[0] = "";
  g_config.visual.ui.icons.input.real_time_module[1] = "🕘";
  g_config.visual.ui.icons.input.real_time_module[2] = "";
  g_config.visual.ui.icons.input.line_column_module[0] = "";
  g_config.visual.ui.icons.input.line_column_module[1] = "▶";
  g_config.visual.ui.icons.input.line_column_module[2] = "";

  g_config.visual.ui.icons.misc.playing[0] = "";
  g_config.visual.ui.icons.misc.playing[1] = "▶";
  g_config.visual.ui.icons.misc.playing[2] = "P";
  g_config.visual.ui.icons.misc.plus_volume[0] = "";
  g_config.visual.ui.icons.misc.plus_volume[1] = "➕";
  g_config.visual.ui.icons.misc.plus_volume[2] = "+";
  g_config.visual.ui.icons.misc.minus_volume[0] = "";
  g_config.visual.ui.icons.misc.minus_volume[1] = "➖";
  g_config.visual.ui.icons.misc.minus_volume[2] = "-";
  g_config.visual.ui.icons.misc.active_volume_bar[0] = "█";
  g_config.visual.ui.icons.misc.active_volume_bar[1] = "█";
  g_config.visual.ui.icons.misc.active_volume_bar[2] = "█";
  g_config.visual.ui.icons.misc.inactive_volume_bar[0] = "▒";
  g_config.visual.ui.icons.misc.inactive_volume_bar[1] = "▒";
  g_config.visual.ui.icons.misc.inactive_volume_bar[2] = "▒";
  g_config.visual.ui.icons.misc.active_progress_bar[0] = "█";
  g_config.visual.ui.icons.misc.active_progress_bar[1] = "█";
  g_config.visual.ui.icons.misc.active_progress_bar[2] = "█";
  g_config.visual.ui.icons.misc.inactive_progress_bar[0] = "▒";
  g_config.visual.ui.icons.misc.inactive_progress_bar[1] = "▒";
  g_config.visual.ui.icons.misc.inactive_progress_bar[2] = "▒";
  g_config.visual.ui.icons.misc.pause[0] = "󰏤";
  g_config.visual.ui.icons.misc.pause[1] = "⏸️";
  g_config.visual.ui.icons.misc.pause[2] = "P";
  g_config.visual.ui.icons.misc.play[0] = "󰐊";
  g_config.visual.ui.icons.misc.play[1] = "▶️";
  g_config.visual.ui.icons.misc.play[2] = "P";
  g_config.visual.ui.icons.misc.skip[0] = "󰒬";
  g_config.visual.ui.icons.misc.skip[1] = "⏭️";
  g_config.visual.ui.icons.misc.skip[2] = "S";
  g_config.visual.ui.icons.misc.visual_cursor = "█";
  g_config.visual.ui.icons.misc.insert_cursor = "▏";
  g_config.visual.ui.icons.misc.border_chars[0] = "┏";
  g_config.visual.ui.icons.misc.border_chars[1] = "┓";
  g_config.visual.ui.icons.misc.border_chars[2] = "┗";
  g_config.visual.ui.icons.misc.border_chars[3] = "┛";
  g_config.visual.ui.icons.misc.border_chars[4] = "━";
  g_config.visual.ui.icons.misc.border_chars[5] = "┃";
  g_config.visual.ui.icons.misc.history[0] = "░░";
  g_config.visual.ui.icons.misc.history[1] = "▒▒";
  g_config.visual.ui.icons.misc.history[2] = "▓▓";
  g_config.visual.ui.icons.misc.history[3] = "██";

  g_config.pomodoro.amount = 4;
  g_config.pomodoro.work_time = 25;
  g_config.pomodoro.short_pause = 5;
  g_config.pomodoro.long_pause = 30;

  g_config.notifications.enabled = 1;
  g_config.notifications.sound = 1;
  g_config.notifications.sound_volume = 0.5f;

  g_config.noise.enabled = 1;
  g_config.noise.master_volume = 50;

  /* Resolve XDG-compliant paths for log files and socket */
  {
    const char* state_home = getenv("XDG_STATE_HOME");
    const char* home = getenv("HOME");
    char state_dir[4096];
    if (state_home && state_home[0] == '/')
      snprintf(state_dir, sizeof(state_dir), "%s/tomato", state_home);
    else if (home)
      snprintf(state_dir, sizeof(state_dir), "%s/.local/state/tomato", home);
    else
      snprintf(state_dir, sizeof(state_dir), "/tmp/tomato");

    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    char socket_dir[4096];
    if (runtime_dir && runtime_dir[0] == '/')
      snprintf(socket_dir, sizeof(socket_dir), "%s", runtime_dir);
    else
      snprintf(socket_dir, sizeof(socket_dir), "%s", state_dir);

    /* Create both directories (mkdir -p) */
    EnsureDir(state_dir);
    EnsureDir(socket_dir);

    char buf[8192];
    snprintf(buf, sizeof(buf), "%s/pomodoro.bin", state_dir);
    g_config.logging.pomodoro_log = strdup(buf);
    snprintf(buf, sizeof(buf), "%s/notes.log", state_dir);
    g_config.logging.notes_log = strdup(buf);
    snprintf(buf, sizeof(buf), "%s/errors.log", state_dir);
    g_config.logging.error_log = strdup(buf);
    snprintf(buf, sizeof(buf), "%s/tomato_timer.sock", socket_dir);
    g_config.logging.timer_file = strdup(buf);
  }
  g_config.logging.timer_log = 1;
  g_config.logging.work_log = 1;
  g_config.logging.notepad_log = 1;
  g_config.logging.timerlog_icons = 1;

  g_config.autostart.work = 1;
  g_config.autostart.pause = 1;

  g_config.visual.clock_24h = 1;
  g_config.visual.icons_index = 0;

  g_config.misc.wsl = 0;
  g_config.misc.fps = 120;
  g_config.misc.max_note_depth = 1;

  g_config.key_bindings = (KeyFunction*)DEFAULT_KEYS;
  g_config.num_keys = DEFAULT_KEYS_COUNT;
}

/**
 * Sync g_config.visual.icons from icons_index (0=nerd-icons, 1=emojis, 2=ascii).
 * Called after any preference dialog commit that changes icons_index.
 */
void SyncIconsFromIndex(void) {
  switch (g_config.visual.icons_index) {
    case 0:
      g_config.visual.icons = "nerd-icons";
      break;
    case 1:
      g_config.visual.icons = "emojis";
      break;
    case 2:
      g_config.visual.icons = "ascii";
      break;
    default:
      g_config.visual.icons = "nerd-icons";
      break;
  }
}

/**
 * Navigate a dotted path (e.g. "visual.ui.icons.noise") through a TOML table.
 * @param root top-level toml_table_t
 * @param dotted dot-separated path string
 * @return sub-table, or NULL if any segment is missing
 */
static toml_table_t* tableAtPath(toml_table_t* root, const char* dotted) {
  toml_table_t* tbl = root;
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", dotted);
  char* save;
  char* part = strtok_r(buf, ".", &save);
  while (part && tbl) {
    tbl = toml_table_table(tbl, part);
    part = strtok_r(NULL, ".", &save);
  }
  return tbl;
}

/**
 * Read an integer value from a dotted TOML path into *out.
 * Silently no-ops if the key is absent or the table segment is missing.
 * @param root Top-level TOML table
 * @param path Dot-separated path to the integer key
 * @param out Output pointer for the read value
 */
static void readInt(toml_table_t* root, const char* path, int* out) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", path);
  char* last_dot = strrchr(buf, '.');
  toml_table_t* tbl;
  const char* key;
  if (last_dot) {
    *last_dot = '\0';
    tbl = tableAtPath(root, buf);
    key = last_dot + 1;
  } else {
    tbl = root;
    key = path;
  }
  if (!tbl) return;
  toml_value_t v = toml_table_int(tbl, key);
  if (v.ok) *out = (int)v.u.i;
}

/**
 * Read a boolean value from a dotted TOML path into *out (0/1).
 * Silently no-ops if the key is absent or the table segment is missing.
 * @param root Top-level TOML table
 * @param path Dot-separated path to the boolean key
 * @param out Output pointer for the read value (0 or 1)
 */
static void readBool(toml_table_t* root, const char* path, int* out) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", path);
  char* last_dot = strrchr(buf, '.');
  toml_table_t* tbl;
  const char* key;
  if (last_dot) {
    *last_dot = '\0';
    tbl = tableAtPath(root, buf);
    key = last_dot + 1;
  } else {
    tbl = root;
    key = path;
  }
  if (!tbl) return;
  toml_value_t v = toml_table_bool(tbl, key);
  if (v.ok) *out = v.u.b ? 1 : 0;
}

/**
 * Read a floating-point value from a dotted TOML path into *out.
 * Silently no-ops if the key is absent or the table segment is missing.
 * @param root Top-level TOML table
 * @param path Dot-separated path to the float key
 * @param out Output pointer for the read value
 */
static void readFloat(toml_table_t* root, const char* path, float* out) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", path);
  char* last_dot = strrchr(buf, '.');
  toml_table_t* tbl;
  const char* key;
  if (last_dot) {
    *last_dot = '\0';
    tbl = tableAtPath(root, buf);
    key = last_dot + 1;
  } else {
    tbl = root;
    key = path;
  }
  if (!tbl) return;
  toml_value_t v = toml_table_double(tbl, key);
  if (v.ok) *out = (float)v.u.d;
}

/**
 * Read a string value from a dotted TOML path into *out.
 * Silently no-ops if the key is absent or the table segment is missing.
 * @param root Top-level TOML table
 * @param path Dot-separated path to the string key
 * @param out Output pointer for the string (caller does not own memory)
 */
static void readString(toml_table_t* root, const char* path, const char** out) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", path);
  char* last_dot = strrchr(buf, '.');
  toml_table_t* tbl;
  const char* key;
  if (last_dot) {
    *last_dot = '\0';
    tbl = tableAtPath(root, buf);
    key = last_dot + 1;
  } else {
    tbl = root;
    key = path;
  }
  if (!tbl) return;
  toml_value_t v = toml_table_string(tbl, key);
  if (v.ok) *out = v.u.s;
}

/**
 * Read a three-element string array from a dotted TOML path into out[].
 * The array must be at least 3 elements long (nerd, emoji, ascii).
 * Silently no-ops if the key is absent or the table segment is missing.
 * @param root Top-level TOML table
 * @param path Dot-separated path or bare key to the array
 * @param out Output array of 3 string pointers
 */
static void readIconArray(toml_table_t* root, const char* path,
                          const char* out[3]) {
  toml_array_t* arr = toml_table_array(root, path);
  if (!arr) {
    /* Try as nested table path */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", path);
    char* last_dot = strrchr(buf, '.');
    if (last_dot) {
      *last_dot = '\0';
      toml_table_t* parent = tableAtPath(root, buf);
      if (parent) arr = toml_table_array(parent, last_dot + 1);
    }
  }
  if (!arr || toml_array_len(arr) < 3) return;
  int len = toml_array_len(arr);
  if (len > 3) len = 3;
  for (int i = 0; i < len; i++) {
    toml_value_t v = toml_array_string(arr, i);
    if (v.ok) out[i] = v.u.s;
  }
}

/**
 * Parse the [keybindings] TOML section and replace g_config.key_bindings.
 *
 * The expected structure is:
 *   [keybindings.MODE.SCENE]
 *   key_name = "ActionName"
 *
 * If any bindings are found, the old array is freed and g_config.key_bindings
 * is replaced with a dynamically-allocated copy of the TOML entries.
 * @param root Top-level TOML table
 */
static void readKeybindings(toml_table_t* root) {
  toml_table_t* kb = toml_table_table(root, "keybindings");
  if (!kb) return;

  /* First pass: count total entries */
  int total = 0;
  for (int m = 0; m < toml_table_len(kb); m++) {
    int klen;
    const char* mode_name = toml_table_key(kb, m, &klen);
    if (!mode_name) continue;
    toml_table_t* mode_tbl = toml_table_table(kb, mode_name);
    if (!mode_tbl) continue;
    for (int s = 0; s < toml_table_len(mode_tbl); s++) {
      int klen2;
      const char* sc_name = toml_table_key(mode_tbl, s, &klen2);
      if (!sc_name) continue;
      toml_table_t* sc_tbl = toml_table_table(mode_tbl, sc_name);
      if (!sc_tbl) continue;
      total += toml_table_len(sc_tbl);
    }
  }
  if (total == 0) return;

  /* Allocate new array */
  KeyFunction* custom =
    (KeyFunction*)CALLOC((size_t)total, sizeof(KeyFunction));
  if (!custom) return;
  int idx = 0;

  for (int m = 0; m < toml_table_len(kb); m++) {
    int klen;
    const char* mode_name = toml_table_key(kb, m, &klen);
    if (!mode_name) continue;
    toml_table_t* mode_tbl = toml_table_table(kb, mode_name);
    if (!mode_tbl) continue;
    int mode = modeFromString(mode_name);
    if (!mode) continue;

    for (int s = 0; s < toml_table_len(mode_tbl); s++) {
      int klen2;
      const char* sc_name = toml_table_key(mode_tbl, s, &klen2);
      if (!sc_name) continue;
      toml_table_t* sc_tbl = toml_table_table(mode_tbl, sc_name);
      if (!sc_tbl) continue;
      int scene = sceneFromString(sc_name);
      if (!scene) continue;

      for (int k = 0; k < toml_table_len(sc_tbl); k++) {
        int klen3;
        const char* key_name = toml_table_key(sc_tbl, k, &klen3);
        if (!key_name) continue;
        toml_value_t v = toml_table_string(sc_tbl, key_name);
        if (!v.ok) continue;

        custom[idx].key = keycodeFromString(key_name);
        custom[idx].action = actionFromString(v.u.s);
        custom[idx].modes = mode;
        custom[idx].scene_types = scene;
        idx++;
      }
    }
  }

  if (idx > 0) {
    if (g_config.key_bindings != (KeyFunction*)keys)
      free(g_config.key_bindings);
    g_config.key_bindings = custom;
    g_config.num_keys = (size_t)idx;
  } else {
    free(custom);
  }
}

/**
 * Parse a single TOML file and overlay its values onto g_config.
 * @param path Absolute path to the .toml file
 */
static void loadTomlFile(const char* path) {
  FILE* f = fopen(path, "r");
  if (!f) return;

  char errbuf[200];
  toml_table_t* root = toml_parse_file(f, errbuf, sizeof(errbuf));
  fclose(f);
  if (!root) return;

  readInt(root, "visual.animations", &g_config.visual.animations);
  readString(root, "visual.icons", &g_config.visual.icons);
  /* Sync icons_index from the string after TOML override */
  if (strcmp(g_config.visual.icons, "nerd-icons") == 0)
    g_config.visual.icons_index = 0;
  else if (strcmp(g_config.visual.icons, "emojis") == 0)
    g_config.visual.icons_index = 1;
  else
    g_config.visual.icons_index = 2;
  readInt(root, "visual.bg_transparency", &g_config.visual.bg_transparency);
  readInt(root, "visual.status_bar_spacing",
          &g_config.visual.status_bar_spacing);
  readInt(root, "visual.status_bar_position",
          &g_config.visual.status_bar_position);
  readInt(root, "visual.unfocused_panel_color",
          &g_config.visual.unfocused_panel_color);
  readInt(root, "visual.focused_panel_color",
          &g_config.visual.focused_panel_color);

  readIconArray(root, "visual.ui.icons.noise.rain",
                g_config.visual.ui.icons.noise.rain);
  readIconArray(root, "visual.ui.icons.noise.fire",
                g_config.visual.ui.icons.noise.fire);
  readIconArray(root, "visual.ui.icons.noise.wind",
                g_config.visual.ui.icons.noise.wind);
  readIconArray(root, "visual.ui.icons.noise.thunder",
                g_config.visual.ui.icons.noise.thunder);
  readIconArray(root, "visual.ui.icons.pomodoro.main_menu",
                g_config.visual.ui.icons.pomodoro.main_menu);
  readIconArray(root, "visual.ui.icons.pomodoro.work",
                g_config.visual.ui.icons.pomodoro.work);
  readIconArray(root, "visual.ui.icons.pomodoro.short_pause",
                g_config.visual.ui.icons.pomodoro.short_pause);
  readIconArray(root, "visual.ui.icons.pomodoro.long_pause",
                g_config.visual.ui.icons.pomodoro.long_pause);
  readIconArray(root, "visual.ui.icons.pomodoro.notes",
                g_config.visual.ui.icons.pomodoro.notes);
  readIconArray(root, "visual.ui.icons.pomodoro.help",
                g_config.visual.ui.icons.pomodoro.help);
  readIconArray(root, "visual.ui.icons.pomodoro.continue_icon",
                g_config.visual.ui.icons.pomodoro.continue_icon);
  readIconArray(root, "visual.ui.icons.pomodoro.idle",
                g_config.visual.ui.icons.pomodoro.idle);
  readIconArray(root, "visual.ui.icons.input.default_mode",
                g_config.visual.ui.icons.input.default_mode);
  readIconArray(root, "visual.ui.icons.input.normal_mode",
                g_config.visual.ui.icons.input.normal_mode);
  readIconArray(root, "visual.ui.icons.input.insert_mode",
                g_config.visual.ui.icons.input.insert_mode);
  readIconArray(root, "visual.ui.icons.input.visual_mode",
                g_config.visual.ui.icons.input.visual_mode);
  readIconArray(root, "visual.ui.icons.input.real_time_module",
                g_config.visual.ui.icons.input.real_time_module);
  readIconArray(root, "visual.ui.icons.input.line_column_module",
                g_config.visual.ui.icons.input.line_column_module);
  readIconArray(root, "visual.ui.icons.misc.playing",
                g_config.visual.ui.icons.misc.playing);
  readIconArray(root, "visual.ui.icons.misc.plus_volume",
                g_config.visual.ui.icons.misc.plus_volume);
  readIconArray(root, "visual.ui.icons.misc.minus_volume",
                g_config.visual.ui.icons.misc.minus_volume);
  readIconArray(root, "visual.ui.icons.misc.active_volume_bar",
                g_config.visual.ui.icons.misc.active_volume_bar);
  readIconArray(root, "visual.ui.icons.misc.inactive_volume_bar",
                g_config.visual.ui.icons.misc.inactive_volume_bar);
  readIconArray(root, "visual.ui.icons.misc.active_progress_bar",
                g_config.visual.ui.icons.misc.active_progress_bar);
  readIconArray(root, "visual.ui.icons.misc.inactive_progress_bar",
                g_config.visual.ui.icons.misc.inactive_progress_bar);
  readIconArray(root, "visual.ui.icons.misc.pause",
                g_config.visual.ui.icons.misc.pause);
  readIconArray(root, "visual.ui.icons.misc.play",
                g_config.visual.ui.icons.misc.play);
  readIconArray(root, "visual.ui.icons.misc.skip",
                g_config.visual.ui.icons.misc.skip);

  readString(root, "visual.ui.icons.misc.visual_cursor",
             &g_config.visual.ui.icons.misc.visual_cursor);
  readString(root, "visual.ui.icons.misc.insert_cursor",
             &g_config.visual.ui.icons.misc.insert_cursor);
  readIconArray(root, "visual.ui.icons.misc.border_chars",
                g_config.visual.ui.icons.misc.border_chars);
  readIconArray(root, "visual.ui.icons.misc.history",
                g_config.visual.ui.icons.misc.history);

  readInt(root, "pomodoro.amount", &g_config.pomodoro.amount);
  readInt(root, "pomodoro.work_time", &g_config.pomodoro.work_time);
  readInt(root, "pomodoro.short_pause", &g_config.pomodoro.short_pause);
  readInt(root, "pomodoro.long_pause", &g_config.pomodoro.long_pause);

  readBool(root, "notifications.enabled", &g_config.notifications.enabled);
  readBool(root, "notifications.sound", &g_config.notifications.sound);
  readFloat(root, "notifications.sound_volume",
            &g_config.notifications.sound_volume);

  readBool(root, "noise.enabled", &g_config.noise.enabled);
  readInt(root, "noise.master_volume", &g_config.noise.master_volume);

  readString(root, "logging.pomodoro_log", &g_config.logging.pomodoro_log);
  if (g_config.logging.pomodoro_log)
    g_config.logging.pomodoro_log = strdup(g_config.logging.pomodoro_log);
  readString(root, "logging.notes_log", &g_config.logging.notes_log);
  if (g_config.logging.notes_log)
    g_config.logging.notes_log = strdup(g_config.logging.notes_log);
  readString(root, "logging.error_log", &g_config.logging.error_log);
  if (g_config.logging.error_log)
    g_config.logging.error_log = strdup(g_config.logging.error_log);
  readBool(root, "logging.timer_log", &g_config.logging.timer_log);
  readString(root, "logging.timer_file", &g_config.logging.timer_file);
  if (g_config.logging.timer_file)
    g_config.logging.timer_file = strdup(g_config.logging.timer_file);
  readBool(root, "logging.work_log", &g_config.logging.work_log);
  readBool(root, "logging.notepad_log", &g_config.logging.notepad_log);
  readBool(root, "logging.timerlog_icons", &g_config.logging.timerlog_icons);

  readBool(root, "autostart.work", &g_config.autostart.work);
  readBool(root, "autostart.pause", &g_config.autostart.pause);

  readBool(root, "misc.wsl", &g_config.misc.wsl);
  readInt(root, "misc.fps", &g_config.misc.fps);
  readInt(root, "misc.max_note_depth", &g_config.misc.max_note_depth);

  readKeybindings(root);

  toml_free(root);
}

/**
 * Load configuration from TOML files.
 *
 * Order of precedence (later overrides earlier):
 *   1. Built-in defaults (setDefaults)
 *   2. System-wide /etc/tomato/config.toml
 *   3. User $XDG_CONFIG_HOME/tomato/config.toml or
 *   $HOME/.config/tomato/config.toml
 *
 * This function is idempotent — it only runs once (guarded by a static flag).
 */
void LoadConfig(void) {
  static bool loaded = false;
  if (loaded) return;
  loaded = true;

  setDefaults();

  loadTomlFile(SYSTEM_CONFIG_PATH);

  const char* xdg = getenv("XDG_CONFIG_HOME");
  const char* home = getenv("HOME");
  char path[1024];
  if (xdg && xdg[0]) {
    snprintf(path, sizeof(path), "%s/tomato/config.toml", xdg);
  } else if (home) {
    snprintf(path, sizeof(path), "%s/%s/tomato/config.toml", home,
             DEFAULT_USER_CONFIG_DIR);
  } else {
    return;
  }
  loadTomlFile(path);
}
