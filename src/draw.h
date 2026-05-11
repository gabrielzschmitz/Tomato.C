#ifndef DRAW_H_
#define DRAW_H_

#include <stdbool.h>

#include "error.h"

typedef struct AppData AppData;

/**
 * Draw the entire screen based on the current app state.
 * Renders all visible panels, animations, and UI elements.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType DrawScreen(AppData* app);

/**
 * Check if the screen size is sufficient and render the error screen.
 * Returns false if screen is too small, true otherwise.
 * @param app Pointer to the application data
 * @return true if screen is large enough, false otherwise
 */
bool ValidateAndRenderScreenSize(AppData* app);

#endif /* DRAW_H_ */
