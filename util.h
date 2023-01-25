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
#include <ncurses.h>

/* Defining the colors pallete size */
#define PALLETE_SIZE (COLOR_WHITE - COLOR_BLACK + 1)

/* Defining the app struct */
typedef struct appData appData;
struct appData{
    char *logPrefix;
    char *logFile;
    char *tmpFile;
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
    int framems;
    int timerms;
    int logoFrame;
    int coffeeFrame;
    int machineFrame;
    int beachFrame;
    int userInput;
    int pausedTimer;
    int cycles;
    int needToLog;
    int needResume;
    int resume;
    int newDay;
    char date[50];
    int x;
    int y;
};

/* Defining the app funtions */
extern void initScreen();
extern void setColor(short int , short int , chtype );
extern void getWindowSize(appData * );

extern void createLog(appData * );
extern void readLog(appData * );
extern void setLogVars(appData * );
extern void deleteLastLog(appData * );
extern void printLog(appData * );
extern void printResume(appData * );

extern void timer(appData * );

extern void printMainMenu(appData * , const char * );
extern void printPomodoroCounter(appData * );
extern void printPauseIndicator(appData * , const char * );
extern void printTimer(appData * , const char * );
extern void printSettings(appData * );

