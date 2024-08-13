#ifndef INPUT_H_
#define INPUT_H_

#include "tomato.h"

/* Handle user input and app state */
ErrorType HandleInputs(AppData *app);

/* Handle normal mode input */
ErrorType HandleNormalMode(AppData *app);

/* Handle insert mode input */
ErrorType HandleInsertMode(AppData *app);

/* Update animation mode */
void ChangeDebugAnimation(AppData *app, int step);

#endif /* INPUT_H_ */
