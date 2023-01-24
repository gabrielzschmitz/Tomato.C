/*
//         .             .              .		    
//         |             |              |           .	    
// ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
// | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
// `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
//  ,|							    
//  `'							    
// input.h
*/
#include <ncurses.h>

/* Defining some ASCII Keys */
# define ESC 27
# define ENTER 10
# define CTRLC 3
# define CTRLP 16
# define CTRLR 18
# define CTRLX 24

/* Handle user input and app state */
extern void handleInputs(appData * , const int , const int , const char * , const int );
extern void mouseInput(appData * , MEVENT , char , const int , const int , const char * , const int );
extern void resumeInput(appData * , char );
extern void mainMenuInput(appData * , char , const int , const int , const char * , const int );
extern void settingsInput(appData * , char );

