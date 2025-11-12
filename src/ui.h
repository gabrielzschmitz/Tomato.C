#ifndef UI_H_
#define UI_H_

#include "anim.h"
#include "util.h"

#define MAX_PANELS 2

/* UI specific structs */
typedef struct Border Border;
typedef struct Screen Screen;
typedef struct Panel Panel;
typedef struct History History;
typedef struct HistoryNode HistoryNode;
typedef struct Menu Menu;
typedef struct MenuItem MenuItem;
typedef struct FloatingDialog FloatingDialog;

/* Forward declaration of AppData */
typedef struct AppData AppData;
/* Define a type for menu item action functions */
typedef void (*MenuAction)(AppData* app);

/* Structure for a menu item with a label and an associated action */
struct MenuItem {
  const char* label; /* Label for the menu item */
  MenuAction action; /* Function pointer for the action */
};

/* Structure for managing a menu and its items */
struct Menu {
  MenuItem* items;     /* Array of menu items */
  int selected_item;   /* Index of the currently selected item */
  int focused_color;   /* Color code for the focused (selected) item */
  int unfocused_color; /* Color code for the unfocused (deselected) items */
  const char* select_style_left;  /* String for the left style selected item */
  const char* select_style_right; /* String for the right style selected item */
  int item_count;                 /* Number of items in the menu */
};

typedef enum {
  MAIN_MENU_MENU,
  PREFERENCES_MENU,
  WORK_TIME_MENU,
  SHORT_PAUSE_MENU,
  LONG_PAUSE_MENU,
  NOTES_MENU,
  HELP_MENU,
  CONTINUE_MENU,
} MenuType;

/* Structure for storing a single history node */
struct HistoryNode {
  HistoryNode* next; /* Pointer to the next node in the history stack */
  int scene;         /* Identifier for the stored scene */
};

/* Structure for managing history stacks */
struct History {
  HistoryNode* future_stack; /* Stack of future scenes for redo operations */
  HistoryNode* past_stack;   /* Stack of past scenes for undo operations */
  int present;               /* Identifier of the current scene */
};

/* Structure for storing properties of a panel */
struct Panel {
  History* scene_history; /* Pointer to the history management struct */
  Dimensions size;        /* Dimensions of the panel */
  Vector2D position;      /* Position of the panel on the screen */
  int mode;               /* Mode or state of the panel */
  int menu_index;         /* Menu index of the current panel */
  bool visible;           /* Visibility status of the panel */
};

/* Structure for managing the screen and its panels */
struct Screen {
  Panel panels[MAX_PANELS];  /* Array of panels on the screen */
  Dimensions size;           /* Dimensions of the entire screen */
  Dimensions min_panel_size; /* Minimal dimensions for each panel to have */
  int rendered_panel_count;  /* Count of currently rendered panels */
  int current_panel;         /* Index of the currently active panel */
};

/* Structure for defining the border of a panel */
struct Border {
  const char* top_left;     /* Character for the top-left corner */
  const char* top_right;    /* Character for the top-right corner */
  const char* bottom_left;  /* Character for the bottom-left corner */
  const char* bottom_right; /* Character for the bottom-right corner */
  const char* horizontal;   /* Character for the horizontal border line */
  const char* vertical;     /* Character for the vertical border line */
};

/* Structure for representing a floating dialog box */
struct FloatingDialog {
  Dimensions size;   /* Dimensions of the dialog (width and height) */
  Vector2D position; /* Position of the dialog on the screen */
  Border border;     /* Border style for the dialog */
  Menu menu;         /* Menu within the dialog */
  char* message;     /* Message displayed in the dialog */
  bool visible;      /* Visibility status of the dialog */
};

/* Create a screen struct with MAX_PANELS in horizontal rows */
Screen* CreateScreen(void);

/* Free all memory associated with a Screen struct */
void FreeScreen(Screen* screen);

/* Create a screen struct with the current screen size and MAX_PANELS */
Panel CreatePanel(Dimensions size, Vector2D position);

/* Function to free the memory of a panel */
void FreePanel(Panel* panel);

/* Render a border in a given panel */
void RenderPanelBorder(Panel panel, Border border);

/* Render a border for a FloatingDialog */
void RenderFloatingDialogBorder(FloatingDialog* dialog);

/* Render a quit confirmation message at the center of the screen */
void RenderQuitConfirmation(AppData* app);

/* Render a reset pomodoro menu at the center of the screen */
void RenderResetMenu(AppData* app);

/* Render a skip confirmation message at the center of the screen */
void RenderSkipConfirmation(AppData* app);

/* Update panels from a given screen */
void UpdateScreen(Screen* screen);

/* Update panel dimensions and position */
void UpdatePanel(Panel* panel, Dimensions size, Vector2D position);

/* Render content at panel center */
void RenderAtPanelCenter(Panel* panel, const char* content, Vector2D offset);

/* Render animation at panel center */
void RenderAnimationAtPanelCenter(Panel* panel, Rollfilm* animation,
                                  Vector2D offset);

/* Render screen size error */
void RenderScreenSizeError(Screen* screen, Panel* panel);

/* Function to push a scene onto a stack */
void PushHistory(HistoryNode** stack, int scene);

/* Function to pop a scene from a stack */
int PopHistory(HistoryNode** stack);

/* Function to clear a stack */
void ClearStack(HistoryNode** stack);

/* Function to create and initialize a new history */
History* CreateHistory(void);

/* Function to free the memory of a history */
void FreeHistory(History* history);

/* Function to perform undo operation */
void UndoHistory(History* history);

/* Function to perform redo operation */
void RedoHistory(History* history);

/* Function to perform do operation */
void ExecuteHistory(History* history, int new_scene);

/* Print a given menu at the center of the screen + offset */
void PrintMenuAtCenter(Panel* panel, Menu* menu, Vector2D offset,
                       int line_spacing);

/* Print a given menu side by side with offset and spacing */
void PrintMenuSideBySide(Menu* menu, Vector2D offset, int spacing,
                         int container_width);

/* Function to initialize a menu and return a pointer to it */
Menu* CreateMenu(MenuItem items[], int num_items, int focused_color,
                 int unfocused_color, const char* select_style_left,
                 const char* select_style_right);

/* Function to free the memory allocated for the menu */
void FreeMenu(Menu* menu);

/* Function to change the selected item in the menu */
void ChangeSelectedItem(Menu* menu, int direction);

/* Create a new FloatingDialog */
FloatingDialog* CreateFloatingDialog(Vector2D position, Dimensions size,
                                     Border border, Menu menu,
                                     const char* message);

/* Free all memory of a FloatingDialog */
void FreeFloatingDialog(FloatingDialog* dialog);

/* Render a FloatingDialog using ncurses */
void RenderFloatingDialog(FloatingDialog* dialog);

/* Create a FloatingDialog centered on the screen */
FloatingDialog* CreateCenterFloatingDialog(Screen* screen, Menu menu,
                                           const char* message, Border border);

/* Render a pomodoro status */
void RenderPomodoroStatus(AppData* app, Dimensions anim_size,
                          Vector2D anim_pos);

/* Render a pomodoro controllers */
void RenderPomodoroControls(AppData* app, Vector2D pos);

#endif /* UI_H_ */
