#ifndef CONFIG_H_
#define CONFIG_H_

#include <ncurses.h>

#include "input.h"

/* Key function struct */
typedef struct {
  int key;
  void (*action)(AppData* app);
  int modes;
  int scene_types;
} KeyFunction;

/* Visual Settings ---------------------------------------------------------- */
/* 1 if you want animations, 0 if not (default: 1) */
static const int ANIMATIONS = 1;
/* nerd-icons - emojis - ascii (default: nerd-icons)
 * Note: you'll need a patched nerdicons for that option */
static const char* ICONS = "nerd-icons";
/* 1 if you want transparent background, 0 if not (default: 1)
 * Note: you'll need a terminal already transparent */
static const int BG_TRANSPARENCY = 1;
/* amount of space between status bar modules (default: 1) */
static const int STATUS_BAR_SPACING = 1;
/* either 1 for top, or 0 for bottom (default: 0) */
static const int STATUS_BAR_POSITION = 0;
/* 0-7 BLACK-WHITE, color for the unfocused panels (default: 7) */
static const int UNFOCUSED_PANEL_COLOR = 7;
/* 0-7 BLACK-WHITE, color for the focused panel (default: 1) */
static const int FOCUSED_PANEL_COLOR = 1;

/* UI Settings -------------------------------------------------------------- */
/* Note: icons need to follow this pattern: nerd-icons - emojis - ascii */
/* Noise Icons */
static const char* RAIN_ICONS[3] = {"󰖖", "☔", "R"};
static const char* FIRE_ICONS[3] = {"󰈸", "🔥", "F"};
static const char* WIND_ICONS[3] = {"󰖝", "🍃", "W"};
static const char* THUNDER_ICONS[3] = {"󱐋", "⚡", "T"};
static const char* PLUS_VOLUME_ICONS[3] = {"", "➕", "+"};
static const char* MINUS_VOLUME_ICONS[3] = {"", "➖", "-"};
static const char* ACTIVE_VOLUME_BAR_ICONS[3] = {"█", "█", "█"};
static const char* INACTIVE_VOLUME_BAR_ICONS[3] = {"▒", "▒", "▒"};
/* Pomodoro Icons */
static const char* MAIN_MENU_ICONS[3] = {"󰍜", "🧾", ""};
static const char* WORK_ICONS[3] = {"", "🍅", ""};
static const char* SHORT_PAUSE_ICONS[3] = {"", "☕", ""};
static const char* LONG_PAUSE_ICONS[3] = {"", "🌴", ""};
static const char* NOTES_ICONS[3] = {"", "📝", ""};
static const char* HELP_ICONS[3] = {"", "⁉️", ""};
static const char* CONTINUE_ICONS[3] = {"", "⏯️", ""};
static const char* IDLE_ICONS[3] = {"", "🌙", ""};
/* Input Icons */
static const char* NORMAL_MODE_ICONS[3] = {"", "🧭", ""};
static const char* INSERT_MODE_ICONS[3] = {"", "✏ ", ""};
static const char* VISUAL_MODE_ICONS[3] = {"󰕢", "🔲", ""};
static const char* REAL_TIME_MODULE_ICONS[3] = {"", "🕘", ""};
static const char* LINE_COLUMN_MODULE_ICONS[3] = {"", "▶", ""};
static const char* VISUAL_CURSOR_ICON = "█";
static const char* INSERT_CURSOR_ICON = "▏";
/* Misc Icons */
static const char* BORDER_CHARS[6] = {"┏", "┓", "┗", "┛", "━", "┃"};
static const char* PAUSE_ICONS[3] = {"󰏤", "⏸️", "P"};
static const char* SKIP_ICONS[3] = {"󰒬", "⏭️", "S"};
static const char* HISTORY_ICONS[6] = {"▢", "◫", "☒", "▤", "▦", "■"};

/* Pomodoro Settings -------------------------------------------------------- */
/* amount of pomodoros from 1 to 8 (default: 4) */
static const int POMODOROS_AMOUNT = 4;
/* time for a work stage from 5 to 75 (default: 25) (increment it by 5 by 5) */
static const int WORKTIME_TIME = 25;
/* time for short pause from 1 to 10 (default: 5) */
static const int SHORT_PAUSE_TIME = 5;
/* time for long pause from 5 to 60 (default: 30) (increment it by 5 by 5) */
static const int LONG_PAUSE_TIME = 30;

/* Notification Settings ---------------------------------------------------- */
/* 1 means notifications on, 0 off (default: 1)
 * Note: you'll need libnotify if you're at linux */
static const int NOTIFICATIONS = 1;
/* 1 means notification sound on, 0 off (default: 1)
 * Note: you'll need mpv */
static const int NOTIFICATIONS_SOUND = 1;
/* volume for the notification sounds in percentage from 0.0 to 1.0 */
static const float NOTIFICATIONS_SOUND_VOLUME = 0.5;

