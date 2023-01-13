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
#include "config.h"
#include "util.h"
#include "input.h"
#include "update.h"
#include "anim.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

/* Initialize variables */
void initApp(appData * app){
    app->longPause = LONGPAUSE;
    app->workTime = WORKTIME;
    app->shortPause = SHORTPAUSE;
    app->pomodoros = POMODOROS;
    app->menuPos = 1;
    app->pomodoroCounter = 0;
    app->currentMode = 0;
    app->logoFrame = 0;
    app->coffeeFrame= 0;
    app->frameTimer = 0;
    app->timer = 0;
    app->framems = 0;
    app->timerms = 0;
    app->pausedTimer = 0;
}

/* Print at screen */
void drawScreen(appData * app){
    erase();
    
    switch(app->currentMode){
        case -1:
            printGear(app, 1);
            printSettings(app);
            printGear(app, 0);
            break;

        case 0:
            printMainMenu(app, ICONS);
            break;

        case 1:
            printPomodoroCounter(app);
            printPauseIndicator(app, ICONS);
            printCoffee(app);
            printTimer(app, ICONS);
            break;

        case 2:
            printPomodoroCounter(app);
            printPauseIndicator(app, ICONS);
            printMachine(app, ICONS);
            printTimer(app, ICONS);
            break;

        case 3:
            printPauseIndicator(app, ICONS);
            printBeach(app);
            printTimer(app, ICONS);
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

    initScreen();
    initApp(&app);

    /* Main app loop */
    while(1){
        handleInputs(&app, NOTIFY, SOUND, ICONS, WSL);

        doUpdate(&app, NOTIFY, SOUND, ICONS, WSL);

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

