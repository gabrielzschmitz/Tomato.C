/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// util.c
*/
#include "util.h"
#include "input.h"
#include "update.h"
#include "anim.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>

/* Update variables */
void doUpdate(appData * app, const int NOTIFY, const int SOUND, const char * ICONS, const int WSL){

    /* Mode 0 (Main Menu) */
    if(app->currentMode == 0){
        app->pausedTimer = 0;
        frameTimer(app);

        /* Tomato Asounds/nimation */
        if(app->frameTimer == (1 * 16)) app->logoFrame = 1;
        else if(app->frameTimer == (2 * 16)) app->logoFrame = 2;
        else if(app->frameTimer == (3 * 16)) app->logoFrame = 3;
        else if(app->frameTimer == (4 * 16)) app->logoFrame = 4;
        else if(app->frameTimer == (5 * 16)) app->logoFrame = 5;
        else if(app->frameTimer == (6 * 16)) app->logoFrame = 6;
        else if(app->frameTimer == (7 * 16)) app->logoFrame = 7;
        else if(app->frameTimer == (8 * 16)){
            app->logoFrame = 0;
            app->frameTimer = 0;
        }
    }

    /* Mode 1 (Work Time) */
    if(app->currentMode == 1){
        timer(app);
        frameTimer(app);
        if(app->timer == 0){
            if(app->pomodoroCounter == app->pomodoros){
                app->timer = (app->longPause * 60 * 16);
                app->pomodoroCounter = 0;
                app->frameTimer = 0;
                app->currentMode = 3;
            #ifdef __APPLE__
                if(NOTIFY == 1){
                    if(strcmp(ICONS, "nerdicons") == 0)
                        system("osascript -e \'display notification \" Pause Break\" with title \"You have some time chill\"\'");
                    else if(strcmp(ICONS, "iconson") == 0)
                        system("osascript -e \'display notification \"🌴 Pause Break\" with title \"You have some time chill\"\'");
                    else
                        system("osascript -e \'display notification \"Long Pause Break\" with title \"You have some time chill\"\'");
                }
            #else
                if(NOTIFY == 1){
                    if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                        system("notify-send -t 5000 -c Tomato.C \' Pause Break\' \'You have some time chill\'");
                    else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                        system("notify-send -t 5000 -c Tomato.C \'🌴 Pause Break\' \'You have some time chill\'");
                    else
                        system("notify-send -t 5000 -c Tomato.C \'Long Pause Break. You have some time chill\'");
                }
            #endif
                if(SOUND == 1 && WSL == 0)
                    system("mpv --no-vid --volume=50 /usr/local/share/tomato/sounds/pausenotify.mp3 --really-quiet &");
            }else{
                app->timer = (app->shortPause * 60 * 16);
                app->frameTimer = 0;
                app->currentMode = 2;
            #ifdef __APPLE__
                if(NOTIFY == 1){
                    if(strcmp(ICONS, "nerdicons") == 0)
                        system("osascript -e \'display notification \" Pause Break\" with title \"You have some time chill\"\'");
                    else if(strcmp(ICONS, "iconson") == 0)
                        system("osascript -e \'display notification \"☕ Pause Break\" with title \"You have some time chill\"\'");
                    else
                        system("osascript -e \'display notification \"Pause Break\" with title \"You have some time chill\"\'");
                }
            #else
                if(NOTIFY == 1){
                    if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                        system("notify-send -t 5000 -c Tomato.C \' Pause Break\' \'You have some time chill\'");
                    else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                        system("notify-send -t 5000 -c Tomato.C \'☕ Pause Break\' \'You have some time chill\'");
                    else
                        system("notify-send -t 5000 -c Tomato.C \'Pause Break. You have some time chill\'");
                }
            #endif
                if(SOUND == 1 && WSL == 0)
                    system("mpv --no-vid --volume=50 /usr/local/share/tomato/sounds/pausenotify.mp3 --really-quiet &");
            }
        }

        /* Coffee Animation */
        if(app->frameTimer == (3 * 16)) app->coffeeFrame = 1;
        else if(app->frameTimer == (6 * 16)){
            app->coffeeFrame = 0;
            app->frameTimer = 0;
        }
    }

    /* Mode 2 (Short Pause) */
    if(app->currentMode == 2){
        timer(app);
        frameTimer(app);
        if(app->timer == 0){
            app->timer = (app->workTime * 60 * 16);
            app->frameTimer = 0;
            app->currentMode = 1;
            app->pomodoroCounter = app->pomodoroCounter + 1;
        #ifdef __APPLE__
            if(NOTIFY == 1){
                if(strcmp(ICONS, "nerdicons") == 0)
                    system("osascript -e \'display notification \"華 Work!\" with title \"You need to focus\"\'");
                else if(strcmp(ICONS, "iconson") == 0)
                    system("osascript -e \'display notification \"👷 Work!\" with title \"You need to focus\"\'");
                else
                    system("osascript -e \'display notification \"Work!\" with title \"You need to focus\"\'");
            }
        #else
            if(NOTIFY == 1){
                if(strcmp(ICONS, "nerdicons") == 0 && WSL == 0)
                    system("notify-send -t 5000 -c Tomato.C \'華 Work!\' \'You need to focus\'");
                else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                    system("notify-send -t 5000 -c Tomato.C \'👷 Work!\' \'You need to focus\'");
                else
                    system("notify-send -t 5000 -c Tomato.C \'Work! You need to focus\'");
            }
        #endif
            if(SOUND == 1 && WSL == 0)
                system("mpv --no-vid --volume=50 /usr/local/share/tomato/sounds/dfltnotify.mp3 --really-quiet &");
        }

        /* Machine Animation */
        if(app->frameTimer == (2 * 16)) app->machineFrame = 1;
        else if(app->frameTimer == (4 * 16)) app->machineFrame = 2;
        else if(app->frameTimer == (6 * 16)){
            app->machineFrame = 0;
            app->frameTimer = 0;
        }
    }

    /* Mode 3 (Long Pause) */
    if(app->currentMode == 3){
        timer(app);
        frameTimer(app);
        if(app->timer == 0)
            app->currentMode = 0;
        
        /* Beach Animation */
        if(app->frameTimer == (3 * 16)) app->beachFrame = 1;
        else if(app->frameTimer == (6 * 16)){
            app->beachFrame = 0;
            app->frameTimer = 0;
        }
    }

    /*Get X and Y window size*/
    getWindowSize(app);
}

