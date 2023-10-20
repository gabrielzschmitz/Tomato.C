/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// anim.c
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

/* Time the animations frames */
void frameTimer(appData * app){
    int sec = 60;
    clock_t end = clock() + sec * (CLOCKS_PER_SEC);
    if(clock() < end) {
        if(app->pausedTimer != 1){
            app->framems++;
            if(app->framems >= app->sfps){
                app->framems = 0;
                app->frameTimer = app->frameTimer + 1;
            }
        }
    }
}

/* Print the logo frames */
void printLogo(appData * app){
    if(strcmp(ICONS, "nerdicons") == 0){
        if (app->logoFrame == 0){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),      "     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 1){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |╱    \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 2){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |      ─   |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 3){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\     ╲   /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 4){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\    |    /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 5){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\   ╱     /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 6){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |   ─      |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else{
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /    ╲|     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |          |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
    }
    else if(strcmp(ICONS, "iconson") == 0){
        if (app->logoFrame == 0){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),      "     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 1){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |╱    \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 2){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o ─   |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 3){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\     ╲   /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 4){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\    |    /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 5){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\   ╱     /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 6){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |   ─ o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else{
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /    ╲|     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     o     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
    }
    else{
        if (app->logoFrame == 0){
           setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
           mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
           mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
           setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
           mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
           mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
           mvprintw((app->middley - 2), (app->middlex - 10),      "     \\         /     ");
           mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
           setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
           mvprintw((app->middley), (app->middlex - 10),    "___                 _");
           mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
           mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 1){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |/    \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 2){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0 -   |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 3){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\     \\   /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 4){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\    |    /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 5){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\   /     /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else if(app->logoFrame == 6){
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /     |     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |   - 0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
        else{
            setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 6), (app->middlex - 10),"       __\\W/__       ");
            mvprintw((app->middley - 5), (app->middlex - 10),"     .\'.-\'_\'-.\'.     ");
            setColor(COLOR_RED, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 4), (app->middlex - 10),"    /    \\|     \\    ");
            mvprintw((app->middley - 3), (app->middlex - 10),"    |     0     |    ");
            mvprintw((app->middley - 2), (app->middlex - 10),"     \\         /     ");
            mvprintw((app->middley - 1), (app->middlex - 10),"      \'-.___.-\'      ");
            setColor(COLOR_MAGENTA, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley), (app->middlex - 10),    "___                 _");
            mvprintw((app->middley + 1), (app->middlex - 10)," | _  _  _ |_ _    / ");
            mvprintw((app->middley + 2), (app->middlex - 10)," |(_)|||(_||_(_) . \\_");
        }
    }
}

/* Print the coffee frames */
void printCoffee(appData * app){
    if(app->coffeeFrame == 0){
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex - 3),"   ) )  ");
        mvprintw((app->middley - 1), (app->middlex - 3),"  ( (   ");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(app->middley, (app->middlex - 3),      "....... ");
        mvprintw((app->middley + 1), (app->middlex - 3),"|     |]");
        mvprintw((app->middley + 2), (app->middlex - 3),"\\     / ");
        mvprintw((app->middley + 3), (app->middlex - 3)," `---\'  ");
    }
    else{
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex - 3)," ( (    ");
        mvprintw((app->middley - 1), (app->middlex - 3),"  ) )   ");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw(app->middley, (app->middlex - 3),      "....... ");
        mvprintw((app->middley + 1), (app->middlex - 3),"|     |]");
        mvprintw((app->middley + 2), (app->middlex - 3),"\\     / ");
        mvprintw((app->middley + 3), (app->middlex - 3)," `---\'  ");
    }
}
 
