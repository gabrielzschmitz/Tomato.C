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
#include <sys/stat.h>
#include <inttypes.h>

/* Function to create a new note */
note *createNote(char type){
    note *newNote = (note *)malloc(sizeof(note));
    newNote->note = NULL;
    newNote->type = type;
    return newNote;
}

/* Function to create a new notepad */
notepad *createNotepad(notepad *notes){
    notes = (notepad *)malloc(sizeof(notepad));
    return notes;
}

/* Initialize screen with colors, enabled keyboard and another little configs */
void initScreen(void){
#ifdef XCURSES
    Xinitscr(argc, argv);
#else
    initscr();
#endif
    if (has_colors()){
        if(BGTRANSPARENCY == 1)
            use_default_colors();
        int realbg;
        int bg = 0, fg = 0;
        start_color();
        for(bg = COLOR_BLACK; bg <= COLOR_WHITE; bg++)
            for(fg = COLOR_BLACK; fg <= COLOR_WHITE; fg++){
                if(BGTRANSPARENCY == 1)
                    realbg = -1;
                else
                    realbg = bg;
                init_pair(bg*PALLETE_SIZE + fg + 1, fg, realbg);
            }
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
    app->middlex = app->x >> 1;
    app->middley = app->y >> 1;
}

/* Set the log folder and files */
void createLog(appData * app){
    /* Get /home/user */
    char * home = getenv("HOME");
    int homeLen = strlen(home);

    /* Set log folder fullpath */
    char * logPrefix = NULL;
    logPrefix = malloc(homeLen + sizeof(char) + strlen(app->logPrefix) + 1);
    strcpy(logPrefix, home);
    strcat(logPrefix, "/");
    strcat(logPrefix, app->logPrefix);
    app->logPrefix = logPrefix;

    /* Set log file fullpath */
    char * logFile = NULL;
    logFile = malloc(homeLen + sizeof(char) + strlen(app->logFile) + 1);
    strcpy(logFile, home);
    strcat(logFile, "/");
    strcat(logFile, app->logFile);
    app->logFile = logFile;

    /* Set tmp file fullpath */
    char * tmpFile = NULL;
    tmpFile = malloc(homeLen + sizeof(char) + strlen(app->tmpFile) + 1);
    strcpy(tmpFile, home);
    strcat(tmpFile, "/");
    strcat(tmpFile, app->tmpFile);
    app->tmpFile = tmpFile;
    
    /* Set timer file fullpath */
    char * timerFile = NULL;
    timerFile = malloc(homeLen + sizeof(char) + strlen(app->timerFile) + 1);
    strcpy(timerFile, home);
    strcat(timerFile, "/");
    strcat(timerFile, app->timerFile);
    app->timerFile = timerFile;
    
    /* Set notepad file fullpath */
    char * notepadFile = NULL;
    notepadFile = malloc(homeLen + sizeof(char) + strlen(app->notepadFile) + 1);
    strcpy(notepadFile, home);
    strcat(notepadFile, "/");
    strcat(notepadFile, app->notepadFile);
    app->notepadFile = notepadFile;
    
    /* Create log folder */
    mkdir(app->logPrefix, 0766);

    /* Create log file if doesn't exist */
    FILE * log;
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

    /* Get unfinished variables */
    sscanf(lastline, "%d/%d WT %d D %d %*d %*d",
           &app->unfinishedPomodoroCounter, &app->unfinishedPomodoros,
           &app->unfinishedTimer, &app->unfinishedFullTimer);
    sscanf(lastline, "%d/%d SP %d D %*d %d %*d",
           &app->unfinishedPomodoroCounter, &app->unfinishedPomodoros,
           &app->unfinishedTimer, &app->unfinishedFullTimer);
    sscanf(lastline, "%d/%d LP %d D %*d %*d %d",
           &app->unfinishedPomodoroCounter, &app->unfinishedPomodoros,
           &app->unfinishedTimer, &app->unfinishedFullTimer);
    app->unfinishedTimer = app->unfinishedTimer / (60 * 8);
    app->unfinishedFullTimer = app->unfinishedFullTimer / (60 * 8);

    fclose(log);
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
              &app->pomodoroCounter, &app->pomodoros, &app->timer,
              &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->workTime) * -1;
        app->frameTimer = 0;
        app->lastMode = app->currentMode;
        app->currentMode = 1;
        notify("worktime");
    }
    else if(sscanf(lastline, "%d/%d SP %d D %d %d %d",
                   &app->pomodoroCounter, &app->pomodoros, &app->timer,
                   &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->shortPause) * -1;
        app->frameTimer = 0;
        app->lastMode = app->currentMode;
        app->currentMode = 2;
        notify("shortpause");
    }
    else if(sscanf(lastline, "%d/%d LP %d D %d %d %d",
                   &app->pomodoroCounter, &app->pomodoros, &app->timer,
                   &app->workTime, &app->shortPause, &app->longPause) == 6){
        app->timer = (app->timer - app->longPause) * -1;
        app->frameTimer = 0;
        app->lastMode = app->currentMode;
        app->currentMode = 3;
        notify("longpause");
    }

    fclose(log);
    deleteLastLog(app);
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

