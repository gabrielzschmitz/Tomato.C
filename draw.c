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

/* Print keybinds */
void printKeybinds(appData * app, int frame){
    if(frame == 0){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex - 22),"edit note:              ");

        setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex + 3),"e               ");
    }
    else if(frame == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex - 22),"check task:             ");
        mvprintw((app->middley - 9 ), (app->middlex - 21),"toggle notepad:         ");
        mvprintw((app->middley - 8 ), (app->middlex - 21),"quit:                   ");
        mvprintw((app->middley - 7 ), (app->middlex - 21),"add note/task:          ");
        mvprintw((app->middley - 6 ), (app->middlex - 21),"edit note:              ");

        setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex + 3),"space           ");
        mvprintw((app->middley - 9 ), (app->middlex + 4),"n               ");
        mvprintw((app->middley - 8 ), (app->middlex + 4),"q, esc          ");
        mvprintw((app->middley - 7 ), (app->middlex + 4),"a/A             ");
        mvprintw((app->middley - 6 ), (app->middlex + 4),"e               ");
    }
    else if(frame == 2){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex - 22),"return to main menu:    ");
        mvprintw((app->middley - 9 ), (app->middlex - 21),"toggle pause:           ");
        mvprintw((app->middley - 8 ), (app->middlex - 21),"choose option:          ");
        mvprintw((app->middley - 7 ), (app->middlex - 21),"toggle help page:       ");
        mvprintw((app->middley - 6 ), (app->middlex - 21),"check task:             ");
        mvprintw((app->middley - 5 ), (app->middlex - 21),"toggle notepad:         ");
        mvprintw((app->middley - 4 ), (app->middlex - 21),"quit:                   ");
        mvprintw((app->middley - 3 ), (app->middlex - 21),"add note/task:          ");
        mvprintw((app->middley - 2 ), (app->middlex - 21),"edit note:              ");
        
        setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex + 3),"ctrl+x, esc, q  ");
        mvprintw((app->middley - 9 ), (app->middlex + 4),"p, ctrl+p       ");
        mvprintw((app->middley - 8 ), (app->middlex + 4),"enter           ");
        mvprintw((app->middley - 7 ), (app->middlex + 4),"?, H            ");
        mvprintw((app->middley - 6 ), (app->middlex + 4),"space           ");
        mvprintw((app->middley - 5 ), (app->middlex + 4),"n               ");
        mvprintw((app->middley - 4 ), (app->middlex + 4),"q, esc          ");
        mvprintw((app->middley - 3 ), (app->middlex + 4),"a/A             ");
        mvprintw((app->middley - 2 ), (app->middlex + 4),"e               ");
    }
    else if(frame == 3){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex - 22),"decrease noise volume:  ");
        mvprintw((app->middley - 9 ), (app->middlex - 21),"increase noise volume:  ");
        mvprintw((app->middley - 8 ), (app->middlex - 21),"manage noise volume:    ");
        mvprintw((app->middley - 7 ), (app->middlex - 21),"delete note/task:       ");
        mvprintw((app->middley - 6 ), (app->middlex - 21),"return to main menu:    ");
        mvprintw((app->middley - 5 ), (app->middlex - 21),"toggle pause:           ");
        mvprintw((app->middley - 4 ), (app->middlex - 21),"choose option:          ");
        mvprintw((app->middley - 3 ), (app->middlex - 21),"toggle help page:       ");
        mvprintw((app->middley - 2 ), (app->middlex - 21),"check task:             ");
        mvprintw((app->middley - 1 ), (app->middlex - 21),"toggle notepad:         ");
        mvprintw((app->middley - 0 ), (app->middlex - 21),"quit:                   ");
        mvprintw((app->middley + 1 ), (app->middlex - 21),"add note/task:          ");
        mvprintw((app->middley + 2 ), (app->middlex - 21),"edit note:              ");
        
        setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 10), (app->middlex + 3),"ctrl+(r f w t)  ");
        mvprintw((app->middley - 9 ), (app->middlex + 4),"shift+(r f w t) ");
        mvprintw((app->middley - 8 ), (app->middlex + 4),"scroll          ");
        mvprintw((app->middley - 7 ), (app->middlex + 4),"dd, D, ctrl+d   ");
        mvprintw((app->middley - 6 ), (app->middlex + 4),"ctrl+x, esc, q  ");
        mvprintw((app->middley - 5 ), (app->middlex + 4),"p, ctrl+p       ");
        mvprintw((app->middley - 4 ), (app->middlex + 4),"enter           ");
        mvprintw((app->middley - 3 ), (app->middlex + 4),"?, H            ");
        mvprintw((app->middley - 2 ), (app->middlex + 4),"space           ");
        mvprintw((app->middley - 1 ), (app->middlex + 4),"n               ");
        mvprintw((app->middley - 0 ), (app->middlex + 4),"q, esc          ");
        mvprintw((app->middley + 1 ), (app->middlex + 4),"a/A             ");
        mvprintw((app->middley + 2 ), (app->middlex + 4),"e               ");
    }
    else{
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 7 ), (app->middlex - 21),"cancel adding note/task:");
        mvprintw((app->middley - 6 ), (app->middlex - 21),"toggle noise:           ");
        mvprintw((app->middley - 5 ), (app->middlex - 21),"move and select:        ");
        mvprintw((app->middley - 4 ), (app->middlex - 21),"decrease noise volume:  ");
        mvprintw((app->middley - 3 ), (app->middlex - 21),"increase noise volume:  ");
        mvprintw((app->middley - 2 ), (app->middlex - 21),"manage noise volume:    ");
        mvprintw((app->middley - 1 ), (app->middlex - 21),"delete note/task:       ");
        mvprintw((app->middley - 0 ), (app->middlex - 21),"return to main menu:    ");
        mvprintw((app->middley + 1 ), (app->middlex - 21),"toggle pause:           ");
        mvprintw((app->middley + 2 ), (app->middlex - 21),"choose option:          ");
        mvprintw((app->middley + 3 ), (app->middlex - 21),"toggle help page:       ");
        mvprintw((app->middley + 4 ), (app->middlex - 21),"check task:             ");
        mvprintw((app->middley + 5 ), (app->middlex - 21),"toggle notepad:         ");
        mvprintw((app->middley + 6 ), (app->middlex - 21),"quit:                   ");
        mvprintw((app->middley + 7 ), (app->middlex - 21),"add note/task:          ");
        mvprintw((app->middley + 8 ), (app->middlex - 21),"edit note:              ");
        
        setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
        mvprintw((app->middley - 7 ), (app->middlex + 4),"esc             ");
        mvprintw((app->middley - 6 ), (app->middlex + 4),"r f w t, 1 2 3 4");
        mvprintw((app->middley - 5 ), (app->middlex + 4),"arrows, vim keys");
        mvprintw((app->middley - 4 ), (app->middlex + 4),"ctrl+(r f w t)  ");
        mvprintw((app->middley - 3 ), (app->middlex + 4),"shift+(r f w t) ");
        mvprintw((app->middley - 2 ), (app->middlex + 4),"scroll          ");
        mvprintw((app->middley - 1 ), (app->middlex + 4),"dd, D, ctrl+d   ");
        mvprintw((app->middley - 0 ), (app->middlex + 4),"ctrl+x, esc, q  ");
        mvprintw((app->middley + 1 ), (app->middlex + 4),"p, ctrl+p       ");
        mvprintw((app->middley + 2 ), (app->middlex + 4),"enter           ");
        mvprintw((app->middley + 3 ), (app->middlex + 4),"?, H            ");
        mvprintw((app->middley + 4 ), (app->middlex + 4),"space           ");
        mvprintw((app->middley + 5 ), (app->middlex + 4),"n               ");
        mvprintw((app->middley + 6 ), (app->middlex + 4),"q, esc          ");
        mvprintw((app->middley + 7 ), (app->middlex + 4),"a/A             ");
        mvprintw((app->middley + 8 ), (app->middlex + 4),"e               ");
    }
}

