/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// draw.h
*/
#ifndef DRAW_H_
#define DRAW_H_
#include <ncurses.h>

/* Print keybinds */
extern void printKeybinds(appData * , int );

/* Print cursor */
extern void printCursor(appData * );

/* Print part of the notes */
extern void printPartialNotes(appData * , int );

/* Print notes */
extern void printNotes(appData * );

/* Print noise menu */
extern void printNoiseMenu(appData * );

/* Print resume menu */
extern void printResume(appData * );

/* Print the pomodoro counter */
extern void printPomodoroCounter(appData * );

/* Print the pause indicator */
extern void printPauseIndicator(appData * );

/* Print the Help Indicator */
extern void printHelpIndicator(appData * );

/* Print the Notepad Indicator */
extern void printNotepadIndicator(appData * );

/* Print the Main Menu */
extern void printMainMenu(appData * );

/* Print the settings menu */
extern void printSettings(appData * );

/* Print the Timer */
extern void printTimer(appData * );

#endif