/* Read notes in the notepad log file */
void readNotepad(appData * app){
    FILE *log;
    log = fopen(app->notepadFile, "r");
    if(!log){
        log = fopen(app->logFile, "w");
        fclose(log);
        return;
    }
    
    char line[1024];
    while(fgets(line, sizeof(line), log)){
        app->notes.lines[app->notesAmount] = createNote('-');
        app->notes.lines[app->notesAmount]->note = (char *)malloc(sizeof(char) * MAXINPUTLENGTH + 1);
        char *note = (char *)malloc(sizeof(char) * MAXINPUTLENGTH + 1);
        if(sscanf(line, "[x] %s", note))
            app->notes.lines[app->notesAmount]->type = 'x';
        if(sscanf(line, "[ ] %s", note))
            app->notes.lines[app->notesAmount]->type = 'o';
        if(sscanf(line, " -  %s", note))
            app->notes.lines[app->notesAmount]->type = '-';
        strcpy(app->notes.lines[app->notesAmount]->note, note);
        app->notesAmount += 1;
        app->emptyNotepad = 0;
    }
    fclose(log);
}

/* Write current notes in the notepad log file */
void writeToNotepad(appData * app){
    FILE *log;
    log = fopen(app->notepadFile, "w");
    
    int i = 0;
    while(i < app->notesAmount){
        if(app->notes.lines[i]->type == '-')
            fprintf(log, " -  ");
        else if(app->notes.lines[i]->type == 'o')
            fprintf(log, "[ ] ");
        else
            fprintf(log, "[%c] ", app->notes.lines[i]->type);
        fprintf(log, "%s\n", app->notes.lines[i]->note);
        i++;
    }

    fclose(log);
}

/* Write current infos in the log file */
void writeToLog(appData * app){
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
        default:
            break;
    }
    fclose(log);
}

/* End the timer log file */
void endTimerLog(appData * app){
    FILE *time;
    time = fopen(app->timerFile, "w");
    fprintf(time, "00:00");
    fclose(time);
}

/* Time the pomodoros */
void timer(appData * app){
    int sec = 60;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        if(app->pausedTimer != 1){
            app->timerms++;
            if(app->timerms >= app->sfps){
                app->timerms = 0;
                /* Debug */
                app->timer = app->timer - 60;
                //app->timer = app->timer - 1;
            }
        }
    }
}

/* Initialize timer log paths */
char * initTimerPath(const char * timerFile){
    /* Get /home/user */
    char * home = getenv("HOME");
    int homeLen = strlen(home);

    /* Set timer file fullpath */
    char * path = NULL;
    path = malloc(homeLen + sizeof(char) + strlen(timerFile) + 1);
    strcpy(path, home);
    strcat(path, "/");
    strcat(path, timerFile);

    return path;
}

/* Read timer log and print the content */
int printTimerLog(const char * path){
    FILE *log;
    log = fopen(path, "r");
    char timer[32]={0,};
    
    if(!log){
        perror("Couldn't read log file");
        return 1;
    }else{
        fgets(timer, sizeof timer, log);
        if(strstr(timer, "00:00"))
            puts("");
        else
            printf("%s", timer);
    }

    fclose(log);
    return 0;
}

