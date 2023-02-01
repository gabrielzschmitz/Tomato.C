/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
//  tomato.c
*/
#include "config.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

/* Initialize variables */
char * initPath(void){
    /* Get timer file path from config.mk */
    char * timerFile = malloc(strlen(TIMERFILE) + 1);
    strcpy(timerFile, TIMERFILE);

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

/* Read log file and print the content */
int printLog(char * path){
    FILE *log;
    log = fopen(path, "r");
    char timer[1024]={0,};
    
    do {
        if(!log){
            perror("Couldn't read log file");
            return 1;
        }else{
            fgets(timer, sizeof timer, log);
            if(strstr(timer, "00:00"))
                puts("");
            else
                printf("%s", timer);
            break;
        }
    } while (0);

    fclose(log);
    return 0;
}

/* Putting it all together */
int main(void){
    /* Initializing the app */
    char * path = NULL;
    path = initPath();

    /* Print the timer file content */
    int status = printLog(path);

    /* Return exit status */
    return status;
}

