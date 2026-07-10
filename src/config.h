#ifndef CONFIG_H_
#define CONFIG_H_

#include <ncurses.h>

#include "input.h"

/* ── Config struct hierarchy ── */

/**
 * Icons for white-noise tracks.
 */
typedef struct {
  const char* rain[3];    /**< [nerd, emoji, ascii] */
  const char* fire[3];    /**< [nerd, emoji, ascii] */
  const char* wind[3];    /**< [nerd, emoji, ascii] */
  const char* thunder[3]; /**< [nerd, emoji, ascii] */
} ConfigNoiseIcons;

/**
 * Icons for pomodoro-stage screens.
 */
typedef struct {
  const char* main_menu[3];     /**< [nerd, emoji, ascii] */
  const char* work[3];          /**< [nerd, emoji, ascii] */
  const char* short_pause[3];   /**< [nerd, emoji, ascii] */
  const char* long_pause[3];    /**< [nerd, emoji, ascii] */
  const char* notes[3];         /**< [nerd, emoji, ascii] */
  const char* help[3];          /**< [nerd, emoji, ascii] */
  const char* continue_icon[3]; /**< [nerd, emoji, ascii] */
  const char* idle[3];          /**< [nerd, emoji, ascii] */
} ConfigPomodoroIcons;

/**
 * Icons for input-mode indicators in the status bar.
 */
typedef struct {
  const char* default_mode[3];       /**< [nerd, emoji, ascii] */
  const char* normal_mode[3];        /**< [nerd, emoji, ascii] */
  const char* insert_mode[3];        /**< [nerd, emoji, ascii] */
  const char* visual_mode[3];        /**< [nerd, emoji, ascii] */
  const char* real_time_module[3];   /**< [nerd, emoji, ascii] */
  const char* line_column_module[3]; /**< [nerd, emoji, ascii] */
} ConfigInputIcons;

/**
 * Miscellaneous icons (player controls, progress bars, cursors, borders, etc.).
 */
typedef struct {
  const char* playing[3];               /**< [nerd, emoji, ascii] */
  const char* plus_volume[3];           /**< [nerd, emoji, ascii] */
  const char* minus_volume[3];          /**< [nerd, emoji, ascii] */
  const char* active_volume_bar[3];     /**< [nerd, emoji, ascii] */
  const char* inactive_volume_bar[3];   /**< [nerd, emoji, ascii] */
  const char* active_progress_bar[3];   /**< [nerd, emoji, ascii] */
  const char* inactive_progress_bar[3]; /**< [nerd, emoji, ascii] */
  const char* pause[3];                 /**< [nerd, emoji, ascii] */
  const char* play[3];                  /**< [nerd, emoji, ascii] */
  const char* skip[3];                  /**< [nerd, emoji, ascii] */
  const char* visual_cursor;            /**< single string (not triplet) */
  const char* insert_cursor;            /**< single string (not triplet) */
  const char* border_chars[6];          /**< [tl, tr, bl, br, h, v] */
  const char* history[4];               /**< levels: 0=none, 1, 2, 3+ */
} ConfigMiscIcons;

/**
 * All icon sets grouped by category.
 */
typedef struct {
  ConfigNoiseIcons noise;
  ConfigPomodoroIcons pomodoro;
  ConfigInputIcons input;
  ConfigMiscIcons misc;
} ConfigUiIcons;

/**
 * Sub-table for visual.ui — wraps icon sets.
 */
typedef struct {
  ConfigUiIcons icons;
} ConfigUi;

/**
 * Visual/appearance settings.
 */
typedef struct {
  int animations;            /**< 0/1 — enable sprite animations */
  const char* icons;         /**< "nerd-icons" | "emojis" | "ascii" */
  int bg_transparency;       /**< 0/1 — transparent background */
  int status_bar_spacing;    /**< spaces between status bar modules */
  int status_bar_position;   /**< 0=bottom, 1=top */
  int unfocused_panel_color; /**< 0-7 ncurses COLOR_* */
  int focused_panel_color;   /**< 0-7 ncurses COLOR_* */
  ConfigUi ui;               /**< icon sets */
  int clock_24h;             /**< 0/1 — use 24-hour clock */
  int icons_index;           /**< 0=nerd-icons, 1=emojis, 2=ascii */
} ConfigVisual;