/* Putting it all timer log functions together */
int tomatoTimer(const char * timerFile){
    /* Initializing the app */
    char * path = NULL;
    path = initTimerPath(timerFile);

    /* Print the timer file content */
    int status = printTimerLog(path);

    /* Return exit status */
    return status;
}

void toggleNoise(appData * app, int noise){
    if(noise == 1){
        if(app->playRainNoise == 0){
            app->playRainNoise = 1;
            if(app->runRainOnce == 0){
                char * rainnoisecmd[] = {
                    "/usr/local/share/tomato/sounds/ambience-rain.wav",
                    app->rainVolume, "tomato noise rain",
                    NULL};
                app->rainNoisePID = fork();
                app->runRainOnce = 1;
                if(app->rainNoisePID == 0)
                    execv(TOMATONOISE, rainnoisecmd);
            }
            else{
                FILE *tmpfile;
                tmpfile = fopen("/tmp/tomato_rain_state", "w");
                fprintf(tmpfile, "%s ON", app->rainVolume);
                fclose(tmpfile);
            }
        }
        else if(app->playRainNoise == 1){
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_rain_state", "w");
            fprintf(tmpfile, "%s ON", "0");
            fclose(tmpfile);
            app->playRainNoise = 0;
        }
    }
    else if(noise == 2){
        if(app->playFireNoise == 0){
            app->playFireNoise = 1;
            if(app->runFireOnce == 0){
                char * firenoisecmd[] = {
                    "/usr/local/share/tomato/sounds/ambience-fire.wav",
                    app->fireVolume, "tomato noise fire",
                    NULL};
                app->fireNoisePID = fork();
                app->runFireOnce = 1;
                if(app->fireNoisePID == 0)
                    execv(TOMATONOISE, firenoisecmd);
            }
            else{
                FILE *tmpfile;
                tmpfile = fopen("/tmp/tomato_fire_state", "w");
                fprintf(tmpfile, "%s ON", app->fireVolume);
                fclose(tmpfile);
            }
        }
        else if(app->playFireNoise == 1){
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_fire_state", "w");
            fprintf(tmpfile, "%s ON", "0");
            fclose(tmpfile);
            app->playFireNoise = 0;
        }
    }
    else if(noise == 3){
        if(app->playWindNoise == 0){
            app->playWindNoise = 1;
            if(app->runWindOnce == 0){
                char * windnoisecmd[] = {
                    "/usr/local/share/tomato/sounds/ambience-wind.wav",
                    app->windVolume, "tomato noise wind",
                    NULL};
                app->windNoisePID = fork();
                app->runWindOnce = 1;
                if(app->windNoisePID == 0)
                    execv(TOMATONOISE, windnoisecmd);
            }
            else{
                FILE *tmpfile;
                tmpfile = fopen("/tmp/tomato_wind_state", "w");
                fprintf(tmpfile, "%s ON", app->windVolume);
                fclose(tmpfile);
            }
        }
        else if(app->playWindNoise == 1){
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_wind_state", "w");
            fprintf(tmpfile, "%s ON", "0");
            fclose(tmpfile);
            app->playWindNoise = 0;
        }
    }
    else if(noise == 4){
        if(app->playThunderNoise == 0){
            app->playThunderNoise = 1;
            if(app->runThunderOnce == 0){
                char * thundernoisecmd[] = {
                    "/usr/local/share/tomato/sounds/ambience-thunder.wav",
                    app->thunderVolume, "tomato noise thunder",
                    NULL};
                app->thunderNoisePID = fork();
                app->runThunderOnce = 1;
                if(app->thunderNoisePID == 0)
                    execv(TOMATONOISE, thundernoisecmd);
            }
            else{
                FILE *tmpfile;
                tmpfile = fopen("/tmp/tomato_thunder_state", "w");
                fprintf(tmpfile, "%s ON", app->thunderVolume);
                fclose(tmpfile);
            }
        }
        else if(app->playThunderNoise == 1){
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_thunder_state", "w");
            fprintf(tmpfile, "%s ON", "0");
            fclose(tmpfile);
            app->playThunderNoise = 0;
        }
    }
}

