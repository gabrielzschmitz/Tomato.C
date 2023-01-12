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

/* Handle user input and app state */
void handleInputs(appData * app, const int NOTIFY, const int SOUND, const char * ICONS, const int WSL){
    app->userInput = getch();
    char key;

    switch(app->userInput){
        case ENTER:
            key = 'E';
            if(app->currentMode == 0)
                mainMenuInput(app, key, NOTIFY, SOUND, ICONS, WSL);
            else if(app->currentMode == -1)
                settingsInput(app, key);
            break;

        case KEY_UP:
        case 'K':
        case 'k':
            if(app->menuPos != 1)
                app->menuPos--;
            break;

        case KEY_DOWN:
        case 'J':
        case 'j':
            key = 'D';
            if(app->currentMode == -1)
                settingsInput(app, key);
            else if(app->currentMode == 0)
                mainMenuInput(app, key, NOTIFY, SOUND, ICONS, WSL);
            break;

        case KEY_LEFT:
        case 'H':
        case 'h':
            key = 'L';
            if(app->currentMode == -1)
                settingsInput(app, key);
            break;

        case KEY_RIGHT:
        case 'L':
        case 'l':
            key = 'R';
            if(app->currentMode == 0)
                mainMenuInput(app, key, NOTIFY, SOUND, ICONS, WSL);
            else if(app->currentMode == -1)
                settingsInput(app, key);
            break;

        case CTRLX:
        case 'X':
        case 'x':
            if(app->currentMode != 0){
                app->frameTimer = 0;
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
        case 'Q':
        case 'q':
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
    }

    /* Throws away any typeahead that has been typed by
     * the user and has not yet been read by the program */
    flushinp();
}

void mainMenuInput(appData * app, char key, const int NOTIFY, const int SOUND, const char * ICONS, const int WSL){
    if(key == 'E'){
        if(app->menuPos == 1){
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
                    system("notify-send -t 5000 -c cpomo \'華 Work!\' \'You need to focus\'");
                else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                    system("notify-send -t 5000 -c cpomo \'👷 Work!\' \'You need to focus\'");
                else
                    system("notify-send -t 5000 -c cpomo \'Work! You need to focus\'");
            }
        #endif
            if(SOUND == 1 && WSL == 0)
                system("mpv --no-vid --volume=50 /usr/local/share/tomato/sounds/dfltnotify.mp3 --really-quiet &");
        }
        else if(app->menuPos == 2){
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
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
                    system("notify-send -t 5000 -c cpomo \'華 Work!\' \'You need to focus\'");
                else if(strcmp(ICONS, "iconson") == 0 && WSL == 0)
                    system("notify-send -t 5000 -c cpomo \'👷 Work!\' \'You need to focus\'");
                else
                    system("notify-send -t 5000 -c cpomo \'Work! You need to focus\'");
            }
        #endif
            if(SOUND == 1 && WSL == 0)
                system("mpv --no-vid --volume=50 /usr/local/share/tomato/sounds/dfltnotify.mp3 --really-quiet &");
        }
        else if(app->menuPos == 2){
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
            endwin();
            exit(EXIT_SUCCESS);
        }
    }else
        return;
}

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
            if(app->workTime != 5)
                app->workTime = app->workTime - 5;
        }
        else if(app->menuPos == 3){
            if(app->shortPause != 1)
                app->shortPause = app->shortPause - 1;
        }
        else if(app->menuPos == 4){
            if(app->longPause != 5)
                app->longPause = app->longPause - 5;
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
            if(app->workTime != 50)
                app->workTime = app->workTime + 5;
        }
        else if(app->menuPos == 3){
            if(app->shortPause != 10)
                app->shortPause = app->shortPause + 1;
        }
        else if(app->menuPos == 4){
            if(app->longPause != 60)
                app->longPause = app->longPause + 5;
        }else{
            app->frameTimer = 0;
            app->logoFrame = 0;
            app->currentMode = 0;
            app->menuPos = 1;
        }
    }else
        return;
}

