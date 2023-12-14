/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// anim.h
*/
#ifndef ANIM_H_
#define ANIM_H_
#include <ncurses.h>

/* Time the animations frames */
extern void frameTimer(appData*);

/* Printing the animations frames */
extern void printLogo(appData*);
extern void printCoffee(appData*);
extern void printMachine(appData*);
extern void printBeach(appData*);
extern void printWrench(appData*, int);
extern void printBanner(appData*);
extern void printPergament(appData*);
extern void printNotepad(appData*);

#endif
