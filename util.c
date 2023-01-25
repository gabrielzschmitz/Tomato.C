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
#include <sys/stat.h>
#include <sys/types.h>

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
    raw();
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

/* Set the log folder and files */
void createLog(appData * app){
    /* Get /home/user */
    char * home = getenv("HOME");

    /* Set log folder fullpath */
    char * logPrefix = NULL;
    logPrefix = malloc(strlen(home) + strlen(app->logPrefix) + 1);
    strcpy(logPrefix, home);
    strcat(logPrefix, "/");
    strcat(logPrefix, app->logPrefix);
    app->logPrefix = logPrefix;

    /* Set log file fullpath */
    char * logFile= NULL;
    logFile = malloc(strlen(home) + strlen(app->logFile) + 1);
    strcpy(logFile, home);
    strcat(logFile, "/");
    strcat(logFile, app->logFile);
    app->logFile = logFile;

    /* Set tmp file fullpath */
    char * tmpFile= NULL;
    tmpFile = malloc(strlen(home) + strlen(app->tmpFile) + 1);
    strcpy(tmpFile, home);
    strcat(tmpFile, "/");
    strcat(tmpFile, app->tmpFile);
    app->tmpFile = tmpFile;
    
    /* Create log folder */
    mkdir(app->logPrefix, 0766);

    /* Create log file if doesn't exist */
    FILE *log;
    log = fopen(app->logFile, "r");
    if(log)
        fclose(log);
    else{
        log = fopen(app->logFile, "w");
        fprintf(log, "# Month/Day/Year\n"
        "# mm/dd/yy\n"
        "#\n"
        "# Total session time (in minutes)\n"
        "# TT %%dmin\n"
        "#\n"
        "# Default values - WorkTime ShortPause LongPause (in ticks)\n"
        "# D %%d %%d %%d\n"
        "#\n"
        "# Current work time (in ticks)\n"
        "# WT %%d\n"
        "#\n"
        "# Current short pause (in ticks)\n"
        "# SP %%d\n"
        "#\n"
        "# Current long pause (in ticks)\n"
        "# LP %%d\n"
        "#\n"
        "# Current pomodoro/Total pomodoros\n"
        "# %%d/%%d\n"
        "#");
        fclose(log);
    }
}

/* Read log file at startup */
void readLog(appData * app){
    FILE *log;
    log = fopen(app->logFile, "r");
    
    /* Get date and set if app was already runned in the day */
    char line[4095+1];
    time_t time_raw_format;
    struct tm * ptr_time;

    time (&time_raw_format);
    ptr_time = localtime (&time_raw_format);
    if(strftime(app->date, 50, "%d/%m/%Y", ptr_time) == 0){
        perror("Couldn't read log file");
    }else{
        while(fgets(line, sizeof line, log))
            if(strstr(line, app->date))
                app->newDay = 0;
    }

    /* Get last line of the log file */
    fseek(log, 0, SEEK_SET);
    char lastline[1024]={0,};
    while(fgets(lastline, 1024, log) != NULL){/* Just to get the last line */}

    /* Check for session to resume */
    if(strstr(lastline, "WT") != NULL || strstr(lastline, "SP") != NULL || strstr(lastline, "LP") != NULL)
        app->needResume = 1;

    fclose(log);
}

/* Delete the last line of the log file */
void deleteLastLog(appData * app){
    FILE *log;
    log = fopen(app->logFile, "r");
    char lastline[1024]={0,};
    int lastlineIndex = 0;
    while(fgets(lastline, 1024, log) != NULL){lastlineIndex++;}

    fseek(log, 0, SEEK_SET);
    FILE *tmp;
    tmp = fopen(app->tmpFile, "w");
    
    char line[1024];
    int lineCounter = 0;
    
    while(fgets(line, 1024, log) != NULL){
        if(lineCounter != lastlineIndex - 1)
            fputs(line, tmp);
        lineCounter++;
    }
    
    fclose(log);
    fclose(tmp);

    rename(app->tmpFile, app->logFile);
}
/* Set the variables as described in the Log instead of the default */
void setLogVars(appData * app){
    FILE *log;
    log = fopen(app->logFile, "r");
    
    /* Get last line of the log file */
    fseek(log, 0, SEEK_SET);
    char lastline[1024]={0,};
    int lastlineIndex = 0;
    /* Just to get the last line and count lines */
    while(fgets(lastline, 1024, log) != NULL){lastlineIndex++;}

    /* Get variables */
    if(sscanf(lastline, "%d/%d WT %d D %d %d %d",
              &app->pomodoroCounter, &app->pomodoros, &app->timer, &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->workTime) * -1;
        app->frameTimer = 0;
        app->currentMode = 1;
    }
    else if(sscanf(lastline, "%d/%d SP %d D %d %d %d",
                   &app->pomodoroCounter, &app->pomodoros, &app->timer, &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->shortPause) * -1;
        app->frameTimer = 0;
        app->currentMode = 2;
    }
    else if(sscanf(lastline, "%d/%d LP %d D %d %d %d",
                   &app->pomodoroCounter, &app->pomodoros, &app->timer, &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->longPause) * -1;
        app->frameTimer = 0;
        app->currentMode = 3;
    }

    fclose(log);
    deleteLastLog(app);
}

