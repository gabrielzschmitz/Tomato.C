#ifndef INIT_H_
#define INIT_H_

#include "tomato.h"

/* Initialize ncurses screen and configure settings */
void InitScreen(void);

/* Initialize variables */
ErrorType InitApp(AppData *app);

/* End ncurses screen and delete default window and screen */
ErrorType EndScreen(void);

#endif /* INIT_H_ */