/* Print the coffee machine frames */
void printMachine(appData * app){
    if(strcmp(ICONS, "iconsoff") == 0){
        if(app->machineFrame == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    -     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   -   |   ___    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
        else if(app->machineFrame == 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    I     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   -   |   ___    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
        else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    -     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   -   |   _|_    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
    }else{
        if(app->machineFrame == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    ¯     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   ¯   |   ___    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
        else if(app->machineFrame == 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    †     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   ¯   |   ___    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
        else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley - 5), (app->middlex - 9),"________._________ ");
            mvprintw((app->middley - 4), (app->middlex - 9),"|   _   |\\       / ");
            mvprintw((app->middley - 3), (app->middlex - 9),"|  |.|  | \\     /  ");
            mvprintw((app->middley - 2), (app->middlex - 9),"|  |.|  |__\\___/   ");
            mvprintw((app->middley - 1), (app->middlex - 9),"|  |.|  |    ¯     ");
            mvprintw((app->middley), (app->middlex - 9),    "|   ¯   |   _|_    ");
            mvprintw((app->middley + 1), (app->middlex - 9),"|_______|  \\___/_  ");
            mvprintw((app->middley + 2), (app->middlex - 9),"| _____ |  /~~~\\ \\ ");
            mvprintw((app->middley + 3), (app->middlex - 9),"||     ||__\\___/__ ");
            mvprintw((app->middley + 4), (app->middlex - 9),"||_____|          |");
            mvprintw((app->middley + 5), (app->middlex - 9),"|_________________|");
        }
    }
}

/* Print the beach frames */
void printBeach(appData * app){
    if(app->beachFrame == 0){
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 5), (app->middlex - 7), "|");
        mvprintw((app->middley - 4), (app->middlex - 9), "\\ _ /");
        mvprintw((app->middley - 3), (app->middlex - 11),"-= (_) =-");
        mvprintw((app->middley - 2), (app->middlex - 9), "/   \\");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex + 3), "_\\/_");
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 1), (app->middlex - 7), "|");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 1), (app->middlex + 3), "//o\\  _\\/_");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex - 12),"_ _ __ __ ____ _");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 5), "|");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 6), "__");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 9), "/o\\\\");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 1), (app->middlex - 12),"__=_-= _=_=-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 1), (app->middlex + 1), "_,-\'|\"\'\"\"-|-");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 2), (app->middlex - 12),"-=- -_=-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 2), (app->middlex - 3), "_,-\"          |");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 3), (app->middlex - 12),"=- -=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 3), (app->middlex - 7), ".--\"");
    }
    else{
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 4), (app->middlex - 9), "\\ | /");
        mvprintw((app->middley - 3), (app->middlex - 10),"- (_) -");
        mvprintw((app->middley - 2), (app->middlex - 9), "/ | \\");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 2), (app->middlex + 2), "_\\/_");
        setColor(COLOR_YELLOW, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 1), (app->middlex - 8), "     ");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 1), (app->middlex + 2), "//o\\  _\\/_");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex - 12),"__ ____ __  ____");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 5), "|");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 6), "_ ");
        setColor(COLOR_GREEN, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 8), "//o\\");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 0), (app->middlex + 12),"_");
        mvprintw((app->middley + 1), (app->middlex - 12),"--_=-__=  _-=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 1), (app->middlex + 1), "_,-\'|\"\'\"\"-|-");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 2), (app->middlex - 12),"_-= _=-_=");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 2), (app->middlex - 3) ,"_,-\"          |");
        setColor(COLOR_BLUE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 3), (app->middlex - 12),"-= _-");
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 3), (app->middlex - 7) ,".--\"");
    }
}

/* Print the gear frames */
void printWrench(appData * app, int flip){
    if(flip == 1){
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley + 5), (app->middlex - 15)," .----.                 .---.");
        mvprintw((app->middley + 6), (app->middlex - 15),"'---,  `._____________.'  _  `.");
        mvprintw((app->middley + 7), (app->middlex - 15),"     )   _____________   <_>  :");
        mvprintw((app->middley + 8), (app->middlex - 15),".---'  .'             `.     .'");
        mvprintw((app->middley + 9), (app->middlex - 15)," `----'                 `---'");
    }else{
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 9), (app->middlex - 15),"  .---.                 .----.");
        mvprintw((app->middley - 8), (app->middlex - 15),".`  _  '._____________.`  ,---'");
        mvprintw((app->middley - 7), (app->middlex - 15),":  >_<   _____________   )");
        mvprintw((app->middley - 6), (app->middlex - 15),"'.     .`             '.  '---.");
        mvprintw((app->middley - 5), (app->middlex - 15),"  '---`                 '----`");
    }
}