void killNoise(void){
    FILE *tmpfile;

    tmpfile = fopen("/tmp/tomato_rain_state", "w");
    fprintf(tmpfile, "%d OFF", RAINVOLUME);
    fclose(tmpfile);

    tmpfile = fopen("/tmp/tomato_fire_state", "w");
    fprintf(tmpfile, "%d OFF", FIREVOLUME);
    fclose(tmpfile);

    tmpfile = fopen("/tmp/tomato_wind_state", "w");
    fprintf(tmpfile, "%d OFF", WINDVOLUME);
    fclose(tmpfile);

    tmpfile = fopen("/tmp/tomato_thunder_state", "w");
    fprintf(tmpfile, "%d OFF", THUNDERVOLUME);
    fclose(tmpfile);
}

void controlVolumeNoise(appData * app, int noise, char sign){
    if(noise == 1){
        if(sign == '-'){
            uintmax_t rainVolume = strtoumax(app->rainVolume, NULL, 10);
            if(rainVolume > 0)
                sprintf(app->rainVolume, "%lu", (rainVolume - 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_rain_state", "w");
            fprintf(tmpfile, "%s ON", app->rainVolume);
            fclose(tmpfile);
        }
        else if(sign == '+'){
            uintmax_t rainVolume = strtoumax(app->rainVolume, NULL, 10);
            if(rainVolume < 100)
                sprintf(app->rainVolume, "%lu", (rainVolume + 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_rain_state", "w");
            fprintf(tmpfile, "%s ON", app->rainVolume);
            fclose(tmpfile);
        }
    }
    else if(noise == 2){
        if(sign == '-'){
            uintmax_t fireVolume = strtoumax(app->fireVolume, NULL, 10);
            if(fireVolume > 0)
                sprintf(app->fireVolume, "%lu", (fireVolume - 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_fire_state", "w");
            fprintf(tmpfile, "%s ON", app->fireVolume);
            fclose(tmpfile);
        }
        else if(sign == '+'){
            uintmax_t fireVolume = strtoumax(app->fireVolume, NULL, 10);
            if(fireVolume < 100)
                sprintf(app->fireVolume, "%lu", (fireVolume + 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_fire_state", "w");
            fprintf(tmpfile, "%s ON", app->fireVolume);
            fclose(tmpfile);
        }
    }
    else if(noise == 3){
        if(sign == '-'){
            uintmax_t windVolume = strtoumax(app->windVolume, NULL, 10);
            if(windVolume > 0)
                sprintf(app->windVolume, "%lu", (windVolume - 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_wind_state", "w");
            fprintf(tmpfile, "%s ON", app->windVolume);
            fclose(tmpfile);
        }
        else if(sign == '+'){
            uintmax_t windVolume = strtoumax(app->windVolume, NULL, 10);
            if(windVolume < 100)
                sprintf(app->windVolume, "%lu", (windVolume + 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_wind_state", "w");
            fprintf(tmpfile, "%s ON", app->windVolume);
            fclose(tmpfile);
        }
    }
    else if(noise == 4){
        if(sign == '-'){
            uintmax_t thunderVolume = strtoumax(app->thunderVolume, NULL, 10);
            if(thunderVolume > 0)
                sprintf(app->thunderVolume, "%lu", (thunderVolume - 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_thunder_state", "w");
            fprintf(tmpfile, "%s ON", app->thunderVolume);
            fclose(tmpfile);
        }
        else if(sign == '+'){
            uintmax_t thunderVolume = strtoumax(app->thunderVolume, NULL, 10);
            if(thunderVolume < 100)
                sprintf(app->thunderVolume, "%lu", (thunderVolume + 10));
            FILE *tmpfile;
            tmpfile = fopen("/tmp/tomato_thunder_state", "w");
            fprintf(tmpfile, "%s ON", app->thunderVolume);
            fclose(tmpfile);
        }
    }
}

