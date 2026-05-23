#ifndef INIT_H_
#define INIT_H_

#include "error.h"
#include "ui.h"

typedef struct AppData AppData;

/**
 * ---------------------------------------------------------------------------
 * App Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize application variables and data structures.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType InitApp(AppData* app);

/**
 * End application and free all allocated resources.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType EndApp(AppData* app);

/**
 * Initialize ncurses screen and configure settings.
 * Sets up terminal for curses mode with required features.
 * @return ErrorType NO_ERROR on success, or WINDOW_CREATION_ERROR on failure
 */
ErrorType InitScreen(void);

/**
 * End ncurses screen and clean up default window.
 * Must be called before program exit.
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType EndScreen(void);

/**
 * ---------------------------------------------------------------------------
 * Components
 * ---------------------------------------------------------------------------
 */

/**
 * Initialize a Border struct with configured character values.
 * @return Border struct with default border characters
 */
Border InitBorder(void);

#endif /* INIT_H_ */
