/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// draw.c
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
#include <inttypes.h>

/* Print noise menu */
void printNoiseMenu(appData * app){
    if(NOISE == 1){
        if(app->playNoise == 0 && app->needResume != 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            if(strcmp(ICONS, "nerdicons") == 0){
                mvprintw( 1, 2, "󰖖 ");
                mvprintw( 2, 2, "󰈸 ");
                mvprintw( 3, 2, "󰖝 ");
                mvprintw( 4, 2, "󱐋 ");
            }
            else if(strcmp(ICONS, "iconson") == 0){
                mvprintw( 1, 2, "☔ ");
                mvprintw( 2, 2, "🔥 ");
                mvprintw( 3, 2, "🍃 ");
                mvprintw( 4, 2, "⚡ ");
            }
            else{
                mvprintw( 1, 2, "R ");
                mvprintw( 2, 2, "F ");
                mvprintw( 3, 2, "W ");
                mvprintw( 4, 2, "T ");
            }
        }

        if(app->playRainNoise == 1 && app->needResume != 1){
            setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
            if(strcmp(ICONS, "nerdicons") == 0) mvprintw( 1, 2, "󰖖 ");
            else if(strcmp(ICONS, "iconson") == 0) mvprintw( 1, 2, "☔ ");
            else mvprintw( 1, 2, "R ");

            uintmax_t rainVolume = strtoumax(app->rainVolume, NULL, 10);
            if(app->printVolume == 1){
                switch(rainVolume){
                    case 0:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 7, "▒▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 10:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- █");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 8, "▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 20:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ██");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 9, "▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 30:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ███");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 10, "▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 40:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 11, "▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 50:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- █████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 12, "▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 60:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ██████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 13, "▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 70:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ███████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 14, "▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 80:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 15, "▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 90:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- █████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 16, "▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    case 100:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 5,  "- ██████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 1, 17, " +");
                        break;
                    default:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 1, 5, "- ▒▒▒▒▒▒▒▒▒▒ +");
                        break;
                }
            }
        }
        if(app->playFireNoise == 1 && app->needResume != 1){
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            if(strcmp(ICONS, "nerdicons") == 0) mvprintw( 2, 2, "󰈸 ");
            else if(strcmp(ICONS, "iconson") == 0)  mvprintw( 2, 2, "🔥 ");
            else mvprintw( 2, 2, "F ");

            uintmax_t fireVolume = strtoumax(app->fireVolume, NULL, 10);
            if(app->printVolume == 2){
                switch(fireVolume){
                    case 0:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 7, "▒▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 10:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- █");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 8, "▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 20:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ██");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 9, "▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 30:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ███");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 10, "▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 40:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 11, "▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 50:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- █████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 12, "▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 60:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ██████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 13, "▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 70:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ███████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 14, "▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 80:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 15, "▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 90:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- █████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 2, 16, "▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    case 100:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 5,  "- ██████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 2, 17, " +");
                        break;
                    default:
                        mvprintw( 2, 5, "- ▒▒▒▒▒▒▒▒▒▒ +");
                        break;
                }
            }
        }
        if(app->playWindNoise == 1 && app->needResume != 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            if(strcmp(ICONS, "nerdicons") == 0) mvprintw( 3, 2, "󰖝 ");
            else if(strcmp(ICONS, "iconson") == 0)  mvprintw( 3, 2, "🍃 ");
            else mvprintw( 3, 2, "W ");

            uintmax_t windVolume = strtoumax(app->windVolume, NULL, 10);
            if(app->printVolume == 3){
                switch(windVolume){
                    case 0:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 7, "▒▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 10:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- █");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 8, "▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 20:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ██");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 9, "▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 30:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ███");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 10, "▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 40:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 11, "▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 50:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- █████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 12, "▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 60:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ██████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 13, "▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 70:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ███████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 14, "▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 80:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 15, "▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 90:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- █████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 3, 16, "▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    case 100:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 5,  "- ██████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 3, 17, " +");
                        break;
                    default:
                        mvprintw( 3, 5, "- ▒▒▒▒▒▒▒▒▒▒ +");
                        break;
                }
            }
        }
        if(app->playThunderNoise == 1 && app->needResume != 1){
            setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
            if(strcmp(ICONS, "nerdicons") == 0) mvprintw( 4, 2, "󱐋 ");
            else if(strcmp(ICONS, "iconson") == 0)  mvprintw( 4, 2, "⚡ ");
            else mvprintw( 4, 2, "T ");

            uintmax_t thunderVolume = strtoumax(app->thunderVolume, NULL, 10);
            if(app->printVolume == 4){
                switch(thunderVolume){
                    case 0:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 7, "▒▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 10:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- █");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 8, "▒▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 20:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ██");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 9, "▒▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 30:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ███");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 10, "▒▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 40:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 11, "▒▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 50:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- █████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 12, "▒▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 60:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ██████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 13, "▒▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 70:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ███████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 14, "▒▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 80:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 15, "▒▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 90:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- █████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
                        mvprintw( 4, 16, "▒");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    case 100:
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 5,  "- ██████████");
                        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
                        mvprintw( 4, 17, " +");
                        break;
                    default:
                        mvprintw( 4, 5, "- ▒▒▒▒▒▒▒▒▒▒ +");
                        break;
                }
            }
        }
    }
}

/* Print resume menu */
void printResume(appData * app){
    if(app->needResume == 1){
        printBanner(app);
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex - 16), "An unfinished cycle was detected!");
        mvprintw((app->middley - 1), (app->middlex - 16), "         Want to resume?         ");
        if(app->menuPos == 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 0), (app->middlex - 16), "      -> Yes <- ");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 0), (app->middlex - 16), "         Yes    ");
        }

        if(app->menuPos == 2){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 0), (app->middlex + 1),  " -> No <-       ");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 0), (app->middlex + 1),  "    No          ");
        }
    }
}

