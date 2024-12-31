#ifndef UPDATE_H_
#define UPDATE_H_

#include "error.h"
#include "tomato.h"

/* Update variables */
ErrorType UpdateApp(AppData* app);

/* Get the screen size */
void GetScreenSize(AppData* app);

/* Update MAIN_MENU */
void UpdateMainMenu(AppData* app);

/* Update WORK_TIME */
void UpdateWorkTime(AppData* app);

/* Update SHORT_PAUSE */
void UpdateShortPause(AppData* app);

/* Update LONG_PAUSE */
void UpdateLongPause(AppData* app);

/* Update NOTES */
void UpdateNotes(AppData* app);

/* Update HELP */
void UpdateHelp(AppData* app);

/* Update CONTINUE */
void UpdateContinue(AppData* app);

#endif /* UPDATE_H_ */
