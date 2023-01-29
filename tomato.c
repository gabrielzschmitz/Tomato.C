/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
//  tomato.c
*/
#include "tomato.h"
#include "anim.h"
#include "draw.h"
#include "input.h"
#include "notify.h"
#include "update.h"
#include "util.h"
#include "config.h"
#include <ncurses.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

/* Initialize variables */
void initApp(appData * app){
    /* One Second Based in the Frames per Second */
    app->sfps = (2 * sqrt(15));

    app->longPause = (LONGPAUSE * 60 * 8);
    app->workTime = (WORKTIME * 60 * 8);
    app->shortPause = (SHORTPAUSE * 60 * 8);
    app->pomodoros = POMODOROS;
    app->menuPos = 1;
    app->pomodoroCounter = 0;
    app->currentMode = 0;
    app->logoFrame = 0;
    app->coffeeFrame= 0;
    app->bannerFrame= 0;
    app->frameTimer = 0;
    app->timer = 0;
    app->framems = 0;
    app->timerms = 0;
    app->pausedTimer = 0;
    app->cycles = 0;
    app->needToLog = 0;
    app->needResume = 0;
    app->resume = 0;
    app->newDay = 1;
    app->runOnce = 1; 

    /* Defined in the config.mk */
    app->logPrefix = LOGPREFIX;
    app->logFile = LOGFILE;
    app->tmpFile = TMPFILE;
    app->timerFile = TIMERFILE;

    createLog(app);
    readLog(app);
}

/* Update variables */
void doUpdate(appData * app){
    /* Update all the app modes */
    updateMainMenu(app);
    updateWorkTime(app);
    updateShortPause(app);
    updateLongPause(app);

    /* Get X and Y window size */
    getWindowSize(app);
}

/* Print at screen */
void drawScreen(appData * app){
    erase();
    
    switch(app->currentMode){
        case -1:
            printWrench(app, 1);
            printSettings(app);
            printWrench(app, 0);
            break;

        case 0:
            printResume(app);
            printMainMenu(app);
            break;

        case 1:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printCoffee(app);
            printTimer(app);
            break;

        case 2:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printMachine(app);
            printTimer(app);
            break;

        case 3:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printBeach(app);
            printTimer(app);
            break;
    }
    refresh();
}

/* Putting it all together */
int main(void){
    /* Enable Emojis */
    setlocale(LC_CTYPE, "");

    /* Creating the app struct */
    appData app;

    /* Initializing the app */
    initScreen();
    initApp(&app);

    /* Main app loop */
    while(1){
        handleInputs(&app);

        doUpdate(&app);

        drawScreen(&app);

        /* Setting the screen refresh rate to 60 */
        napms(1000 / 60);
    }

    /* Makes terminal stop reporting mouse movement events */
    printf("\033[?1003l\n");

    /* Endding the screen created at initScreen */
    endwin();

    return 0;
}

