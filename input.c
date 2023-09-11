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
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

/* Handle user input and app state */
void handleInputs(appData * app){
    // Setting delay for ESC key
    ESCDELAY = 25;

    MEVENT event;
    app->userInput = getch();
    char key;
    key = '0';

    if(app->inputMode == 'n'){
        switch(app->userInput){
	        case KEY_MOUSE:
                if(getmouse(&event) == OK)
                    mouseInput(app, event, key);
	        break;

            case '1':
            case 'r':
                if(app->needResume != 1)
                    toggleNoise(app, 1);
                break;
            case 'R':
                if(app->playRainNoise == 1){
                    app->printVolume = 1;
                    controlVolumeNoise(app, 1, '+');
                }
                break;
            case CTRLR:
                if(app->playRainNoise == 1){
                    app->printVolume = 1;
                    controlVolumeNoise(app, 1, '-');
                }
                break;

            case '2':
            case 'f':
                if(app->needResume != 1)
                    toggleNoise(app, 2);
                break;
            case 'F':
                if(app->playFireNoise == 1){
                    app->printVolume = 2;
                    controlVolumeNoise(app, 2, '+');
                }
                break;
            case CTRLF:
                if(app->playFireNoise == 1){
                    app->printVolume = 2;
                    controlVolumeNoise(app, 2, '-');
                }
                break;

            case '3':
            case 'w':
                if(app->needResume != 1)
                    toggleNoise(app, 3);
                break;
            case 'W':
                if(app->playWindNoise == 1){
                    app->printVolume = 3;
                    controlVolumeNoise(app, 3, '+');
                }
                break;
            case CTRLW:
                if(app->playWindNoise == 1){
                    app->printVolume = 3;
                    controlVolumeNoise(app, 3, '-');
                }
                break;
            
            case '4':
            case 't':
                if(app->needResume != 1)
                    toggleNoise(app, 4);
                break;
            case 'T':
                if(app->playThunderNoise == 1){
                    app->printVolume = 4;
                    controlVolumeNoise(app, 4, '+');
                }
                break;
            case CTRLT:
                if(app->playThunderNoise == 1){
                    app->printVolume = 4;
                    controlVolumeNoise(app, 4, '-');
                }
                break;

            case 'D':
            case CTRLD:
                if(app->currentNote == 0 && app->notesAmount == 1){
                    app->notes.lines[app->currentNote]->note = NULL;
                    app->notesAmount -= 1;
                }
                else if(app->notesAmount - 1 > app->currentNote){
                    int i = app->currentNote;
                    while(i < app->notesAmount - 1){
                        strcpy(app->notes.lines[i]->note, app->notes.lines[i + 1]->note);
                        app->notes.lines[i]->type = app->notes.lines[i + 1]->type;
                        i++;
                    }
                    app->notes.lines[app->notesAmount - 1]->note = NULL;
                    if(app->currentNote > 0)
                        app->currentNote -= 1;
                    app->notesAmount -= 1;
                }
                else if(app->notesAmount - 1 == app->currentNote){
                    app->notes.lines[app->currentNote]->note = NULL;
                    if(app->currentNote > 0)
                        app->currentNote -= 1;
                    app->notesAmount -= 1;
                }
                break;

            case 'A':
                if(app->currentMode == -2 && app->notesAmount < MAXLINES && NOTEPAD == 1){
                    app->addingTask = 1;
                    app->inputLength = 0;
                    app->inputMode = 'i';
                    app->notes.lines[app->notesAmount] = createNote('o');
                    app->notes.lines[app->notesAmount]->note = (char *)malloc(sizeof(char) * MAXINPUTLENGTH + 1);
                    app->notes.lines[app->notesAmount]->note[0] = '\0';
                    app->emptyNotepad = 0;
                }
                break;
            case 'a':
                if(app->currentMode == -2 && app->notesAmount < MAXLINES && NOTEPAD == 1){
                    app->addingNote = 1;
                    app->inputLength = 0;
                    app->inputMode = 'i';
                    app->notes.lines[app->notesAmount] = createNote('-');
                    app->notes.lines[app->notesAmount]->note = (char *)malloc(sizeof(char) * MAXINPUTLENGTH + 1);
                    app->notes.lines[app->notesAmount]->note[0] = '\0';
                    app->emptyNotepad = 0;
                }
                break;

            case 'N':
            case 'n':
                if(app->currentMode != -2 && NOTEPAD == 1){
                    app->lastMode = app->currentMode;
                    app->currentMode = -2;
                }
                else if(NOTEPAD == 1){
                    app->currentMode = app->lastMode;
                    app->lastMode = -2;
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

            case ' ':
                if(app->currentMode == -2 && app->emptyNotepad != 1 && NOTEPAD == 1){
                    if(app->notes.lines[app->currentNote]->type == 'o')
                        app->notes.lines[app->currentNote]->type = 'x';
                    else if(app->notes.lines[app->currentNote]->type == 'x')
                        app->notes.lines[app->currentNote]->type = 'o';
                }
                break;

            case KEY_UP:
            case 'K':
            case 'k':
                if(app->currentNote != 0 && app->currentMode == -2)
                    app->currentNote--;
                if(app->menuPos != 1 && app->needResume == 0)
                    app->menuPos--;
                break;

            case KEY_DOWN:
            case 'J':
            case 'j':
                if(app->currentNote != app->notesAmount - 1 && app->currentMode == -2)
                    app->currentNote++;
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
                    app->lastMode = app->currentMode;
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
                if(app->currentMode == 0){
                    printf("\033[?1003l\n");
                    killNoise();
                    if(WORKLOG == 1)
                        writeToLog(app);
                    if(TIMERLOG == 1)
                        endTimerLog(app);
                    endwin();
                    printf("Goodbye!\n");
                    exit(EXIT_SUCCESS);
                }
                else if(app->currentMode == -2){
                    if(app->currentMode != -2 && NOTEPAD == 1){
                        app->lastMode = app->currentMode;
                        app->currentMode = -2;
                    }
                    else if(NOTEPAD == 1){
                        app->currentMode = app->lastMode;
                        app->lastMode = -2;
                    }
                }
                else{
                    app->runOnce = 1;
                    app->pausedTimer = 0;
                    app->currentMode = 0;
                    app->frameTimer = 0;
                    app->timer = 0;
                    app->timerms = 0;
                }
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
    }
    else if(app->inputMode == 'i'){
        if(app->addingNote == 1 && app->userInput != -1 && app->userInput != KEY_RESIZE && app->userInput != KEY_MOUSE){
            inputNote(app);
        }
        if(app->addingTask == 1 && app->userInput != -1 && app->userInput != KEY_RESIZE && app->userInput != KEY_MOUSE){
            inputTask(app);
        }
    }

    /* Throws away any typeahead that has been typed by
     * the user and has not yet been read by the program */
    flushinp();
}

/* Input note */
void inputNote(appData * app){
    if((app->userInput == BACKSPACE || app->userInput == KEY_BACKSPACE)&& app->inputLength > 0){
        app->notes.lines[app->notesAmount]->note[app->inputLength - 1] = '\0';
        app->inputLength -= 1;
    }
    else{
        if(app->userInput == ESC || (app->userInput == ENTER && app->inputLength < 1)){
            app->addingNote = 0;
            app->inputMode = 'n';
            app->inputLength = 0;
        }
        else{
            app->notes.lines[app->notesAmount]->note[app->inputLength] = app->userInput;
            app->notes.lines[app->notesAmount]->note[app->inputLength + 1] = '\0';
            app->inputLength += 1;
            if((app->inputLength > MAXINPUTLENGTH - 1 && app->inputLength > 1) || (app->userInput == ENTER && app->inputLength > 1)){
                app->notesAmount += 1;
                app->inputLength = 0;
                app->addingNote = 0;
                app->inputMode = 'n';
            }
        }
    }
}

/* Input task */
void inputTask(appData * app){
    if((app->userInput == BACKSPACE || app->userInput == KEY_BACKSPACE) && app->inputLength > 0){
        app->notes.lines[app->notesAmount]->note[app->inputLength - 1] = '\0';
        app->inputLength -= 1;
    }
    else{
        if(app->userInput == ESC || (app->userInput == ENTER && app->inputLength < 1)){
            app->addingTask = 0;
            app->inputMode = 'n';
            app->inputLength = 0;
        }
        else{
            app->notes.lines[app->notesAmount]->note[app->inputLength] = app->userInput;
            app->notes.lines[app->notesAmount]->note[app->inputLength + 1] = '\0';
            app->inputLength += 1;
            if((app->inputLength > MAXINPUTLENGTH - 1 && app->inputLength > 1) || (app->userInput == ENTER && app->inputLength > 1)){
                app->notesAmount += 1;
                app->inputLength = 0;
                app->addingTask = 0;
                app->inputMode = 'n';
            }
        }
    }
}

/* Handle mouse input */
void mouseInput(appData * app, MEVENT event, char key){
    if(app->needResume != 1){
        /* Show volume bars */
        app->printVolume = 0;
        if((app->playRainNoise == 1) && (event.y == 1) && (2 <= event.x && event.x <= 19))
            app->printVolume = 1;
        else if((app->playFireNoise == 1) && (event.y == 2) && (2 <= event.x && event.x <= 19))
            app->printVolume = 2;
        else if((app->playWindNoise == 1) && (event.y == 3) && (2 <= event.x && event.x <= 19))
            app->printVolume = 3;
        else if((app->playThunderNoise == 1) && (event.y == 4) && (2 <= event.x && event.x <= 19))
            app->printVolume = 4;

        /* Toggle on or off notepad */
        if((event.y == 1) && (event.x == app->x - 2 || event.x == app->x - 1) && NOTEPAD == 1){
            if(event.bstate & BUTTON1_PRESSED){
                if(app->currentMode != -2){
                    app->lastMode = app->currentMode;
                    app->currentMode = -2;
                }
                else{
                    app->currentMode = app->lastMode;
                    app->lastMode = -2;
                }
            }
        }

        /* Toggle on or off noises */
        if((event.y == 1) && (event.x == 2 || event.x == 3)){
            if(event.bstate & BUTTON1_PRESSED)
                toggleNoise(app, 1);
        }
        else if((event.y == 2) && (event.x == 2 || event.x == 3)){
            if(event.bstate & BUTTON1_PRESSED)
                toggleNoise(app, 2);
        }
        else if((event.y == 3) && (event.x == 2 || event.x == 3)){
            if(event.bstate & BUTTON1_PRESSED)
                toggleNoise(app, 3);
        }
        else if((event.y == 4) && (event.x == 2 || event.x == 3)){
            if(event.bstate & BUTTON1_PRESSED)
                toggleNoise(app, 4);
        }

        /* Noise volume control */
        if((app->printVolume == 1) && (event.y == 1) && (event.x == 5) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 1, '-');
        else if((app->printVolume == 1) && (event.y == 1) && (event.x == 18) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 1, '+');
        if((app->printVolume == 2) && (event.y == 2) && (event.x == 5) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 2, '-');
        else if((app->printVolume == 2) && (event.y == 2) && (event.x == 18) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 2, '+');
        if((app->printVolume == 3) && (event.y == 3) && (event.x == 5) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 3, '-');
        else if((app->printVolume == 3) && (event.y == 3) && (event.x == 18) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 3, '+');
        if((app->printVolume == 4) && (event.y == 4) && (event.x == 5) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 4, '-');
        else if((app->printVolume == 4) && (event.y == 4) && (event.x == 18) && (event.bstate & BUTTON1_PRESSED))
            controlVolumeNoise(app, 4, '+');
        if((app->printVolume == 1) && (event.bstate & BUTTON5_PRESSED))
            controlVolumeNoise(app, 1, '-');
        else if((app->printVolume == 1) && (event.bstate & BUTTON4_PRESSED))
            controlVolumeNoise(app, 1, '+');
        if((app->printVolume == 2) && (event.bstate & BUTTON5_PRESSED))
            controlVolumeNoise(app, 2, '-');
        else if((app->printVolume == 2) && (event.bstate & BUTTON4_PRESSED))
            controlVolumeNoise(app, 2, '+');
        if((app->printVolume == 3) && (event.bstate & BUTTON5_PRESSED))
            controlVolumeNoise(app, 3, '-');
        else if((app->printVolume == 3) && (event.bstate & BUTTON4_PRESSED))
            controlVolumeNoise(app, 3, '+');
        if((app->printVolume == 4) && (event.bstate & BUTTON5_PRESSED))
            controlVolumeNoise(app, 4, '-');
        else if((app->printVolume == 4) && (event.bstate & BUTTON4_PRESSED))
            controlVolumeNoise(app, 4, '+');

    }
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
        if(event.y == (app->middley + 1) && (app->middlex - 7) >= event.x  && event.x >= (app->middlex - 15)){
            app->menuPos = 1;
            if(event.bstate & BUTTON1_PRESSED){
                key = 'E';
                resumeInput(app, key);
            }
        }
        else if(event.y == (app->middley + 1) && (app->middlex + 14) >= event.x  && event.x >= (app->middlex + 7)){
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
    if(app->currentMode != 0 && app->currentMode != -1){
        if(event.y == (app->middley - 7) && (app->middlex - 10) >= event.x && event.x >= (app->middlex - 11)){
            if(event.bstate & BUTTON1_PRESSED)
                app->pausedTimer = app->pausedTimer ^ 1;
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
            app->lastMode = app->currentMode;
            app->currentMode = 1;
            if(app->pomodoroCounter == 0)
                app->pomodoroCounter = 1;
            notify("worktime");
        }
        else if(app->menuPos == 2){
            app->lastMode = app->currentMode;
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
            printf("\033[?1003l\n");
            killNoise();
            if(TIMERLOG == 1)
                endTimerLog(app);
            endwin();
            printf("Goodbye!\n");
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
            app->lastMode = app->currentMode;
            app->currentMode = 1;
            if(app->pomodoroCounter == 0)
                app->pomodoroCounter = 1;
            notify("worktime");
        }
        else if(app->menuPos == 2){
            app->lastMode = app->currentMode;
            app->currentMode = -1;
            app->menuPos = 1;
        }else{
            printf("\033[?1003l\n");
            killNoise();
            if(TIMERLOG == 1)
                endTimerLog(app);
            endwin();
            printf("Goodbye!\n");
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
            app->lastMode = app->currentMode;
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
            app->lastMode = app->currentMode;
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
            if(app->workTime != (75 * 60 * 8))
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
            app->lastMode = app->currentMode;
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