/* Print cursor */
void printCursor(appData * app){
    setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
    if(app->editingNote == 1 || app->editingTask == 1 || app->addingNote == 1 || app->addingTask == 1)
        mvprintw((app->middley - 9) + app->cursory, (app->middlex - 17 + 4) + app->cursorx, "▏");
    else
        mvprintw((app->middley - 9) + app->cursory, (app->middlex - 17 + 4) + app->cursorx, "█");
}

/* Print part of the notes */
void printPartialNotes(appData * app, int max){
    if(app->emptyNotepad != 1 && app->notesAmount >= max){
        for(int i = max; i < app->notesAmount; i++){
            if(i == app->currentNote)
                setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            else
                setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);

            if(app->notes.lines[i]->type == '-'){
                mvprintw((app->middley - 9) + i, (app->middlex - 17+ 1), "%c", app->notes.lines[i]->type);
                for(int j = 0; j < (int) strlen(app->notes.lines[i]->note); j++){
                    if(j < app->cursorx || i != app->currentNote)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else if(app->addingNote == 1 || app->addingTask == 1)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j + 1, "%c", app->notes.lines[i]->note[j]);
                }
            }
            else{
                if(app->notes.lines[i]->type == 'x')
                    mvprintw((app->middley - 9) + i, (app->middlex - 17), "[X]");
                else
                    mvprintw((app->middley - 9) + i, (app->middlex - 17), "[ ]");
                for(int j = 0; j < (int) strlen(app->notes.lines[i]->note); j++){
                    if(j < app->cursorx || i != app->currentNote)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else if(app->addingNote == 1 || app->addingTask == 1)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j + 1, "%c", app->notes.lines[i]->note[j]);
                }
            }
        }
    }
}

