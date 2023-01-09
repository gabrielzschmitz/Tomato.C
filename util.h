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
    int logoFrame;
    int coffeeFrame;
    int machineFrame;
    int beachFrame;
    int userInput;
    int x;
    int y;
    char E;
};

/* Defining the app funtions */
extern void initScreen();
extern void setColor(short int , short int , chtype );
extern void getWindowSize(appData * );

extern void frameTimer(appData * );
extern void timer(appData * );

extern void printLogo(appData * );
extern void printCoffee(appData * );
extern void printMachine(appData * );
extern void printBeach(appData * );
extern void printGear(appData * , int );

extern void printMainMenu(appData *);
extern void printPomodoroCounter(appData * );
extern void printTimer(appData *);
extern void printSettings(appData *);