/* Print the pomodoro counter */
void printPomodoroCounter(appData * app){
    if(app->currentMode == 1)
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);

    mvprintw((app->middley - 7), (app->middlex + 10) ,"%02d", app->pomodoroCounter);
}

/* Print the pause indicator */
void printPauseIndicator(appData * app){
    if(app->currentMode == 1 && app->pausedTimer == 1)
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
    else if((app->currentMode == 2 && app->pausedTimer == 1) || (app->currentMode == 3 && app->pausedTimer == 1))
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
    
    if(strcmp(ICONS, "nerdicons") == 0){
        mvprintw((app->middley - 7), (app->middlex - 11) ," ");
    }
    else if(strcmp(ICONS, "iconson") == 0){
        mvprintw((app->middley - 7), (app->middlex - 11) ,"⏸️ ");
    }
    else{
        mvprintw((app->middley - 7), (app->middlex - 11) ,"P ");;
    }
}

/* Print the main menu */
void printMainMenu(appData * app){
    if(app->needResume == 0){
        printLogo(app);

        if(app->menuPos == 1 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 4), (app->middlex - 5), "-> start <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 4), (app->middlex - 2), "start");
        }

        if(app->menuPos == 2 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 5), (app->middlex - 8), "-> preferences <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 5), (app->middlex - 5), "preferences");
        }

        if(app->menuPos == 3 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex - 5), "-> leave <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 6), (app->middlex - 2), "leave");
        }
    }
}

/* Print the settings menu */
void printSettings(appData * app){
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 4), (app->middlex - 5), "preferences");
    if(app->menuPos == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex - 9), "<- pomodoros  %02d ->", app->pomodoros);
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 2), (app->middlex - 6), "pomodoros  %02d", app->pomodoros);
    }

    if(app->menuPos == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 1), (app->middlex - 9), "<- work time %02dm ->", app->workTime / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 1), (app->middlex - 6), "work time %02dm", app->workTime / (60 * 8));
    }

    if(app->menuPos == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley), (app->middlex - 10), "<- short pause %02dm ->", app->shortPause / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley), (app->middlex - 7), "short pause %02dm", app->shortPause / (60 * 8));
    }
    
    if(app->menuPos == 4){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 1), (app->middlex - 10), "<- long pause  %02dm ->", app->longPause / (60 * 8));
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley + 1), (app->middlex - 7), "long pause  %02dm", app->longPause / (60 * 8));
    }
    if(app->menuPos == 5){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 4), (app->middlex - 11), "-> back to main menu <-");
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley + 4), (app->middlex - 8), "back to main menu");
    }
}
/* Print the Timer */
void printTimer(appData * app){
    FILE *time;
    if(TIMERLOG == 1)
        time = fopen(app->timerFile, "w");

    int x = app->timer / 8;
    int div = x / 60;
    int mod = x % 60;

    char minutes[3] = {(div / 10) + '0', (div % 10) + '0', '\0'};
    char seconds[3] = {(mod / 10) + '0', (mod % 10) + '0', '\0'};

    if(app->currentMode == 1){
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw((app->middley + 6), (app->middlex - 11), " Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex + 0), "[%02d minutes]", app->workTime / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, " ");
                fprintf(time, " ");
            }
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw((app->middley + 6), (app->middlex - 12), "🍅 Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex + 1), "[%02d minutes]", app->workTime / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, "⏸️ ");
                fprintf(time, "🍅 ");
            }
        }
        else{
            mvprintw((app->middley + 6), (app->middlex - 10), "Pomodoro");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex - 1), "[%02d minutes]", app->workTime / (60 * 8));
            if(TIMERLOG == 1 && app->pausedTimer == 1)
                fprintf(time, "P ");
        }
    }
    else if(app->currentMode == 2){
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw((app->middley + 6), (app->middlex - 10), " Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex - 1), "[%02d minutes]", app->shortPause / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, " ");
                fprintf(time, " ");
            }
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw((app->middley + 6), (app->middlex - 10), "☕ Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex - 1), "[%02d minutes]", app->shortPause / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, "⏸️ ");
                fprintf(time, "☕ ");
            }
        }
        else{
            mvprintw((app->middley + 6), (app->middlex - 9), "Pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex - 2), "[%02d minutes]", app->shortPause / (60 * 8));
            if(TIMERLOG == 1 && app->pausedTimer == 1)
                fprintf(time, "P ");
        }
    }
    else{
        setColor(COLOR_CYAN, COLOR_BLACK, A_BOLD);
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw((app->middley + 6), (app->middlex - 12), " Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex + 1), "[%02d minutes]", app->longPause / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, " ");
                fprintf(time, " ");
            }
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw((app->middley + 6), (app->middlex - 13), "🌴 Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex + 2), "[%02d minutes]", app->longPause / (60 * 8));
            if(TIMERLOG == 1){
                if(app->pausedTimer == 1)
                    fprintf(time, "⏸️ ");
                fprintf(time, "🌴 ");
            }
        }
        else{
            mvprintw((app->middley + 6), (app->middlex - 11), "Long pause");
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 6), (app->middlex + 0), "[%02d minutes]", app->longPause / (60 * 8));
            if(TIMERLOG == 1 && app->pausedTimer == 1)
                fprintf(time, "P ");
        }
    }
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley + 7), (app->middlex - 2), "%s:%s", minutes, seconds);
    if(TIMERLOG == 1){
        fprintf(time, "%s:%s", minutes, seconds);
        fclose(time);
    }
}