/* Noise Settings ----------------------------------------------------------- */
/* 1 means noises on, 0 off (default: 1)
 * Note: you'll need mpv */
static const int NOISE = 1;
/* noises volume level stage from 10 to 100 (default: 50)
 * Note: you'll need mpv (increment it by 10 by 10)*/
static const int RAIN_VOLUME = 50;
static const int FIRE_VOLUME = 50;
static const int WIND_VOLUME = 50;
static const int THUNDER_VOLUME = 50;

/* Logging Settings --------------------------------------------------------- */
/* the file path for the errors log (default: /tmp/tomato_errors.log) */
static const char* ERROR_LOG = "/tmp/tomato_errors.log";
/* 1 means timer log on, 0 off (default: 1)
 * Note: if you turn it off "$tomato -t" will not work */
static const int TIMER_LOG = 1;
/* the file path for the timer log (default: /tmp/tomato_timer.sock) */
static const char* TIMER_FILE = "/tmp/tomato_timer.sock";
/* 1 means work log on, 0 off (default: 1)
 * Note: if you turn it off the app will not resume from unfinished cycle
 * anymore */
static const int WORK_LOG = 1;
/* 1 means notepad log on, 0 off (default: 1)
 * Note: if you turn it off notepad will not be saved when you exit */
static const int NOTEPAD_LOG = 1;
/* 1 means icons ontimer log on, 0 off (default: 1) */
static const int TIMERLOG_ICONS = 1;

/* Auto-Start Settings ------------------------------------------------------ */
/* 1 means you'll be asked to continue after each work cycle, 0 means not
 * (default: 1) */
static const int AUTOSTART_WORK = 1;
/* 1 means you'll be asked to continue after each pause, 0 means not
 * (default: 1) */
static const int AUTOSTART_PAUSE = 1;

/* Misc Settings ------------------------------------------------------------ */
/* 1 if you're in WSL, 0 if not (default: 0)
 * Note: you'll need wsl-notify-send for the notifications. The notifications
 * sounds and white noises will not work */
static const int WSL = 0;
/* string used in the sprites to separate the types of sprites
 * Note: just tweak if strictly necessary */
static const char* SEPARATOR =
  "---------------------------------------------------------------------------";
/* sets the fps app runs (default: 120) */
static const int FPS = 120;

/* Scene types bitmask for key binding filters */
#define POMODORO_SCENES \
  (SCENE_WORK_TIME | SCENE_SHORT_PAUSE | SCENE_LONG_PAUSE | SCENE_MAIN_MENU)
#define ALL_SCENES (POMODORO_SCENES | SCENE_NOTES | SCENE_HELP | SCENE_CONTINUE)

/* Struct to map a key to a function */
static const KeyFunction keys[] = {
  /* NORMAL mode - editing keys (when input_len > 0) */
  {'h', InputCursorLeft, NORMAL_WITH_INPUT, SCENE_NOTES},
  {'l', InputCursorRight, NORMAL_WITH_INPUT, SCENE_NOTES},
  {KEY_LEFT, InputCursorLeft, NORMAL_WITH_INPUT, SCENE_NOTES},
  {KEY_RIGHT, InputCursorRight, NORMAL_WITH_INPUT, SCENE_NOTES},
  {'x', InputDeleteChar, NORMAL_WITH_INPUT, SCENE_NOTES},
  {'i', SwitchToInsertMode, NORMAL_WITH_INPUT, SCENE_NOTES},
  {'v', SwitchToVisualMode, NORMAL_WITH_INPUT, SCENE_NOTES},
  {ESC, InputESC, NORMAL_WITH_INPUT, SCENE_NOTES},
  {ENTER, InputCommit, NORMAL_WITH_INPUT, SCENE_NOTES},
  {'\r', InputCommit, NORMAL_WITH_INPUT, SCENE_NOTES},
  {KEY_ENTER, InputCommit, NORMAL_WITH_INPUT, SCENE_NOTES},

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
  {KEY_LEFT, InputCursorLeft, VISUAL, SCENE_NOTES},
  {KEY_RIGHT, InputCursorRight, VISUAL, SCENE_NOTES},
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

  /* NORMAL mode - navigation keys (when input_len == 0) */
  {'j', NoteDownApp, NORMAL, SCENE_NOTES},
  {'k', NoteUpApp, NORMAL, SCENE_NOTES},
  {KEY_DOWN, NoteDownApp, NORMAL, SCENE_NOTES},
  {KEY_UP, NoteUpApp, NORMAL, SCENE_NOTES},
  {ENTER, ToggleTaskAtNotes, NORMAL, SCENE_NOTES},
  {'d', DeleteNoteAtNotes, NORMAL, SCENE_NOTES},
  {'a', AddNewNote, NORMAL, SCENE_NOTES},
  {'A', AddNewNoteItem, NORMAL, SCENE_NOTES},

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

#endif /* CONFIG_H_ */
