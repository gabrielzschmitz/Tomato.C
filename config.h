/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
//  config.h
*/
#ifndef CONFIG_H_
#define CONFIG_H_

/* 1 if you're in WSL, 0 if not 
 * Note: you'll need wsl-notify-send
 * for the notifications and the 
 * notifications sounds and white noises
 * will not work */
static const int WSL = 0;

/* iconsoff - iconson - nerdicons
 * Note: you'll need a patched 
 * nerdicons for that option */
static const char * ICONS = "nerdicons";

/* 1 means you'll be asked to continue 
 * after each work cycle, 0 means not */
static const int AUTOSTARTWORK = 1;

/* 1 means you'll be asked to continue 
 * after each pause, 0 means not */
static const int AUTOSTARTPAUSE = 1;

/* 1 means notifications on, 0 off
 * Note: you'll need libnotify if you're at linux*/
static const int NOTIFY = 1;

/* 1 means notification sound on, 0 off 
 * Note: you'll need mpv */
static const int SOUND = 1;

/* 1 means noises on, 0 off
 * Note: you'll need mpv */
static const int NOISE = 1;

/* 1 means notepad enabled, 0 disabled */
static const int NOTEPAD = 1;

/* noises volume level stage from 10 to 100 (default: 50)
 * Note: you'll need mpv
 * (increment it by 10 by 10)*/
static const int RAINVOLUME = 50;
static const int FIREVOLUME = 50;
static const int WINDVOLUME = 50;
static const int THUNDERVOLUME = 50;

/* 1 if you want transparent background, 0 if not
 * Note: you'll need a terminal already transparent */
static const int BGTRANSPARENCY = 1;

/* amount of pomodoros from 1 to 8 (default: 4) */
static const int POMODOROS = 4;

/* time for a work stage from 5 to 75 (default: 25)
 * (increment it by 5 by 5)*/
static const int WORKTIME = 25;

/* time for the short pause from 1 to 10 (default: 5) */
static const int SHORTPAUSE = 5;

/* time for the long pause from 5 to 60 (default: 30)
 * (increment it by 5 by 5)*/
static const int LONGPAUSE = 30;

/* 1 means work log on, 0 off
 * Note: if you turn it off the app will not resume
 * from unfinished cycle anymore */
static const int WORKLOG = 1;

/* 1 means timer log on, 0 off
 * Note: if you turn it off "$tomato -t"
 * will not work */
static const int TIMERLOG = 1;

#endif

