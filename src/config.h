#ifndef CONFIG_H_
#define CONFIG_H_

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
/* Input Icons */
static const char* NORMAL_MODE_ICONS[3] = {"", "🧭", ""};
static const char* INSERT_MODE_ICONS[3] = {"", "✏ ", ""};
static const char* VISUAL_MODE_ICONS[3] = {"󰕢", "🔲", ""};
static const char* REAL_TIME_MODULE_ICONS[3] = {"", "🕘", ""};
static const char* VISUAL_CURSOR_ICON = "█";
static const char* INSERT_CURSOR_ICON = "▏";
/* Misc Icons */
static const char* BORDER_CHARS[6] = {"┏", "┓", "┗", "┛", "━", "┃"};
static const char* PAUSE_ICONS[3] = {"󰏤", "⏸️", "P"};

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
static const int NOTIFY = 1;
/* 1 means notification sound on, 0 off (default: 1)
 * Note: you'll need mpv */
static const int SOUND = 1;

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
/* 1 means timer log on, 0 off (default: 1)
 * Note: if you turn it off "$tomato -t" will not work */
static const int TIMER_LOG = 1;
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

#endif /* CONFIG_H_ */
