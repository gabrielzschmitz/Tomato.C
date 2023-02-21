/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// tomato.h
*/
#ifndef TOMATO_H_
#define TOMATO_H_
#include <ncurses.h>
#include <mpv/client.h>

/* Defining the app struct */
typedef struct appData appData;
struct appData{
    int currentPID;
    int rainNoisePID;
    int fireNoisePID;
    int windNoisePID;
    int thunderNoisePID;
    int runRainOnce;
    int runFireOnce;
    int runWindOnce;
    int runThunderOnce;
    int playNoise;
    int playRainNoise;
    int playFireNoise;
    int playWindNoise;
    int playThunderNoise;
    int printVolume;
    int printRainVolume;
    int printFireVolume;
    int printWindVolume;
    int printThunderVolume;
    char rainVolume[4];
    char fireVolume[4];
    char windVolume[4];
    char thunderVolume[4];
    char * mpvCmd;
    char * logPrefix;
    char * logFile;
    char * tmpFile;
    char * timerFile;
    int pomodorosLevels;
    int workTimeLevels;
    int shortPauseLevels;
    int longPauseLevels;
    int pomodoros;
    int workTime;
    int shortPause;
    int longPause;
    int menuPos;
    int pomodoroCounter;
    int currentMode;
    int needMainMenu;
    int frameTimer;
    int timer;
    double sfps;
    int framems;
    int timerms;
    int logoFrame;
    int coffeeFrame;
    int machineFrame;
    int beachFrame;
    int bannerFrame;
    int userInput;
    int pausedTimer;
    int cycles;
    int needToLog;
    int unfinishedPomodoroCounter;
    int unfinishedPomodoros;
    int unfinishedTimer;
    int unfinishedFullTimer;
    int needResume;
    int resume;
    int newDay;
    int runOnce; 
    char date[50];
    int middlex;
    int middley;
    int x;
    int y;
};

/* Initialize variables */
extern void initApp(appData * );

/* Update variables */
extern void doUpdate(appData * );

/* Print at screen */
extern void drawScreen(appData * );

/* Putting it all together */
extern int main(int , char *[]);

#endif