/* Print notes */
void printNotes(appData * app){
    if(app->emptyNotepad != 1){
        for(int i = 0; i < app->notesAmount; i++){
            if(i == app->currentNote)
                setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            else
                setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);

            if(app->notes.lines[i]->type == '-'){
                mvprintw((app->middley - 9) + i, (app->middlex - 17+ 1), "%c", app->notes.lines[i]->type);
                for(int j = 0; j < (int) strlen(app->notes.lines[i]->note); j++){
                    if(j < app->cursorx || i != app->currentNote)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else if(app->addingNote == 1 || app->addingTask == 1)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j + 1, "%c", app->notes.lines[i]->note[j]);
                }
            }
            else{
                if(app->notes.lines[i]->type == 'x')
                    mvprintw((app->middley - 9) + i, (app->middlex - 17), "[X]");
                else
                    mvprintw((app->middley - 9) + i, (app->middlex - 17), "[ ]");
                for(int j = 0; j < (int) strlen(app->notes.lines[i]->note); j++){
                    if(j < app->cursorx || i != app->currentNote)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else if(app->addingNote == 1 || app->addingTask == 1)
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j, "%c", app->notes.lines[i]->note[j]);
                    else
                        mvprintw((app->middley - 9) + i, (app->middlex - 17 + 4) + j + 1, "%c", app->notes.lines[i]->note[j]);
                }
            }
        }
    }

    setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
    if(app->addingNote == 1){
        if(app->inputLength == 0)
            mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4), "▏");
        mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 1), "%c", app->notes.lines[app->notesAmount]->type);
        for(int i = 0; i < app->inputLength; i++){
            if(i < app->insertCursorx)
                mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + i, "%c", app->notes.lines[app->notesAmount]->note[i]);
            else
                mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + i + 1, "%c", app->notes.lines[app->notesAmount]->note[i]);
            mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + app->insertCursorx, "▏");
        }
    }
    else if(app->editingNote == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        if(app->inputLength == 0)
            mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4), "▏");
        mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 1), "%c", app->notes.lines[app->currentNote]->type);
        for(int i = 0; i < app->inputLength; i++){
            if(i < app->insertCursorx)
                mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + i, "%c", app->notes.lines[app->currentNote]->note[i]);
            else
                mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + i + 1, "%c", app->notes.lines[app->currentNote]->note[i]);
            mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + app->insertCursorx, "▏");
        }
    }
    else if(app->addingTask == 1){
        if(app->inputLength == 0)
            mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4), "▏");
        mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17), "[ ]");
        for(int i = 0; i < app->inputLength; i++){
            if(i < app->insertCursorx)
                mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + i, "%c", app->notes.lines[app->notesAmount]->note[i]);
            else
                mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + i + 1, "%c", app->notes.lines[app->notesAmount]->note[i]);
            mvprintw((app->middley - 9) + app->notesAmount, (app->middlex - 17 + 4) + app->insertCursorx, "▏");
        }
    }
    else if(app->editingTask == 1){
        setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
        if(app->inputLength == 0)
            mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4), "▏");
        for(int i = 0; i < app->inputLength; i++){
            if(i < app->insertCursorx)
                mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + i, "%c", app->notes.lines[app->currentNote]->note[i]);
            else
                mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + i + 1, "%c", app->notes.lines[app->currentNote]->note[i]);
            mvprintw((app->middley - 9) + app->currentNote, (app->middlex - 17 + 4) + app->insertCursorx, "▏");
        }
    }
}

