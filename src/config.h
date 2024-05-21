#ifndef CONFIG_H_
#define CONFIG_H_

/* Visual Settings ---------------------------------------------------------- */
/* ascii - emojis - nerd-icons (default: nerd-icons)
 * Note: you'll need a patched nerdicons for that option */
static const char* ICONS = "nerd-icons";
/* 1 if you want transparent background, 0 if not (default: 1)
 * Note: you'll need a terminal already transparent */
static const int BGTRANSPARENCY = 1;

/* Pomodoro Settings -------------------------------------------------------- */
/* amount of pomodoros from 1 to 8 (default: 4) */
static const int POMODOROS = 4;
/* time for a work stage from 5 to 75 (default: 25) (increment it by 5 by 5) */
static const int WORKTIME = 25;
/* time for short pause from 1 to 10 (default: 5) */
static const int SHORTPAUSE = 5;
/* time for long pause from 5 to 60 (default: 30) (increment it by 5 by 5) */
static const int LONGPAUSE = 30;

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
static const int RAINVOLUME = 50;
static const int FIREVOLUME = 50;
static const int WINDVOLUME = 50;
static const int THUNDERVOLUME = 50;

/* Logging Settings --------------------------------------------------------- */
/* 1 means timer log on, 0 off (default: 1)
 * Note: if you turn it off "$tomato -t" will not work */
static const int TIMERLOG = 1;
/* 1 means work log on, 0 off (default: 1)
 * Note: if you turn it off the app will not resume from unfinished cycle
 * anymore */
static const int WORKLOG = 1;
/* 1 means notepad log on, 0 off (default: 1)
 * Note: if you turn it off notepad will not be saved when you exit */
static const int NOTEPADLOG = 1;
/* 1 means icons ontimer log on, 0 off (default: 1) */
static const int TIMERLOGICONS = 1;

/* Auto-Start Settings ------------------------------------------------------ */
/* 1 means you'll be asked to continue after each work cycle, 0 means not
 * (default: 1) */
static const int AUTOSTARTWORK = 1;
/* 1 means you'll be asked to continue after each pause, 0 means not
 * (default: 1) */
static const int AUTOSTARTPAUSE = 1;

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
