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
extern void printLogo(appData * , const char * );
extern void printCoffee(appData * );
extern void printMachine(appData * , const char * );
extern void printBeach(appData * );
extern void printGear(appData * , int );