/* Print noise menu */
void printNoiseMenu(appData * app){
    if(NOISE == 1){
        if(app->playNoise == 0 && app->needResume != 1){
            setColor(COLOR_BLACK, COLOR_BLACK, A_NORMAL);
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
        if(strcmp(ICONS, "nerdicons") == 0){
            mvprintw((app->middley - 1), (app->middlex - 16), " %d/%d🍅                    %02d/%02dm ",
                     app->unfinishedPomodoroCounter, app->unfinishedPomodoros, app->unfinishedTimer, app->unfinishedFullTimer);
        }
        else if(strcmp(ICONS, "iconson") == 0){
            mvprintw((app->middley - 1), (app->middlex - 16), " %d/%d🍅                    %02d/%02dm ",
                     app->unfinishedPomodoroCounter, app->unfinishedPomodoros, app->unfinishedTimer, app->unfinishedFullTimer);
        }
        else{
            mvprintw((app->middley - 1), (app->middlex - 16), " %d/%dP                     %02d/%02dm ",
                     app->unfinishedPomodoroCounter, app->unfinishedPomodoros, app->unfinishedTimer, app->unfinishedFullTimer);
        }
        mvprintw(app->middley, (app->middlex - 16),       "         Want to resume?         ");
        if(app->menuPos == 1){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 1), (app->middlex - 15), "-> YES <-");
        }else{
                setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 1), (app->middlex - 15), "   YES   ");
        }

        if(app->menuPos == 2){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 1), (app->middlex + 7),  "-> NO <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 1), (app->middlex + 7),  "   NO   ");
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
        mvprintw((app->middley - 7), (app->middlex - 11) ,"󰏤 ");
    }
    else if(strcmp(ICONS, "iconson") == 0){
        mvprintw((app->middley - 7), (app->middlex - 11) ,"⏸️ ");
    }
    else{
        mvprintw((app->middley - 7), (app->middlex - 11) ,"P ");;
    }
}

/* Print the Help Indicator */
void printHelpIndicator(appData * app){
    if(app->currentMode == -3)
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
    if(strcmp(ICONS, "nerdicons") == 0){
        mvprintw(1, (app->x - 2) ," ");
    }
    else if(strcmp(ICONS, "iconson") == 0){
        mvprintw(1, (app->x - 2) ,"❓ ");
    }
    else{
        mvprintw(1, (app->x - 2) ,"? ");;
    }
}

/* Print the Notepad Indicator */
void printNotepadIndicator(appData * app){
    if(app->currentMode == -2)
        setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
    else
        setColor(COLOR_BLACK, COLOR_BLACK, A_BOLD);
    if(strcmp(ICONS, "nerdicons") == 0){
        mvprintw(2, (app->x - 2) ,"󱞂 ");
    }
    else if(strcmp(ICONS, "iconson") == 0){
        mvprintw(2, (app->x - 2) ,"📝 ");
    }
    else{
        mvprintw(2, (app->x - 2) ,"N ");;
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
            mvprintw((app->middley + 6), (app->middlex - 7), "-> help menu <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 6), (app->middlex - 4), "help menu");
        }

        if(app->menuPos == 4 && app->needResume == 0){
            setColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
            mvprintw((app->middley + 7), (app->middlex - 5), "-> leave <-");
        }else{
            setColor(COLOR_WHITE, COLOR_BLACK, A_NORMAL);
            mvprintw((app->middley + 7), (app->middlex - 2), "leave");
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
                    fprintf(time, "");
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
                    fprintf(time, "");
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
                    fprintf(time, "");
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

