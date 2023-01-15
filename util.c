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

/* Initialize screen with colors, enabled keyboard and another little configs */
void initScreen(void){
#ifdef XCURSES
    Xinitscr(argc, argv);
#else
    initscr();
#endif
    use_default_colors();
    if (has_colors()){
        int bg = 0, fg = 0;
        start_color();
        for(bg = COLOR_BLACK; bg <= COLOR_WHITE; bg++)
            for(fg = COLOR_BLACK; fg <= COLOR_WHITE; fg++)
                init_pair(bg*PALLETE_SIZE + fg + 1, fg, -1);
    }
    /* User input dont appear at screen */
    noecho();
    /* Makes terminal report mouse movement events */
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n");
    /* User input imediatly avaiable */
    mouseinterval(0);
    cbreak();
    /* Invisible cursor */
    curs_set(0);
    /* Non-blocking getch */
    nodelay(stdscr, TRUE);
    /* Enable keypad */
    keypad(stdscr, TRUE);
}

/* Set text foreground and background colors */
void setColor(short int fg, short int bg, chtype attr){
    chtype color = COLOR_PAIR(bg*PALLETE_SIZE + fg + 1);
    color |= attr;
    attrset(color);
}

/* Get the window X and Y size */
void getWindowSize(appData * app){
    getmaxyx(stdscr, app->y, app->x);
}

/* Time the pomodoros */
void timer(appData * app){
    int sec = 60;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        if(app->pausedTimer != 1){
            /* Debug */
            //app->timer = app->timer - 1;
            app->timerms++;
            if(app->timerms >= 7.745966692){
                app->timerms = 0;
                app->timer = app->timer - 1;
            }
        }
    }
}

/* Print the pomodoro counter */
void printPomodoroCounter(appData * app){
    if(app->currentMode == 1)
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);

    mvprintw(((app->y / 2) - 7), ((app->x / 2) + 10) ,"%02d", app->pomodoroCounter);
}

/* Print the pause indicator */
void printPauseIndicator(appData * app, const char * ICONS){
    if(app->currentMode == 1)
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);

    if(strcmp(ICONS, "nerdicons") == 0){
        if(app->pausedTimer == 1)
            mvprintw(((app->y / 2) - 7), ((app->x / 2) - 11) ," ");
    }
    else if(strcmp(ICONS, "iconson") == 0){
        if(app->pausedTimer == 1)
            mvprintw(((app->y / 2) - 7), ((app->x / 2) - 11) ,"⏸️ ");
    }
    else{
        if(app->pausedTimer == 1)
            mvprintw(((app->y / 2) - 7), ((app->x / 2) - 11) ,"P ");;
    }
}

/* Print the Main Menu */
void printMainMenu(appData * app, const char * ICONS){
    printLogo(app, ICONS);

    if(app->menuPos == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 5), "-> start <-");
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 2), "start");
    }

    if(app->menuPos == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 8), "-> preferences <-");
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 5), "preferences");
    }

    if(app->menuPos == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 5), "-> leave <-");
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 2), "leave");
    }
}

/* Print the Main Menu */
void printSettings(appData * app){
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(((app->y / 2) - 4), ((app->x / 2) - 5), "preferences");
    if(app->menuPos == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9), "<- pomodoros  %02d ->", app->pomodoros);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 6), "pomodoros  %02d", app->pomodoros);
    }

    if(app->menuPos == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9), "<- work time %02dm ->", app->workTime);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 6), "work time %02dm", app->workTime);
    }

    if(app->menuPos == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10), "<- short pause %02dm ->", app->shortPause);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2)), ((app->x / 2) - 7), "short pause %02dm", app->shortPause);
    }
    
    if(app->menuPos == 4){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10), "<- long pause  %02dm ->", app->longPause);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 7), "long pause  %02dm", app->longPause);
    }
    if(app->menuPos == 5){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 11), "-> back to main menu <-");
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 8), "back to main menu");
    }
}
/* Print the Timer */
void printTimer(appData * app, const char * ICONS){
    int x = app->timer / 8;
    int div = x / 60;
    int mod = x % 60;

    char minutes[3] = {(div / 10) + '0', (div % 10) + '0', '\0'};
    char seconds[3] = {(mod / 10) + '0', (mod % 10) + '0', '\0'};

    if(app->currentMode == 1){
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 11), " Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 0), "[%02d minutes]", app->workTime);
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 12), "🍅 Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 1), "[%02d minutes]", app->workTime);
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), "Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->workTime);
        }
    }
    else if(app->currentMode == 2){
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), " Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->shortPause);
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), "☕ Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->shortPause);
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 9), "Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 2), "[%02d minutes]", app->shortPause);
        }
    }
    else{
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 12), " Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 1), "[%02d minutes]", app->longPause);
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 13), "🌴 Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 2), "[%02d minutes]", app->longPause);
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 11), "Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 0), "[%02d minutes]", app->longPause);
        }
    }
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(((app->y / 2) + 7), ((app->x / 2) - 2), "%s:%s", minutes, seconds);
}