/**
 * Pomodoro timer durations.
 */
typedef struct {
  int amount;      /**< pomodoros per cycle (1-8) */
  int work_time;   /**< work stage minutes (5-75, step 5) */
  int short_pause; /**< short pause minutes (1-10) */
  int long_pause;  /**< long pause minutes (5-60, step 5) */
} ConfigPomodoro;

/**
 * Desktop notification settings.
 */
typedef struct {
  int enabled;        /**< 0/1 — requires libnotify */
  int sound;          /**< 0/1 — requires miniaudio (included on external) */
  float sound_volume; /**< 0.0 – 1.0 */
} ConfigNotifications;

/**
 * White-noise playback settings.
 */
typedef struct {
  int enabled;       /**< 0/1 — requires miniaudio (included on external) */
  int master_volume; /**< 0-100 */
} ConfigNoise;

/**
 * File paths and toggles for all application logs.
 */
typedef struct {
  const char* pomodoro_log; /**< path to pomodoro binary log */
  const char* notes_log;    /**< path to notes text log */
  const char* error_log;    /**< path to error log */
  int timer_log;            /**< 0/1 — enables `-t` CLI flag */
  const char* timer_file;   /**< path to timer socket file */
  int work_log;             /**< 0/1 — enables cycle resume */
  int notepad_log;          /**< 0/1 — save notes on exit */
  int timerlog_icons;       /**< 0/1 — icons in timer log */
} ConfigLogging;

/**
 * Auto-start toggles for pomodoro cycles.
 */
typedef struct {
  int work;  /**< 0/1 — ask to continue after work */
  int pause; /**< 0/1 — ask to continue after pause */
} ConfigAutostart;

/**
 * Miscellaneous platform / behavior settings.
 */
typedef struct {
  int wsl;            /**< 0/1 — WSL mode (wsl-notify-send) */
  int fps;            /**< target frame rate */
  int max_note_depth; /**< 0-3 — hierarchical sub-note depth */
} ConfigMisc;

/**
 * Top-level runtime configuration.
 *
 * Every field is overridable via TOML files at
 * /etc/tomato/config.toml and $XDG_CONFIG_HOME/tomato/config.
 */
typedef struct {
  ConfigVisual visual;
  ConfigPomodoro pomodoro;
  ConfigNotifications notifications;
  ConfigNoise noise;
  ConfigLogging logging;
  ConfigAutostart autostart;
  ConfigMisc misc;
  KeyFunction* key_bindings; /**< dynamic keybinding array */
  size_t num_keys;           /**< length of key_bindings */
} Config;

/** Global configuration singleton. */
extern Config g_config;

/** Load (or reload) configuration from TOML files and set defaults. */
void LoadConfig(void);

/** Sync g_config.visual.icons string from icons_index (0-2). */
void SyncIconsFromIndex(void);

/* ── Accessor macros ── */

/* Visual Settings ---------------------------------------------------------- */

/** 0/1 — enable sprite animations (default: 1) */
#define ANIMATIONS (g_config.visual.animations)
/** Icon set: "nerd-icons" | "emojis" | "ascii" (default: "nerd-icons") */
#define ICONS (g_config.visual.icons)
/** 0/1 — transparent background (default: 1) */
#define BG_TRANSPARENCY (g_config.visual.bg_transparency)
/** Spaces between status bar modules (default: 1) */
#define STATUS_BAR_SPACING (g_config.visual.status_bar_spacing)
/** 0=bottom, 1=top (default: 0) */
#define STATUS_BAR_POSITION (g_config.visual.status_bar_position)
/** Color of unfocused panels, 0-7 (default: 7) */
#define UNFOCUSED_PANEL_COLOR (g_config.visual.unfocused_panel_color)
/** Color of focused panel, 0-7 (default: 1) */
#define FOCUSED_PANEL_COLOR (g_config.visual.focused_panel_color)

/* UI / Icons --------------------------------------------------------------- */

