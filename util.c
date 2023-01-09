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
#include <time.h>
#include <locale.h>

/* Initialize screen with colors, enabled keyboard and another little configs */
void initScreen(){
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
    /* User input imediatly avaiable*/
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
    int sec = 30;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        /* Debug */
        //app->timer = app->timer - 60;
        app->timer = app->timer - 1;
        setColor(COLOR_WHITE, COLOR_BLACK, COLOR_WHITE);
    }
}

/* Print the pomodoro counter */
void printPomodoroCounter(appData * app){
    char counter[3] = {(app->pomodoroCounter / 10) + '0', (app->pomodoroCounter % 10) + '0', '\0'};

    if(app->E == 'C'){
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 7), ((app->x / 2) + 10) ,"%s", counter);
    }
    else{
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 7), ((app->x / 2) + 10) ,"%s", counter);
    }
}

/* Print the Main Menu */
void printMainMenu(appData * app){
    printLogo(app);

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
    char pomodoros[3] = {(app->pomodoros / 10) + '0', (app->pomodoros % 10) + '0', '\0'};
    char workTime[3] = {(app->workTime / 10) + '0', (app->workTime % 10) + '0', '\0'};
    char shortPause[3] = {(app->shortPause / 10) + '0', (app->shortPause % 10) + '0', '\0'};
    char longPause[3] = {(app->longPause / 10) + '0', (app->longPause % 10) + '0', '\0'};

    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(((app->y / 2) - 4), ((app->x / 2) - 5), "preferences");
    if(app->menuPos == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9), "-> pomodoros  %s <-", pomodoros);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 6), "pomodoros  %s", pomodoros);
    }

    if(app->menuPos == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9), "-> work time %sm <-", workTime);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 6), "work time %sm", workTime);
    }

    if(app->menuPos == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10), "-> short pause %sm <-", shortPause);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2)), ((app->x / 2) - 7), "short pause %sm", shortPause);
    }
    
    if(app->menuPos == 4){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10), "-> long pause  %sm <-", longPause);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 7), "long pause  %sm", longPause);
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
void printTimer(appData * app){
    int x = app->timer / 16;
    int div = x / 60;
    int mod = x % 60;

    char workTime[3] = {(app->workTime / 10) + '0', (app->workTime % 10) + '0', '\0'};
    char shortPause[3] = {(app->shortPause / 10) + '0', (app->shortPause % 10) + '0', '\0'};
    char longPause[3] = {(app->longPause / 10) + '0', (app->longPause % 10) + '0', '\0'};

    char minutes[3] = {(div / 10) + '0', (div % 10) + '0', '\0'};
    char seconds[3] = {(mod / 10) + '0', (mod % 10) + '0', '\0'};

    if(app->E == 'C'){
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 11), " Pomodoro");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 0), "[%s minutes]", workTime);
    }
    else if(app->E == 'M'){
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), " Pause");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%s minutes]", shortPause);
    }
    else{
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 12), " Long pause");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 6), ((app->x / 2) + 1), "[%s minutes]", longPause);
    }
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(((app->y / 2) + 7), ((app->x / 2) - 2), "%s:%s", minutes, seconds);
}

