#ifndef INIT_H_
#define INIT_H_

#include "tomato.h"

/* Initialize ncurses screen and configure settings */
ErrorType InitScreen(void);

/* Initialize variables */
ErrorType InitApp(AppData *app);

#endif /* INIT_H_ */