void printBanner(appData * app){
    setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    if(app->bannerFrame == 0){
        mvprintw((app->middley - 3), (app->middlex - 20),"    _________________________________    ");
        mvprintw((app->middley - 2), (app->middlex - 20),"___|");
        mvprintw((app->middley - 2), (app->middlex + 17),"|___");
        mvprintw((app->middley - 1), (app->middlex - 20),"\\  |                                 |  /");
        mvprintw((app->middley - 1), (app->middlex + 17),"|  /");
        mvprintw((app->middley + 0), (app->middlex - 20)," \\ |");
        mvprintw((app->middley + 0), (app->middlex + 17),"| / ");
        mvprintw((app->middley + 1), (app->middlex - 20)," < |");
        mvprintw((app->middley + 1), (app->middlex + 17),"| > ");
        mvprintw((app->middley + 2), (app->middlex - 20)," / |_________________________________| \\ ");
        mvprintw((app->middley + 3), (app->middlex - 20),"/______)                         (______\\");
    }
    else if(app->bannerFrame == 1){
        mvprintw((app->middley - 3), (app->middlex - 21),"     _________________________________     ");
        mvprintw((app->middley - 2), (app->middlex - 21),"____|");
        mvprintw((app->middley - 2), (app->middlex + 17),"|____");
        mvprintw((app->middley - 1), (app->middlex - 21),"\\   |");
        mvprintw((app->middley - 1), (app->middlex + 17),"|   /");
        mvprintw((app->middley + 0), (app->middlex - 21)," \\  |");
        mvprintw((app->middley + 0), (app->middlex + 17),"|  / ");
        mvprintw((app->middley + 1), (app->middlex - 21)," <  |");
        mvprintw((app->middley + 1), (app->middlex + 17),"|  > ");
        mvprintw((app->middley + 2), (app->middlex - 21)," /  |_________________________________|  \\ ");
        mvprintw((app->middley + 3), (app->middlex - 21),"/_______)                         (_______\\");
    }
    else if(app->bannerFrame == 2){
        mvprintw((app->middley - 3), (app->middlex - 23),"      ___________________________________      ");
        mvprintw((app->middley - 2), (app->middlex - 23),"_____|");
        mvprintw((app->middley - 2), (app->middlex + 18),"|_____");
        mvprintw((app->middley - 1), (app->middlex - 23),"\\    |");
        mvprintw((app->middley - 1), (app->middlex + 18),"|    /");
        mvprintw((app->middley + 0), (app->middlex - 23)," \\   |");
        mvprintw((app->middley + 0), (app->middlex + 18),"|   / ");
        mvprintw((app->middley + 1), (app->middlex - 23)," <   |");
        mvprintw((app->middley + 1), (app->middlex + 18),"|   > ");
        mvprintw((app->middley + 2), (app->middlex - 23)," /   |___________________________________|   \\ ");
        mvprintw((app->middley + 3), (app->middlex - 23),"/_______)                             (_______\\");
    }else{
        mvprintw((app->middley - 3), (app->middlex - 24),"       ___________________________________       ");
        mvprintw((app->middley - 2), (app->middlex - 24),"______|");
        mvprintw((app->middley - 2), (app->middlex + 18),"|______");
        mvprintw((app->middley - 1), (app->middlex - 24),"\\     |");
        mvprintw((app->middley - 1), (app->middlex + 18),"|     /");
        mvprintw((app->middley + 0), (app->middlex - 24)," \\    |");
        mvprintw((app->middley + 0), (app->middlex + 18),"|    / ");
        mvprintw((app->middley + 1), (app->middlex - 24)," <    |");
        mvprintw((app->middley + 1), (app->middlex + 18),"|    > ");
        mvprintw((app->middley + 2), (app->middlex - 24)," /    |___________________________________|    \\ ");
        mvprintw((app->middley + 3), (app->middlex - 24),"/________)                             (________\\");
    }
}

