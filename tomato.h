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
#ifndef TOMATO_H_
#define TOMATO_H_
#include <ncurses.h>

/* Defining the app struct */
typedef struct appData appData;
struct appData{
    char * logPrefix;
    char * logFile;
    char * tmpFile;
    char * timerFile;
    int pomodorosLevels;
    int workTimeLevels;
    int shortPauseLevels;
    int longPauseLevels;
    int pomodoros;
    int workTime;
    int shortPause;
    int longPause;
    int menuPos;
    int pomodoroCounter;
    int currentMode;
    int needMainMenu;
    int frameTimer;
    int timer;
    double sfps;
    int framems;
    int timerms;
    int logoFrame;
    int coffeeFrame;
    int machineFrame;
    int beachFrame;
    int bannerFrame;
    int userInput;
    int pausedTimer;
    int cycles;
    int needToLog;
    int needResume;
    int resume;
    int newDay;
    int runOnce; 
    char date[50];
    int middlex;
    int middley;
    int x;
    int y;
};

/* Initialize variables */
extern void initApp(appData * );

/* Update variables */
extern void doUpdate(appData * );

/* Print at screen */
extern void drawScreen(appData * );

/* Putting it all together */
extern int main(int , char *[]);

#endif

