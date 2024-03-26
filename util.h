/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// util.h
*/
#ifndef UTIL_H_
#define UTIL_H_
#include <ncurses.h>

#include "tomato.h"

/* Defining the colors pallete size */
#define PALLETE_SIZE (COLOR_WHITE - COLOR_BLACK + 1)

/* Function to create a new note */
extern note* createNote(char);
/* Function to free allocated memory for notepad */
extern void deallocateNotepad(appData *app);
/* Function to free allocated memory for file names */
extern void deallocateFileNames(appData *app);

/* Initialize screen with some little configs */
extern void initScreen(void);
/* Set text foreground and background colors */
extern void setColor(short int, short int, chtype);
/* Get the window size */
extern void getWindowSize(appData*);

/* Log funtions */
extern void createLog(appData*);
extern void readLog(appData*);
extern void readNotepad(appData*);
extern void setLogVars(appData*);
extern void deleteLastLog(appData*);
extern void writeToLog(appData*);
extern void writeToNotepad(appData*);
extern void endTimerLog(appData*);

/* Time the pomodoros */
extern void timer(appData*);

extern char* initTimerPath(const char*);
extern int printTimerLog(const char*);
extern int tomatoTimer(const char*);

/* Noise funtions */
extern void toggleNoise(appData*, int);
extern void killNoise(void);
extern void controlVolumeNoise(appData*, int, char);

#endif
