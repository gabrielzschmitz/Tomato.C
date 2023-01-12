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

/* 1 if you're in WSL, 0 if not 
 * Note: you'll need wsl-notify-send
 * for the notifications and the 
 * notifications sounds will not work */
static const int WSL = 0;

/* iconsoff - iconson - nerdicons
 * Note: you'll need a patched 
 * nerdicons for that option */
static const char * ICONS = "nerdicons";

/* 1 means notifications on, 0 off
 * Note: you'll need dunst */
static const int NOTIFY = 1;

/* 1 means notification sound on, 0 off 
 * Note: you'll need mpv */
static const int SOUND = 1;

/* amount of pomodoros from 1 to 8 (default: 4) */
static const int POMODOROS = 4;

/* time for a work stage from 5 to 50 (default: 25)
 * (increment it by 5 by 5)*/
static const int WORKTIME = 5;

/* time for the short pause from 1 to 10 (default: 5) */
static const int SHORTPAUSE = 25;

/* time for the long pause from 5 to 60 (default: 30)
 * (increment it by 5 by 5)*/
static const int LONGPAUSE = 25;

