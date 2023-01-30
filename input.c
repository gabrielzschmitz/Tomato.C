/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// input.c
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>

/* Handle user input and app state */
void handleInputs(appData * app){
    MEVENT event;

    app->userInput = getch();
    char key;
    key = '0';

    switch(app->userInput){
	case KEY_MOUSE:
            if(app->currentMode == 0 || app->currentMode == -1){
                if(getmouse(&event) == OK)
                    mouseInput(app, event, key);
            }
	    break;

        case ENTER:
            key = 'E';
            if(app->currentMode == 0 && app->needResume == 0)
                mainMenuInput(app, key);
            else if(app->currentMode == 0 && app->needResume == 1)
                resumeInput(app, key);
            else if(app->currentMode == -1)
                settingsInput(app, key);
            break;

        case KEY_UP:
        case 'K':
        case 'k':
            if(app->menuPos != 1 && app->needResume == 0)
                app->menuPos--;
            break;

        case KEY_DOWN:
        case 'J':
        case 'j':
            key = 'D';
            if(app->currentMode == -1)
                settingsInput(app, key);
            if(app->currentMode == 0 && app->needResume == 0)
                mainMenuInput(app, key);
            break;

        case KEY_LEFT:
        case 'H':
        case 'h':
            key = 'L';
            if(app->currentMode == -1)
                settingsInput(app, key);
            else if(app->currentMode == 0 && app->needResume == 1)
                resumeInput(app, key);
            break;

        case KEY_RIGHT:
        case 'L':
        case 'l':
            key = 'R';
            if(app->currentMode == 0 && app->needResume == 0)
                mainMenuInput(app, key);
            else if(app->currentMode == 0 && app->needResume == 1)
                resumeInput(app, key);
            else if(app->currentMode == -1)
                settingsInput(app, key);
            break;

        case CTRLX:
        case 'X':
        case 'x':
            if(app->currentMode != 0){
                app->frameTimer = 0;
                app->framems = 0;
                app->logoFrame = 0;
                app->currentMode = 0;
                app->menuPos = 1;
                app->pomodoroCounter = 0;
            }
            break;

        case CTRLP:
        case 'P':
        case 'p':
            if(app->currentMode != 0 && app->currentMode != -1){
                app->pausedTimer = app->pausedTimer ^ 1;
            }
            break;

        case ESC:
        case CTRLC:
        case 'Q':
        case 'q':
            printf("\033[?1003l\n");
            writeToLog(app);
            endTimerLog(app);
            endwin();
            exit(EXIT_SUCCESS);
            break;

        case KEY_RESIZE:
            endwin();
            initScreen();
            getWindowSize(app);
            clear();
            refresh();
            break;
        default:
            break;
    }

    /* Throws away any typeahead that has been typed by
     * the user and has not yet been read by the program */
    flushinp();
}

/* Handle mouse input */
void mouseInput(appData * app, MEVENT event, char key){
    if(app->currentMode == 0 && app->needResume == 0){
        if(event.y == (app->middley + 4) && (app->middlex + 2) >= event.x  && event.x >= (app->middlex - 2)){
            app->menuPos = 1;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                mainMenuInput(app, key);
            }
        }
        else if(event.y == (app->middley + 5) && (app->middlex + 5) >= event.x  && event.x >= (app->middlex - 5)){
            app->menuPos = 2;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                mainMenuInput(app, key);
            }
        }
        else if(event.y == (app->middley + 6) && (app->middlex + 2) >= event.x  && event.x >= (app->middlex - 2)){
            app->menuPos = 3;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                mainMenuInput(app, key);
            }
        }
    }
    else if(app->currentMode == 0 && app->needResume == 1){
        if(event.y == (app->middley) && (app->middlex - 2) >= event.x  && event.x >= (app->middlex - 10)){
            app->menuPos = 1;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                resumeInput(app, key);
            }
        }
        else if(event.y == (app->middley) && (app->middlex + 9) >= event.x  && event.x >= (app->middlex + 2)){
            app->menuPos = 2;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                resumeInput(app, key);
            }
        }
    }
    else if(app->currentMode == -1){
        if(event.y == (app->middley - 2) && (app->middlex + 9) >= event.x  && event.x >= (app->middlex - 9)){
            app->menuPos = 1;
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley - 2) && (app->middlex - 8) >= event.x  && event.x >= (app->middlex - 9)){
                key = 'L';
                settingsInput(app, key);
            }
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley - 2) && (app->middlex + 9) >= event.x  && event.x >= (app->middlex + 8)){
                key = 'R';
                settingsInput(app, key);
            }
        }
        else if(event.y == (app->middley - 1) && (app->middlex + 9) >= event.x  && event.x >= (app->middlex - 9)){
            app->menuPos = 2;
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley - 1) && (app->middlex - 8) >= event.x  && event.x >= (app->middlex - 9)){
                key = 'L';
                settingsInput(app, key);
            }
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley - 1) && (app->middlex + 9) >= event.x  && event.x >= (app->middlex + 8)){
                key = 'R';
                settingsInput(app, key);
            }
        }
        else if(event.y == app->middley && (app->middlex + 10) >= event.x  && event.x >= (app->middlex - 10)){
            app->menuPos = 3;
            if(event.bstate & BUTTON1_PRESSED && event.y == app->middley && (app->middlex - 9) >= event.x  && event.x >= (app->middlex - 10)){
                key = 'L';
                settingsInput(app, key);
            }
            if(event.bstate & BUTTON1_PRESSED && event.y == app->middley && (app->middlex + 10) >= event.x  && event.x >= (app->middlex + 9)){
                key = 'R';
                settingsInput(app, key);
            }
        }
        else if(event.y == (app->middley + 1) && (app->middlex + 10) >= event.x  && event.x >= (app->middlex - 10)){
            app->menuPos = 4;
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley + 1) && (app->middlex - 9) >= event.x  && event.x >= (app->middlex - 10)){
                key = 'L';
                settingsInput(app, key);
            }
            if(event.bstate & BUTTON1_PRESSED && event.y == (app->middley + 1) && (app->middlex + 10) >= event.x  && event.x >= (app->middlex + 9)){
                key = 'R';
                settingsInput(app, key);
            }
        }
        else if(event.y == (app->middley + 4) && (app->middlex + 8) >= event.x  && event.x >= (app->middlex - 8)){
            app->menuPos = 5;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                settingsInput(app, key);
            }
        }
    }
}