/* Print current infos in the log file */
void printLog(appData * app){
    int totalTime = ((app->pomodoros * app->workTime) + ((app->pomodoros - 1) * app->shortPause) + app->longPause) / (60 * 8);

    FILE *log;
    log = fopen(app->logFile, "a+");
    if(app->newDay == 1)
        fprintf(log, "%s\n", app->date);

    switch(app->currentMode){
        case -1:
        case 0:
            if(app->cycles != 0 && app->needToLog == 1){
                fprintf(log, "TT %dmin D %d %d %d\n", totalTime,
                        app->workTime,
                        app->shortPause,
                        app->longPause);
            }
            break;

        case 1:
            fprintf(log, "%d/%d WT %d D %d %d %d\n",
                    app->pomodoroCounter,
                    app->pomodoros,
                    (app->workTime - app->timer),
                    app->workTime,
                    app->shortPause,
                    app->longPause);
            break;

        case 2:
            fprintf(log, "%d/%d SP %d D %d %d %d\n",
                    app->pomodoroCounter,
                    app->pomodoros,
                    (app->shortPause - app->timer),
                    app->workTime,
                    app->shortPause,
                    app->longPause);
            break;

        case 3:
            fprintf(log, "%d/%d LP %d D %d %d %d\n",
                    app->pomodoros,
                    app->pomodoros,
                    (app->longPause - app->timer),
                    app->workTime,
                    app->shortPause,
                    app->longPause);
            break;

    }
    fclose(log);
}

/* Print resume menu */
void printResume(appData * app){
    if(app->needResume == 1){
        /* Up and Down */
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        for(int i = 0; i < 33; i++){
            mvaddch(((app->y / 2) - 3), (((app->x / 2) - 16) + i), ACS_HLINE);
            mvaddch(((app->y / 2) + 2), (((app->x / 2) - 16) + i), ACS_HLINE);
        }
        /* Corners */
        mvaddch(((app->y / 2) - 3), ((app->x / 2) - 17), ACS_ULCORNER);
        mvaddch(((app->y / 2) - 3), ((app->x / 2) + 17), ACS_URCORNER);
        mvaddch(((app->y / 2) + 2), ((app->x / 2) - 17), ACS_LLCORNER);
        mvaddch(((app->y / 2) + 2), ((app->x / 2) + 17), ACS_LRCORNER);
        /* Sides */
        for(int i = 0; i < 4; i++){
            mvaddch((((app->y / 2) - 2) + i), ((app->x / 2) - 17), ACS_VLINE);
            mvaddch((((app->y / 2) - 2) + i), ((app->x / 2) + 17), ACS_VLINE);
        }

        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 16), "An unfinished cycle was detected!");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 16), "         Want to resume?         ");
        mvprintw(((app->y / 2) + 0), ((app->x / 2) - 16), "                                 ");
        if(app->menuPos == 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 1), ((app->x / 2) - 16), "      -> Yes <- ");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw(((app->y / 2) + 1), ((app->x / 2) - 16), "         Yes    ");
        }

        if(app->menuPos == 2){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 1), ((app->x / 2) + 1),  " -> No <-       ");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw(((app->y / 2) + 1), ((app->x / 2) + 1),  "    No          ");
        }
    }
}

/* Time the pomodoros */
void timer(appData * app){
    int sec = 60;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        if(app->pausedTimer != 1){
            app->timerms++;
            if(app->timerms >= 7.745966692){
                app->timerms = 0;
                /* Debug */
                //app->timer = app->timer - 60;
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
    if(app->needResume == 0){
        printLogo(app, ICONS);

        if(app->menuPos == 1 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 4), ((app->x / 2) - 5), "-> start <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw(((app->y / 2) + 4), ((app->x / 2) - 2), "start");
        }

        if(app->menuPos == 2 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 5), ((app->x / 2) - 8), "-> preferences <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw(((app->y / 2) + 5), ((app->x / 2) - 5), "preferences");
        }

        if(app->menuPos == 3 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 5), "-> leave <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 2), "leave");
        }
    }
    printResume(app);
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
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9), "<- work time %02dm ->", app->workTime / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 6), "work time %02dm", app->workTime / (60 * 8));
    }

    if(app->menuPos == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10), "<- short pause %02dm ->", app->shortPause / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2)), ((app->x / 2) - 7), "short pause %02dm", app->shortPause / (60 * 8));
    }
    
    if(app->menuPos == 4){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10), "<- long pause  %02dm ->", app->longPause / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 7), "long pause  %02dm", app->longPause / (60 * 8));
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
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 0), "[%02d minutes]", app->workTime / (60 * 8));
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 12), "🍅 Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 1), "[%02d minutes]", app->workTime / (60 * 8));
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), "Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->workTime / (60 * 8));
        }
    }
    else if(app->currentMode == 2){
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), " Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->shortPause / (60 * 8));
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 10), "☕ Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 1), "[%02d minutes]", app->shortPause / (60 * 8));
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 9), "Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 2), "[%02d minutes]", app->shortPause / (60 * 8));
        }
    }
    else{
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 12), " Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 1), "[%02d minutes]", app->longPause / (60 * 8));
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 13), "🌴 Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 2), "[%02d minutes]", app->longPause / (60 * 8));
        }
        else{
            mvprintw(((app->y / 2) + 6), ((app->x / 2) - 11), "Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw(((app->y / 2) + 6), ((app->x / 2) + 0), "[%02d minutes]", app->longPause / (60 * 8));
        }
    }
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw(((app->y / 2) + 7), ((app->x / 2) - 2), "%s:%s", minutes, seconds);
}