/** @name Noise icons [nerd, emoji, ascii] */
/**@{*/
#define RAIN_ICONS (g_config.visual.ui.icons.noise.rain)
#define FIRE_ICONS (g_config.visual.ui.icons.noise.fire)
#define WIND_ICONS (g_config.visual.ui.icons.noise.wind)
#define THUNDER_ICONS (g_config.visual.ui.icons.noise.thunder)
/**@}*/

/** @name Media-player icons [nerd, emoji, ascii] */
/**@{*/
#define PLAYING_ICONS (g_config.visual.ui.icons.misc.playing)
#define PLUS_VOLUME_ICONS (g_config.visual.ui.icons.misc.plus_volume)
#define MINUS_VOLUME_ICONS (g_config.visual.ui.icons.misc.minus_volume)
#define ACTIVE_VOLUME_BAR_ICONS \
  (g_config.visual.ui.icons.misc.active_volume_bar)
#define INACTIVE_VOLUME_BAR_ICONS \
  (g_config.visual.ui.icons.misc.inactive_volume_bar)
#define ACTIVE_PROGRESS_BAR_ICONS \
  (g_config.visual.ui.icons.misc.active_progress_bar)
#define INACTIVE_PROGRESS_BAR_ICONS \
  (g_config.visual.ui.icons.misc.inactive_progress_bar)
/**@}*/

/** @name Pomodoro screen icons [nerd, emoji, ascii] */
/**@{*/
#define MAIN_MENU_ICONS (g_config.visual.ui.icons.pomodoro.main_menu)
#define WORK_ICONS (g_config.visual.ui.icons.pomodoro.work)
#define SHORT_PAUSE_ICONS (g_config.visual.ui.icons.pomodoro.short_pause)
#define LONG_PAUSE_ICONS (g_config.visual.ui.icons.pomodoro.long_pause)
#define NOTES_ICONS (g_config.visual.ui.icons.pomodoro.notes)
#define HELP_ICONS (g_config.visual.ui.icons.pomodoro.help)
#define CONTINUE_ICONS (g_config.visual.ui.icons.pomodoro.continue_icon)
#define IDLE_ICONS (g_config.visual.ui.icons.pomodoro.idle)
/**@}*/

/** @name Input-mode icons [nerd, emoji, ascii] */
/**@{*/
#define DEFAULT_MODE_ICONS (g_config.visual.ui.icons.input.default_mode)
#define NORMAL_MODE_ICONS (g_config.visual.ui.icons.input.normal_mode)
#define INSERT_MODE_ICONS (g_config.visual.ui.icons.input.insert_mode)
#define VISUAL_MODE_ICONS (g_config.visual.ui.icons.input.visual_mode)
#define REAL_TIME_MODULE_ICONS (g_config.visual.ui.icons.input.real_time_module)
#define LINE_COLUMN_MODULE_ICONS \
  (g_config.visual.ui.icons.input.line_column_module)
/**@}*/

/** Cursor icons (single string, not triplet). */
#define VISUAL_CURSOR_ICON (g_config.visual.ui.icons.misc.visual_cursor)
#define INSERT_CURSOR_ICON (g_config.visual.ui.icons.misc.insert_cursor)

/** @name Misc UI icons */
/**@{*/
#define BORDER_CHARS (g_config.visual.ui.icons.misc.border_chars)
#define PAUSE_ICONS (g_config.visual.ui.icons.misc.pause)
#define PLAY_ICONS (g_config.visual.ui.icons.misc.play)
#define SKIP_ICONS (g_config.visual.ui.icons.misc.skip)
#define HISTORY_ICONS (g_config.visual.ui.icons.misc.history)
/**@}*/

/* Pomodoro Settings -------------------------------------------------------- */

/** Pomodoros per cycle, 1-8 (default: 4). */
#define POMODOROS_AMOUNT (g_config.pomodoro.amount)
/** Work stage duration in minutes, 5-75 step 5 (default: 25). */
#define WORKTIME_TIME (g_config.pomodoro.work_time)
/** Short pause minutes, 1-10 (default: 5). */
#define SHORT_PAUSE_TIME (g_config.pomodoro.short_pause)
/** Long pause minutes, 5-60 step 5 (default: 30). */
#define LONG_PAUSE_TIME (g_config.pomodoro.long_pause)

