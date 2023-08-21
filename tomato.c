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
#include <unistd.h>

/* Initialize variables */
void initApp(appData * app){
    /* One Second Based in the Frames per Second */
    app->sfps = sqrt(60);

    /* Animation variables */
    app->logoFrame = 0;
    app->coffeeFrame= 0;
    app->bannerFrame= 0;
    app->frameTimer = 0;
    app->framems = 0;

    /* Noise variables */
    sprintf(app->rainVolume, "%d", RAINVOLUME);
    sprintf(app->fireVolume, "%d", FIREVOLUME);
    sprintf(app->windVolume, "%d", WINDVOLUME);
    sprintf(app->thunderVolume, "%d", THUNDERVOLUME);
    app->printVolume = 0;
    app->rainNoisePID = 0;
    app->fireNoisePID = 0;
    app->windNoisePID = 0;
    app->thunderNoisePID = 0;
    app->runRainOnce = 0;
    app->runFireOnce = 0;
    app->runWindOnce = 0;
    app->runThunderOnce = 0;
    app->playNoise = 0;
    app->playRainNoise = 0;
    app->playFireNoise = 0;
    app->playWindNoise = 0;
    app->playThunderNoise = 0;

    /* Pomodoro variables */
    app->pausedTimer = 0;
    app->longPause = (LONGPAUSE * 60 * 8);
    app->workTime = (WORKTIME * 60 * 8);
    app->shortPause = (SHORTPAUSE * 60 * 8);
    app->pomodoros = POMODOROS;
    app->pomodoroCounter = 0;
    app->cycles = 0;
    app->newDay = 1;

    /* Misc variables */
    app->currentPID = getpid();
    app->menuPos = 1;
    app->currentMode = 0;
    app->timer = 0;
    app->timerms = 0;
    app->needToLog = 0;
    app->needResume = 0;
    app->resume = 0;
    app->runOnce = 1;

    /* File variables (defined in the config.mk) */
    if(WORKLOG == 1){
        app->logPrefix = malloc(strlen(LOGPREFIX) + 1);
        strcpy(app->logPrefix, LOGPREFIX);
        app->logFile = malloc(strlen(LOGFILE) + 1);
        strcpy(app->logFile, LOGFILE);
        app->tmpFile = malloc(strlen(TMPFILE) + 1);
        strcpy(app->tmpFile, TMPFILE);
        app->timerFile = malloc(strlen(TIMERFILE) + 1);
        strcpy(app->timerFile, TIMERFILE);
        createLog(app);
        readLog(app);
    }
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
            printNoiseMenu(app);
            break;

        case 0:
            printResume(app);
            printMainMenu(app);
            printNoiseMenu(app);
            break;

        case 1:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printCoffee(app);
            printTimer(app);
            printNoiseMenu(app);
            break;

        case 2:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printMachine(app);
            printTimer(app);
            printNoiseMenu(app);
            break;

        case 3:
            printPomodoroCounter(app);
            printPauseIndicator(app);
            printBeach(app);
            printTimer(app);
            printNoiseMenu(app);
            break;
        default:
            break;
    }
    refresh();
}

/* Putting it all together */
int main(int argc, char *argv[]){
    if(argc == 2 && !strcmp("-t", argv[1]) && TIMERLOG == 1){
        char * timerFile = malloc(strlen(TIMERFILE) + 1);
        strcpy(timerFile, TIMERFILE);
        int status = tomatoTimer(timerFile);
        return status;
    }
    else if(argc == 2 && !strcmp("-t", argv[1]) && TIMERLOG != 1){
        printf("enable timer log to use [-t]");
        return 1;
    }
    else if(argc != 1){
        printf("usage: tomato [-t]");
        return 0;
    }
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

