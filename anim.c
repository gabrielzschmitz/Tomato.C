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

/* Time the animations frames */
void frameTimer(appData * app){
    int sec = 30;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        if(app->pausedTimer != 1)
            app->frameTimer++;
    }
}

/* Print the logo frames */
void printLogo(appData * app){
    app->E = 'L';

    if (app->logoFrame == 0){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),      "     \\         /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 1){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |╱    \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\         /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 2){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤ ─   |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\         /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 3){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\     ╲   /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 4){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\    |    /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 5){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\   ╱     /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else if(app->logoFrame == 6){
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /     |     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |   ─ ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\         /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
    else{
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 10),"       __\\W/__       ");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 10),"     .\'.-\'_\'-.\'.     ");
        setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 10),"    /    ╲|     \\    ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"    |     ⬤     |    ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 10),"     \\         /     ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 10),"      \'-.___.-\'      ");
        setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2)), ((app->x / 2) - 10),    "___                 _");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 10)," | _  _  _ |_ _    / ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 10)," |(_)|||(_||_(_) . \\_");
    }
}

/* Print the coffee frames */
void printCoffee(appData * app){
    app->E = 'C';

    if(app->coffeeFrame == 0){
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 3),"   ) )  ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 3),"  ( (   ");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->y / 2), ((app->x / 2) - 3),      "....... ");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 3),"|     |]");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 3),"\\     / ");
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 3)," `---\'  ");
    }
    else{
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 3)," ( (    ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 3),"  ) )   ");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->y / 2), ((app->x / 2) - 3),      "....... ");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 3),"|     |]");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 3),"\\     / ");
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 3)," `---\'  ");
    }
}
 
/* Print the coffee machine frames */
void printMachine(appData * app){
    app->E = 'M';

    if(app->machineFrame == 0){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 9),"________._________ ");
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 9),"|   _   |\\       / ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 9),"|  |.|  | \\     /  ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9),"|  |.|  |__\\___/   ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9),"|  |.|  |    ¯     ");
        mvprintw(((app->y / 2)), ((app->x / 2) - 9),    "|   ¯   |   ___    ");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 9),"|_______|  \\___/_  ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 9),"| _____ |  /~~~\\ \\ ");
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 9),"||     ||__\\___/__ ");
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 9),"||_____|          |");
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 9),"|_________________|");
    }
    else if(app->machineFrame == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 9),"________._________ ");
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 9),"|   _   |\\       / ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 9),"|  |.|  | \\     /  ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9),"|  |.|  |__\\___/   ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9),"|  |.|  |    †     ");
        mvprintw(((app->y / 2)), ((app->x / 2) - 9),    "|   ¯   |   ___    ");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 9),"|_______|  \\___/_  ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 9),"| _____ |  /~~~\\ \\ ");
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 9),"||     ||__\\___/__ ");
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 9),"||_____|          |");
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 9),"|_________________|");
    }
    else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 9),"________._________ ");
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 9),"|   _   |\\       / ");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 9),"|  |.|  | \\     /  ");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9),"|  |.|  |__\\___/   ");
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 9),"|  |.|  |    ¯     ");
        mvprintw(((app->y / 2)), ((app->x / 2) - 9),    "|   ¯   |   _|_    ");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 9),"|_______|  \\___/_  ");
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 9),"| _____ |  /~~~\\ \\ ");
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 9),"||     ||__\\___/__ ");
        mvprintw(((app->y / 2) + 4), ((app->x / 2) - 9),"||_____|          |");
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 9),"|_________________|");
    }
}

/* Print the beach frames */
void printBeach(appData * app){
    app->E = 'B';

    if(app->beachFrame == 0){
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 7), "|");
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 9), "\\ _ /");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 11),"-= (_) =-");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9), "/   \\");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) + 3), "_\\/_");
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 7), "|");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) + 3), "//o\\  _\\/_");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) - 12),"_ _ __ __ ____ _");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 5), "|");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 6), "__");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 9), "/o\\\\");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 12),"__=_-= _=_=-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) + 1), "_,-\'|\"\'\"\"-|-");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 12),"-=- -_=-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 3), "_,-\"          |");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 12),"=- -=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 7), ".--\"");
    }
    else{
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 4), ((app->x / 2) - 9), "\\ | /");
        mvprintw(((app->y / 2) - 3), ((app->x / 2) - 10),"- (_) -");
        mvprintw(((app->y / 2) - 2), ((app->x / 2) - 9), "/ | \\");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 2), ((app->x / 2) + 2), "_\\/_");
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) - 8), "     ");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 1), ((app->x / 2) + 2), "//o\\  _\\/_");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) - 12),"__ ____ __  ____");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 5), "|");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 6), "_ ");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 8), "//o\\");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 0), ((app->x / 2) + 12),"_");
        mvprintw(((app->y / 2) + 1), ((app->x / 2) - 12),"--_=-__=  _-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 1), ((app->x / 2) + 1), "_,-\'|\"\'\"\"-|-");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 12),"_-= _=-_=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 2), ((app->x / 2) - 3) ,"_,-\"          |");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 12),"-= _-");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 3), ((app->x / 2) - 7) ,".--\"");
    }
}

/* Print the gear frames */
void printGear(appData * app, int flip){
    app->E = 'G';

    if(flip == 1){
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) + 5), ((app->x / 2) - 15)," .----.                 .---.");
        mvprintw(((app->y / 2) + 6), ((app->x / 2) - 15),"'---,  `._____________.'  _  `.");
        mvprintw(((app->y / 2) + 7), ((app->x / 2) - 15),"     )   _____________   <_>  :");
        mvprintw(((app->y / 2) + 8), ((app->x / 2) - 15),".---'  .'             `.     .'");
        mvprintw(((app->y / 2) + 9), ((app->x / 2) - 15)," `----'                 `---'");
    }else{
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw(((app->y / 2) - 9), ((app->x / 2) - 15),"  .---.                 .----.");
        mvprintw(((app->y / 2) - 8), ((app->x / 2) - 15),".`  _  '._____________.`  ,---'");
        mvprintw(((app->y / 2) - 7), ((app->x / 2) - 15),":  >_<   _____________   )");
        mvprintw(((app->y / 2) - 6), ((app->x / 2) - 15),"'.     .`             '.  '---.");
        mvprintw(((app->y / 2) - 5), ((app->x / 2) - 15),"  '---`                 '----`");
    }
}

