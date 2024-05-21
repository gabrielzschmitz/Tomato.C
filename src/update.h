#ifndef UPDATE_H_
#define UPDATE_H_

#include "tomato.h"

/* Update variables */
ErrorType UpdateApp(AppData *app);

/* Get the screen size */
void GetScreenSize(AppData *app);

/* Update MAIN_MENU */
void UpdateMainMenu(AppData *app);

#endif /* UPDATE_H_ */
