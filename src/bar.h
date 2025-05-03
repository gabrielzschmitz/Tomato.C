#ifndef BAR_H_
#define BAR_H_

#include "ui.h"

typedef struct AppData AppData;

/* Bar specific structs */
typedef struct StatusBarModule StatusBarModule;
typedef struct StatusBar StatusBar;

/* Function pointer type for updating a module */
typedef void (*ModuleUpdate)(AppData*, struct StatusBarModule*, struct Panel*);

/* Enum for specifying the bar position */
typedef enum { TOP, BOTTOM } StatusBarPosition;

/* Enum for specifying the module position within the bar */
typedef enum { LEFT, CENTER, RIGHT } StatusBarModulePosition;

/* Structure for defining a status bar module */
struct StatusBarModule {
  StatusBarModule* next;            /* Pointer to the next module in the list */
  StatusBarModulePosition position; /* Position of the module within the bar */
  char* content;                    /* Content to be displayed in the module */
  int content_length;               /* Length of the content */
  int fg_color;                     /* Foreground color of the module */
  int bg_color;                     /* Background color of the module */
  int id;                           /* Unique identifier for the module */
  ModuleUpdate update; /* Function pointer for updating the module */
};

/* Structure for configuring the status bar */
struct StatusBar {
  StatusBarPosition position; /* Position of the status bar (top or bottom) */
  StatusBarModule*
    left_modules; /* Linked list of modules positioned on the left */
  StatusBarModule*
    center_modules; /* Linked list of modules positioned in the center */
  StatusBarModule*
    right_modules; /* Linked list of modules positioned on the right */
};

/* Function to create and allocate a StatusBarModule */
StatusBarModule* CreateStatusBarModule(StatusBarModulePosition position,
                                       char* content, int fg_color,
                                       int bg_color, ModuleUpdate update);

/* Function to add a StatusBarModule to the end of the linked list by values */
void AddStatusBarModule(StatusBar* status_bar, StatusBarModulePosition position,
                        ModuleUpdate update);

/* Function to free a StatusBarModule */
void FreeStatusBarModule(StatusBarModule* widget);

/* Function to free a linked list of StatusBarModule modules */
void FreeStatusBarModules(StatusBarModule* module);

/* Function to create and allocate a StatusBar */
StatusBar* CreateStatusBar(StatusBarPosition position);

/* Function to free a StatusBar */
void FreeStatusBar(StatusBar* bar);

void RenderStatusBar(const StatusBar* status_bar, const Screen* screen);

void RenderStatusBarModule(const StatusBarModule* module, int start_y,
                           int start_x, int max_width);

/* Update status bar module */
void UpdateStatusBarModule(AppData* app, StatusBarModule* module,
                           Panel* current_panel);

/* Update status bar */
void UpdateStatusBar(AppData* app, StatusBar* status_bar, Panel* current_panel);

/* Inverts the order of a linked list of StatusBarModule */
StatusBarModule* InvertModulesOrder(StatusBarModule* module);

/* Input mode module to status bar */
void InputModeModule(AppData* app, StatusBarModule* module, Panel* panel);

/* Real-time module for status bar */
void RealTimeModule(AppData* app, StatusBarModule* module, Panel* panel);

/* Current scene module for status bar */
void SceneModule(AppData* app, StatusBarModule* module, Panel* panel);

/* Current status module for status bar */
void CurrentStatusModule(AppData* app, StatusBarModule* module, Panel* panel);

#endif /* BAR_H_ */