void printPergament(appData * app){
    if(app->helpFrame == 0){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 11), (app->middlex - 27),"  ________________________________________________");
        mvprintw((app->middley - 10), (app->middlex - 27)," / \\                                              \\");
        mvprintw((app->middley - 9 ), (app->middlex - 27),"|   |                                             |");
        mvprintw((app->middley - 8 ), (app->middlex - 27)," \\__|                                             /");

        printKeybinds(app, 0);

        mvprintw((app->middley - 7 ), (app->middlex - 27),"    \\     _______________________________________|_____");
        mvprintw((app->middley - 6 ), (app->middlex - 27),"     \\   /                                            /");
        mvprintw((app->middley - 5 ), (app->middlex - 27),"      \\_/____________________________________________/");
    }
    else if(app->helpFrame == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 11), (app->middlex - 27),"  ________________________________________________");
        mvprintw((app->middley - 10), (app->middlex - 27)," / \\                                              \\");
        mvprintw((app->middley - 9 ), (app->middlex - 27),"|   |                                             |");
        mvprintw((app->middley - 8 ), (app->middlex - 27)," \\__|                                             /");
        mvprintw((app->middley - 7 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 6 ), (app->middlex - 27),"    |");

        printKeybinds(app, 1);

        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 7 ), (app->middlex + 20),"  /");
        mvprintw((app->middley - 6 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 5 ), (app->middlex - 27),"    \\     _______________________________________|_____");
        mvprintw((app->middley - 4 ), (app->middlex - 27),"     \\   /                                            /");
        mvprintw((app->middley - 3 ), (app->middlex - 27),"      \\_/____________________________________________/");
    }
    else if(app->helpFrame == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 11), (app->middlex - 27),"  ________________________________________________");
        mvprintw((app->middley - 10), (app->middlex - 27)," / \\                                              \\");
        mvprintw((app->middley - 9 ), (app->middlex - 27),"|   |                                             |");
        mvprintw((app->middley - 8 ), (app->middlex - 27)," \\__|                                             /");
        mvprintw((app->middley - 7 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 6 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 5 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 4 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 3 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 2 ), (app->middlex - 27),"    |");

        printKeybinds(app, 2);

        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 7 ), (app->middlex + 20),"  /");
        mvprintw((app->middley - 6 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 5 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 4 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 3 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 2 ), (app->middlex + 20),"|");
        mvprintw((app->middley - 1 ), (app->middlex - 27),"    \\     ______________________________________|______");
        mvprintw((app->middley - 0 ), (app->middlex - 27),"     \\   /                                            /");
        mvprintw((app->middley + 1 ), (app->middlex - 27),"      \\_/____________________________________________/");
        
    }
    else if(app->helpFrame == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 11), (app->middlex - 27),"  ________________________________________________");
        mvprintw((app->middley - 10), (app->middlex - 27)," / \\                                              \\");
        mvprintw((app->middley - 9 ), (app->middlex - 27),"|   |                                             |");
        mvprintw((app->middley - 8 ), (app->middlex - 27)," \\__|                                             /");
        mvprintw((app->middley - 7 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 6 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 5 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 4 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 3 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 2 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 1 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 0 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 1 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 2 ), (app->middlex - 27),"    |");

        printKeybinds(app, 3);

        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 7 ), (app->middlex + 20),"  /");
        mvprintw((app->middley - 6 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 5 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 4 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 3 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 2 ), (app->middlex + 20),"|");
        mvprintw((app->middley - 1 ), (app->middlex + 20),"|");
        mvprintw((app->middley - 0 ), (app->middlex + 20),"|");
        mvprintw((app->middley + 1 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 2 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 3 ), (app->middlex - 27),"    \\     _______________________________________|_____");
        mvprintw((app->middley + 4 ), (app->middlex - 27),"     \\   /                                            /");
        mvprintw((app->middley + 5 ), (app->middlex - 27),"      \\_/____________________________________________/");
        
    }else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 11), (app->middlex - 27),"  ________________________________________________");
        mvprintw((app->middley - 10), (app->middlex - 27)," / \\                                              \\");
        mvprintw((app->middley - 9 ), (app->middlex - 27),"|   |                  HELP PAGE                  |");
        mvprintw((app->middley - 8 ), (app->middlex - 27)," \\__|                                             /");
        mvprintw((app->middley - 7 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 6 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 5 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 4 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 3 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 2 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 1 ), (app->middlex - 27),"    |");
        mvprintw((app->middley - 0 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 1 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 2 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 3 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 4 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 5 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 6 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 7 ), (app->middlex - 27),"    |");
        mvprintw((app->middley + 8 ), (app->middlex - 27),"    |");

        printKeybinds(app, 4);

        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
        mvprintw((app->middley - 7 ), (app->middlex + 20),"  /");
        mvprintw((app->middley - 6 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 5 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 4 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 3 ), (app->middlex + 20)," |");
        mvprintw((app->middley - 2 ), (app->middlex + 20),"|");
        mvprintw((app->middley - 1 ), (app->middlex + 20),"|");
        mvprintw((app->middley - 0 ), (app->middlex + 20),"|");
        mvprintw((app->middley + 1 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 2 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 3 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 4 ), (app->middlex + 20)," |");
        mvprintw((app->middley + 5 ), (app->middlex + 20),"  |");
        mvprintw((app->middley + 6 ), (app->middlex + 20),"  |");
        mvprintw((app->middley + 7 ), (app->middlex + 20),"   |");
        mvprintw((app->middley + 8 ), (app->middlex + 20),"   |");
        mvprintw((app->middley + 9 ), (app->middlex - 27),"    \\     _________________________________________|___");
        mvprintw((app->middley + 10), (app->middlex - 27),"     \\   /                                            /");
        mvprintw((app->middley + 11), (app->middlex - 27),"      \\_/____________________________________________/");
    }


}

/* Print the Notepad */
void printNotepad(appData * app){
    setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 11), (app->middlex - 18)," _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ");
    mvprintw((app->middley - 10), (app->middlex - 18),"(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(-(()");
    mvprintw((app->middley - 9 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 8 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 7 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 6 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 5 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 4 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 3 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 2 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 1 ), (app->middlex - 18),"|");
    mvprintw((app->middley - 0 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 1 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 2 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 3 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 4 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 5 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 6 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 7 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 8 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 9 ), (app->middlex - 18),"|");
    mvprintw((app->middley + 10), (app->middlex - 18),"|");

    printNotes(app);

    setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
    mvprintw((app->middley - 9 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 8 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 7 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 6 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 5 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 4 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 3 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 2 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 1 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley - 0 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 1 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 2 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 3 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 4 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 5 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 6 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 7 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 8 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 9 ), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 10), (app->middlex - 18 + 37),"||");
    mvprintw((app->middley + 11), (app->middlex - 18),"|____________________________________|/");
}

