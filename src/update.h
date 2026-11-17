#ifndef UPDATE_H_
#define UPDATE_H_

#include <stddef.h>

#include "error.h"

/**
 * App / Screen
 */

/**
 * Update all application variables and state.
 * Called each frame to refresh app state.
 * @param app Pointer to the application data
 * @return NO_ERROR when successful, or an error code on failure.
 */
ErrorType UpdateApp(AppData* app);

/**
 * Scene Updates
 */

/**
 * Update MAIN_MENU scene state and menu selection.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateMainMenu(AppData* app);

/**
 * Update WORK_TIME scene - timer countdown and animations.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateWorkTime(AppData* app);

/**
 * Update SHORT_PAUSE scene - timer countdown and animations.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateShortPause(AppData* app);

/**
 * Update LONG_PAUSE scene - timer countdown and animations.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateLongPause(AppData* app);

/**
 * Update NOTES scene - note selection and editing state.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateNotes(AppData* app);

/**
 * Update HELP scene - help content display.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateHelp(AppData* app);

/**
 * Update CONTINUE scene - pause/resume confirmation.
 * @param app Pointer to the application data
 * @return void
 */
void UpdateContinue(AppData* app);

#endif /* UPDATE_H_ */
