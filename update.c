/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// update.c
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

/* Mode 0 (Main Menu) */
void updateMainMenu(appData * app){
    if(app->currentMode == 0){
        app->pausedTimer = 0;
        frameTimer(app);

        if(app->needResume == 0){
            /* Tomato Animation */
            if(app->frameTimer == (1 * 8)) app->logoFrame = 1;
            else if(app->frameTimer == (2 * 8)) app->logoFrame = 2;
            else if(app->frameTimer == (3 * 8)) app->logoFrame = 3;
            else if(app->frameTimer == (4 * 8)) app->logoFrame = 4;
            else if(app->frameTimer == (5 * 8)) app->logoFrame = 5;
            else if(app->frameTimer == (6 * 8)) app->logoFrame = 6;
            else if(app->frameTimer == (7 * 8)) app->logoFrame = 7;
            else if(app->frameTimer == (8 * 8)){
                app->logoFrame = 0;
                app->frameTimer = 0;
            }
        }else{
            if(app->runOnce == 1){
                /* Banner Animation */
                if(app->frameTimer == (0 * 1)) app->bannerFrame = 0;
                else if(app->frameTimer == (1 * 1)) app->bannerFrame = 1;
                else if(app->frameTimer == (2 * 1)) app->bannerFrame = 2;
                else if(app->frameTimer == (3 * 1)){
                    app->bannerFrame = 3;
                    app->frameTimer = 0;
                    app->runOnce = 0;
                }
            }
        }
    }
}

/* Mode 1 (Work Time) */
void updateWorkTime(appData * app){
    if(app->currentMode == 1){
        if(app->runOnce == 1){
            notify("worktime");
            app->runOnce = 0;
        }

        timer(app);
        frameTimer(app);
        if(app->timer <= 0){
            if(app->pomodoroCounter == app->pomodoros){
                app->timer = app->longPause;
                app->frameTimer = 0;
                app->currentMode = 3;
                app->pomodoroCounter = app->pomodoros;
                app->runOnce = 1;
            }else{
                app->timer = app->shortPause;
                app->frameTimer = 0;
                app->currentMode = 2;
                app->runOnce = 1;
            }
        }

        /* Coffee Animation */
        if(app->frameTimer == (3 * 8)) app->coffeeFrame = 1;
        else if(app->frameTimer == (6 * 8)){
            app->coffeeFrame = 0;
            app->frameTimer = 0;
        }
    }
}

/* Mode 2 (Short Pause) */
void updateShortPause(appData * app){
    if(app->currentMode == 2){
        if(app->runOnce == 1){
            notify("shortpause");
            app->runOnce = 0;
        }

        timer(app);
        frameTimer(app);
        if(app->timer <= 0){
            app->timer = app->workTime;
            app->frameTimer = 0;
            app->currentMode = 1;
            app->pomodoroCounter = app->pomodoroCounter + 1;
            app->runOnce = 1;
        }

        /* Machine Animation */
        if(app->frameTimer == (2 * 8)) app->machineFrame = 1;
        else if(app->frameTimer == (4 * 8)) app->machineFrame = 2;
        else if(app->frameTimer == (6 * 8)){
            app->machineFrame = 0;
            app->frameTimer = 0;
        }
    }
}

/* Mode 3 (Long Pause) */
void updateLongPause(appData * app){
    if(app->currentMode == 3){
        if(app->runOnce == 1){
            notify("longpause");
            app->runOnce = 0;
        }

        timer(app);
        frameTimer(app);
        if(app->timer <= 0){
            app->currentMode = 0;
            app->cycles++;
            app->needToLog = 1;
            if(WORKLOG == 1)
                writeToLog(app);
            app->needToLog = 0;
            app->pomodoroCounter = 0;
            if(TIMERLOG == 1)
                endTimerLog(app);
            notify("end");
        }
        
        /* Beach Animation */
        if(app->frameTimer == (3 * 8)) app->beachFrame = 1;
        else if(app->frameTimer == (6 * 8)){
            app->beachFrame = 0;
            app->frameTimer = 0;
        }
    }
}

