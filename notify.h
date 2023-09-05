/*
//         .             .              .
//         |             |              |           .
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
//  ,|
//  `'
// notify.h
*/
#ifndef NOTIFY_H_
#define NOTIFY_H_
#include <ncurses.h>

/* Send a notification with sound */
extern void notify(const char *);
extern void send_notification(char *, char *);
extern void play_audio(char *);

#endif

