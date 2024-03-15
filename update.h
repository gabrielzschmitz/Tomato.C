/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// update.h
*/
#ifndef UPDATE_H_
#define UPDATE_H_
#include <ncurses.h>

#include "tomato.h"

/* Update modes */
extern void updateMainMenu(appData *);
extern void updateWorkTime(appData *);
extern void updateShortPause(appData *);
extern void updateLongPause(appData *);
extern void updateNotepad(appData *);
extern void updateHelpPage(appData *);

#endif