/* Notification Settings ---------------------------------------------------- */

/** 0/1 — desktop notifications via libnotify (default: 1). */
#define NOTIFICATIONS (g_config.notifications.enabled)
/** 0/1 — notification sounds via miniaudio (included on external) (default: 1). */
#define NOTIFICATIONS_SOUND (g_config.notifications.sound)
/** Notification volume 0.0 – 1.0 (default: 0.5). */
#define NOTIFICATIONS_SOUND_VOLUME (g_config.notifications.sound_volume)

/* Noise / White-Noise Settings --------------------------------------------- */

/** 0/1 — white-noise playback via miniaudio (included on external) (default: 1). */
#define NOISE_ENABLED (g_config.noise.enabled)
/** Master volume 0-100 (default: 50). */
#define NOISE_MASTER_VOLUME (g_config.noise.master_volume)

/* Logging Settings --------------------------------------------------------- */

/** Path to pomodoro binary log (default: /tmp/tomato_pomodoro.bin). */
#define POMODORO_LOG (g_config.logging.pomodoro_log)
/** Path to notes log (default: /tmp/tomato_notes.log). */
#define NOTES_LOG (g_config.logging.notes_log)
/** Path to error log (default: /tmp/tomato_errors.log). */
#define ERROR_LOG (g_config.logging.error_log)
/** 0/1 — enables `-t` timer-log CLI flag (default: 1). */
#define TIMER_LOG (g_config.logging.timer_log)
/** Path to timer IPC socket (default: /tmp/tomato_timer.sock). */
#define TIMER_FILE (g_config.logging.timer_file)
/** 0/1 — enables resume from unfinished cycle (default: 1). */
#define WORK_LOG (g_config.logging.work_log)
/** 0/1 — save notepad data on exit (default: 1). */
#define NOTEPAD_LOG (g_config.logging.notepad_log)
/** 0/1 — icons in timer-log output (default: 1). */
#define TIMERLOG_ICONS (g_config.logging.timerlog_icons)

/* Auto-Start Settings ------------------------------------------------------ */

/** 0/1 — ask to auto-start next work cycle (default: 1). */
#define AUTOSTART_WORK (g_config.autostart.work)
/** 0/1 — ask to auto-start next pause (default: 1). */
#define AUTOSTART_PAUSE (g_config.autostart.pause)

/* Misc Settings ------------------------------------------------------------ */

/** System-wide config file path. */
extern const char* system_config_path;
/** Base directory name under $HOME for user config (e.g. ".config"). */
extern const char* default_user_config_dir;
/** 0/1 — WSL mode (default: 0). */
#define WSL (g_config.misc.wsl)
/** Sprite separator string. */
static const char* SEPARATOR =
  "---------------------------------------------------------------------------";
/** Target frame rate (default: 120). */
#define FPS (g_config.misc.fps)
/** Maximum hierarchical note depth, 0-3 (default: 1). */
#define MAX_NOTE_DEPTH (g_config.misc.max_note_depth)

/* Keybind Settings --------------------------------------------------------- */

/** Bitmask of all pomodoro timer scenes. */
#define POMODORO_SCENES (SCENE_WORK_TIME | SCENE_SHORT_PAUSE | SCENE_LONG_PAUSE)
/** Bitmask of every scene type. */
#define ALL_SCENES                                                \
  (SCENE_MAIN_MENU | POMODORO_SCENES | SCENE_NOTES | SCENE_NOTES_TRANSITION | SCENE_HELP | \
   SCENE_CONTINUE | SCENE_PREFERENCES)

/** Shorthand for g_config.key_bindings. */
#define keys (g_config.key_bindings)

/** Compile-time default keybinding array. Used as fallback when no TOML override is present. */
extern const KeyFunction default_keys[];
/** Number of entries in default_keys[]. */
extern const size_t default_keys_count;

#endif /* CONFIG_H_ */
