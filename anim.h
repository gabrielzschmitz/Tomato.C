/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// util.h
*/
#include <ncurses.h>

/* Time the animations frames */
extern void frameTimer(appData * );

/* Printing the animations */
extern void printLogo(appData * );
extern void printCoffee(appData * );
extern void printMachine(appData * );
extern void printBeach(appData * );
extern void printGear(appData * , int );

