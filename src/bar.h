#ifndef BAR_H_
#define BAR_H_

#include "ui.h"

/* Bar specific structs */
typedef struct AppData AppData;
typedef struct StatusBar StatusBar;
typedef struct StatusBarModule StatusBarModule;

typedef void (*ModuleUpdate)(AppData*, StatusBarModule*, Panel*);

/* Enum for specifying the bar position */
typedef enum { TOP, BOTTOM } StatusBarPosition;

/* Enum for specifying the module position within the bar */
typedef enum { LEFT, CENTER, RIGHT } StatusBarModulePosition;

/**
 * Structure for defining a status bar module.
 * A module represents a single segment of the status bar (e.g., time, mode).
 */
struct StatusBarModule {
  StatusBarModule* next; /**< Pointer to the next module in the list */
  StatusBarModulePosition position; /**< Position of module within bar */
  char* content;       /**< Content to be displayed in the module */
  int content_length;  /**< Length of the content string */
  int fg_color;        /**< Foreground color of the module text */
  int bg_color;        /**< Background color of the module */
  int id;              /**< Unique identifier for the module */
  ModuleUpdate update; /**< Function pointer for updating the module */
};

/**
 * Structure for configuring the status bar.
 * Contains position and linked lists of modules organized by position.
 */
struct StatusBar {
  StatusBarPosition position; /**< Position of the status bar (TOP or BOTTOM) */
  StatusBarModule* left_modules;   /**< List of modules positioned on left */
  StatusBarModule* center_modules; /**< List of modules on center */
  StatusBarModule* right_modules;  /**< List of modules on right */
};

/**
 * ---------------------------------------------------------------------------
 * StatusBar Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create and allocate a new StatusBar.
 * @param position Position of the status bar (TOP or BOTTOM)
 * @return Pointer to the created status bar, or NULL on allocation failure
 */
StatusBar* CreateStatusBar(StatusBarPosition position);

/**
 * Free a StatusBar and all its modules.
 * @param bar Pointer to the status bar to free
 */
void FreeStatusBar(StatusBar* bar);

/**
 * Add a new StatusBarModule to the end of the linked list by position.
 * @param status_bar Pointer to the status bar
 * @param position Position for the new module
 * @param update Function pointer for the module update logic
 */
void AddStatusBarModule(StatusBar* status_bar, StatusBarModulePosition position,
                        ModuleUpdate update);

/**
 * ---------------------------------------------------------------------------
 * Rendering/Updating
 * ---------------------------------------------------------------------------
 */

/**
 * Render the entire status bar with all its modules.
 * @param status_bar Pointer to the status bar to render
 * @param screen Pointer to the screen for dimensions
 * @param has_error_line If true, render status bar one line above bottom
 */
void RenderStatusBar(const StatusBar* status_bar, const Screen* screen,
                     bool has_error_line);

/**
 * Update all modules in the status bar.
 * @param app Pointer to the application data
 * @param status_bar Pointer to the status bar to update
 * @param current_panel Pointer to the current panel
 */
void UpdateStatusBar(AppData* app, StatusBar* status_bar, Panel* current_panel);

/**
 * ---------------------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------------------
 */

/**
 * Inverts the order of a linked list of StatusBarModule.
 * Used for right-aligned modules that need to be rendered in reverse order.
 * @param module Pointer to the first module in the list
 * @return Pointer to the new first module (was last)
 */
StatusBarModule* InvertModulesOrder(StatusBarModule* module);

/**
 * ---------------------------------------------------------------------------
 * Module Callbacks
 * ---------------------------------------------------------------------------
 */

/**
 * Map a module name string to its ModuleUpdate function pointer.
 * @param name The module name (e.g. "InputMode", "RealTime")
 * @return The corresponding ModuleUpdate, or NULL if unknown
 */
ModuleUpdate ModuleFromString(const char* name);

/**
 * Input mode module update function.
 * Displays the current input mode (NORMAL, INSERT, VISUAL) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void InputModeModule(AppData* app, StatusBarModule* module, Panel* panel);

/**
 * Real-time module update function.
 * Displays the current time in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void RealTimeModule(AppData* app, StatusBarModule* module, Panel* panel);

/**
 * Scene module update function.
 * Displays the current scene type (WORK, PAUSE, etc.) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void SceneModule(AppData* app, StatusBarModule* module, Panel* panel);

/**
 * Current status module update function.
 * Displays the pomodoro status (time remaining, cycle count) in the status bar.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void CurrentStatusModule(AppData* app, StatusBarModule* module, Panel* panel);

/**
 * Line and column module update function for NOTES scene.
 * Displays the current cursor line and column in the notes editor.
 * @param app Pointer to the application data
 * @param module Pointer to the module to update
 * @param panel Pointer to the current panel
 */
void LineColumnModule(AppData* app, StatusBarModule* module, Panel* panel);

#endif /* BAR_H_ */
