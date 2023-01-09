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
    app->longPause = 30;
    app->workTime = 25;
    app->shortPause = 5;
    app->pomodoros = 4;
    app->menuPos = 1;
    app->currentMode = 0;
    app->logoFrame = 0;
    app->coffeeFrame= 0;
    app->frameTimer = 0;
    app->timer = 0;
    app->pomodoroCounter = 0;
    app->longPauseLevels = 5;
    app->workTimeLevels = 4;
    app->shortPauseLevels = 4;
    app->E = '0';
}

/* Print at screen */
void drawScreen(appData * app){
    clear();
    
    switch(app->currentMode){
        case -1:
            printGear(app, 1);
            printSettings(app);
            printGear(app, 0);
            break;

        case 0:
            printMainMenu(app);
            break;

        case 1:
            printPomodoroCounter(app);
            printCoffee(app);
            printTimer(app);
            break;

        case 2:
            printPomodoroCounter(app);
            printMachine(app);
            printTimer(app);
            break;

        case 3:
            printBeach(app);
            printTimer(app);
            break;
    }

    refresh();
}

/* Putting it all together */
int main(int argc, char *argv[]){
    /* Enable Emojis */
    setlocale(LC_CTYPE, "");

    /* Creating the app struct */
    appData app;

    initScreen();
    initApp(&app);

    /* Main app loop */
    while(1){
        handleInputs(&app);

        doUpdate(&app);

        drawScreen(&app);

        /* Setting the screen refresh rate to 60 */
        napms(60);
    }

    /* Endding the screen created at initScreen */
    endwin();

    return 0;
}

