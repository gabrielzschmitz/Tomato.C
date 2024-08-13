#ifndef DRAW_H_
#define DRAW_H_

#include "tomato.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app);

/* Check and Render Screen Size */
bool CheckScreenSize(AppData* app);

/* Show debug info and render a animation */
void DebugAnimation(Panel panel, Rollfilm* animation, Vector2D offset);

#endif /* DRAW_H_ */
