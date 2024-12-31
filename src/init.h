#ifndef INIT_H_
#define INIT_H_

#include "error.h"
#include "tomato.h"

/* Initialize ncurses screen and configure settings */
void InitScreen(void);

/* Initialize variables */
ErrorType InitApp(AppData* app);

/* Function to initialize the app menus */
ErrorType InitMenus(AppData* app);

/* Function to initialize the status bar */
ErrorType InitStatusBar(AppData* app);

/* Initialize animations from sprites */
ErrorType InitAnimations(AppData* app);

/* End ncurses screen and delete default window and screen */
ErrorType EndScreen(void);

/* End/Free variables */
ErrorType EndApp(AppData* app);

/* Init a Border struct with the config values */
Border InitBorder(void);

#endif /* INIT_H_ */