/* Handle input at the main menu */
void mainMenuInput(appData * app, char key){
    if(key == 'E'){
        if(app->menuPos == 1){
            if(app->timer == 0)
                app->timer = app->workTime;
            app->frameTimer = 0;
            app->currentMode = 1;
            if(app->pomodoroCounter == 0)
                app->pomodoroCounter = 1;
            notify("worktime");
        }
        else if(app->menuPos == 2){
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
            printf("\033[?1003l\n");
            endTimerLog(app);
            endwin();
            exit(EXIT_SUCCESS);
        }
    }
    else if(key == 'D'){
        if(app->menuPos != 3)
            app->menuPos++;
    }
    else if(key == 'R'){
        if(app->menuPos == 1){
            if(app->timer == 0)
                app->timer = app->workTime;
            app->frameTimer = 0;
            app->currentMode = 1;
            if(app->pomodoroCounter == 0)
                app->pomodoroCounter = 1;
            notify("worktime");
        }
        else if(app->menuPos == 2){
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
            printf("\033[?1003l\n");
            endTimerLog(app);
            endwin();
            exit(EXIT_SUCCESS);
        }
    }else
        return;
}

/* Handle input at the settings menu */
void settingsInput(appData * app, char key){
    if(key == 'E'){
        if(app->menuPos == 5){
            app->frameTimer = 0;
            app->logoFrame = 0;
            app->currentMode = 0;
            app->menuPos = 1;
        }
    }
    else if(key == 'D'){
        if(app->menuPos != 5)
            app->menuPos++;
    }
    else if(key == 'L'){
        if(app->menuPos == 1){
            if(app->pomodoros != 1)
                app->pomodoros --;
        }
        else if(app->menuPos == 2){
            if(app->workTime != (5 * 60 * 8))
                app->workTime = app->workTime - (5 * 60 * 8);
        }
        else if(app->menuPos == 3){
            if(app->shortPause != (1 * 60 * 8))
                app->shortPause = app->shortPause - (1 * 60 * 8);
        }
        else if(app->menuPos == 4){
            if(app->longPause != (5 * 60 * 8))
                app->longPause = app->longPause - (5 * 60 * 8);
        }else{
            app->frameTimer = 0;
            app->logoFrame = 0;
            app->currentMode = 0;
            app->menuPos = 1;
        }
    }
    else if(key == 'R'){
        if(app->menuPos == 1){
            if(app->pomodoros != 8)
                app->pomodoros ++;
        }
        else if(app->menuPos == 2){
            if(app->workTime != (50 * 60 * 8))
                app->workTime = app->workTime + (5 * 60 * 8);
        }
        else if(app->menuPos == 3){
            if(app->shortPause != (10 * 60 * 8))
                app->shortPause = app->shortPause + (1 * 60 * 8);
        }
        else if(app->menuPos == 4){
            if(app->longPause != (60 * 60 * 8))
                app->longPause = app->longPause + (5 * 60 * 8);
        }else{
            app->frameTimer = 0;
            app->logoFrame = 0;
            app->currentMode = 0;
            app->menuPos = 1;
        }
    }else
        return;
}

/* Handle input at the resume menu */
void resumeInput(appData * app, char key){
    if(key == 'E'){
        if(app->menuPos == 1){
            setLogVars(app);
            app->needResume = 0;
        }
        else if(app->menuPos == 2){
            app->needResume = 0;
            app->menuPos = 1;
            deleteLastLog(app);
        }
    }
    else if(key == 'R')
        app->menuPos = 2;
    else if(key == 'L')
        app->menuPos = 1;
}

