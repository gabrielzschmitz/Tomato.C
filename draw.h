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

/* Print noise menu */
void printNoiseMenu(appData * );

/* Print resume menu */
extern void printResume(appData * );

/* Print the pomodoro counter */
extern void printPomodoroCounter(appData * );

/* Print the pause indicator */
extern void printPauseIndicator(appData * );

/* Print the Main Menu */
extern void printMainMenu(appData * );

/* Print the settings menu */
extern void printSettings(appData * );

/* Print the Timer */
extern void printTimer(appData * );

#endif
