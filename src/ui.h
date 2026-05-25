#ifndef UI_H_
#define UI_H_

#include "anim.h"
#include "history.h"
#include "util.h"

#define MAX_PANELS 2
#define MAX_CLICK_REGIONS 32

/* UI specific structs */
typedef struct Border Border;
typedef struct Screen Screen;
typedef struct Panel Panel;
typedef struct Menu Menu;
typedef struct MenuItem MenuItem;
typedef struct FloatingDialog FloatingDialog;

/* Forward declaration */
typedef struct AppData AppData;
typedef struct InputState InputState;

/**
 * Type definition for menu item action functions.
 * Functions take a pointer to AppData and return void.
 */
typedef void (*MenuAction)(AppData* app);

/**
 * Structure representing a single menu item.
 * Contains display label and associated action function.
 */
struct MenuItem {
  const char* label; /**< Display text for the menu item */
  MenuAction action; /**< Function to execute when item is selected */
};

/**
 * Structure for managing a menu and its items.
 * Handles item selection and styling.
 */
struct Menu {
  MenuItem* items;               /**< Dynamic array of menu items */
  int selected_item;             /**< Index of currently selected item */
  int focused_color;             /**< ncurses color pair for selected item */
  int unfocused_color;           /**< ncurses color pair for unselected items */
  const char* select_style_left; /**< Prefix string for selected item */
  const char* select_style_right; /**< Suffix string for selected item */
  int item_count;                 /**< Number of items in the menu */
};

/**
 * Enum for menu type identifiers.
 * Used to categorize menus and select appropriate behavior.
 */
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

/**
 * Structure for storing panel properties and state.
 * A panel is a rectangular region of the screen containing content.
 */
struct Panel {
  History* scene_history; /**< History manager for this panel's scenes */
  Dimensions size;        /**< Width and height of the panel */
  Vector2D position;      /**< Top-left position of the panel on screen */
  int mode;               /**< Current mode/state of the panel */
  int menu_index;         /**< Index of the active menu for this panel */
  bool visible;           /**< Whether the panel should be rendered */
  InputState* input;      /**< Text input state (NULL in DEFAULT mode) */
};

/**
 * Structure for managing the screen and its panels.
 * Represents the entire application window.
 */
struct Screen {
  Panel panels[MAX_PANELS];  /**< Array of panels on the screen */
  Dimensions size;           /**< Total dimensions of the screen */
  Dimensions min_panel_size; /**< Minimum dimensions for each panel */
  int rendered_panel_count;  /**< Number of panels currently visible */
  int current_panel;         /**< Index of the focused panel */
};

/**
 * Structure for defining border characters.
 * Each field contains a single character (possibly multi-byte UTF-8).
 */
struct Border {
  const char* top_left;     /**< Character for top-left corner */
  const char* top_right;    /**< Character for top-right corner */
  const char* bottom_left;  /**< Character for bottom-left corner */
  const char* bottom_right; /**< Character for bottom-right corner */
  const char* horizontal;   /**< Character for horizontal borders */
  const char* vertical;     /**< Character for vertical borders */
};

/**
 * Structure for representing a floating dialog box.
 * Modal dialog that overlays the main content.
 */
struct FloatingDialog {
  Dimensions size;   /**< Width and height of the dialog */
  Vector2D position; /**< Top-left position on screen */
  Border border;     /**< Border characters for the dialog frame */
  Menu menu;         /**< Menu displayed inside the dialog */
  char* message;     /**< Message text displayed in the dialog */
  bool visible;      /**< Whether the dialog is currently shown */
};

/**
 * Mouse click region types.
 */
typedef enum {
  REGION_DIRECT,     /**< Direct action (skip/pause buttons) */
  REGION_MENU_ITEM,  /**< Regular menu item */
  REGION_POPUP_ITEM, /**< Popup dialog menu item */
  REGION_NOTE_ITEM,  /**< Note/task item in the notes panel */
} RegionType;

/**
 * Structure for tracking clickable screen regions.
 * Registered during rendering, tested against mouse events.
 */
typedef struct {
  Vector2D pos;      /**< Top-left position */
  Dimensions size;   /**< Region dimensions */
  RegionType type;   /**< Type of region */
  MenuAction action; /**< Action for REGION_DIRECT */
  int menu_index;    /**< Menu index for REGION_MENU_ITEM */
  int item_index;    /**< Item index within menu */
  int note_id;       /**< Note item ID for REGION_NOTE_ITEM */
} ClickRegion;

/**
 * ---------------------------------------------------------------------------
 * Screen / Panel
 * ---------------------------------------------------------------------------
 */

/**
 * Create a screen struct with MAX_PANELS in horizontal rows.
 * Initializes panels based on current terminal size.
 * @return Pointer to the created Screen, or NULL on allocation failure
 */
Screen* CreateScreen(void);

/**
 * Free all memory associated with a Screen struct.
 * @param screen Pointer to the screen to free
 */
void FreeScreen(Screen* screen);

/**
 * Update panel positions and dimensions based on screen layout.
 * @param screen Pointer to the screen to update
 * @param has_error_line If true, reduce panel height to leave room for error line
 */
void UpdateScreen(Screen* screen, bool has_error_line);

/**
 * Render a border around a panel using ncurses.
 * @param panel Panel to render border around
 * @param border Border character configuration
 */
void RenderPanelBorder(Panel panel, Border border);

/**
 * Render an animation at the center of a panel.
 * @param panel Pointer to the panel
 * @param animation Pointer to the animation to render
 * @param offset Offset from the center position
 */
void RenderAnimationAtPanelCenter(Panel* panel, Rollfilm* animation,
                                  Vector2D offset);

/**
 * Render a screen size error message.
 * Displayed when terminal is too small for the app.
 * @param screen Pointer to the screen
 * @param panel Pointer to a panel to use for dimensions
 */
void RenderScreenSizeError(Screen* screen, Panel* panel);

/* ---------------------------------------------------------------------------
 * History
 * --------------------------------------------------------------------------- */

/**
 * Perform undo operation - move back in history.
 * Pops from past stack and pushes current to future stack.
 * @param history Pointer to the history manager
 */
void UndoHistory(History* history);

/**
 * Perform redo operation - move forward in history.
 * Pops from future stack and pushes current to past stack.
 * @param history Pointer to the history manager
 */
void RedoHistory(History* history);

/**
 * Execute a new scene, adding current to past stack.
 * Clears future stack and sets new present scene.
 * @param history Pointer to the history manager
 * @param new_scene SceneType to execute
 */
void ExecuteHistory(History* history, int new_scene);

/* ---------------------------------------------------------------------------
 * Menu
 * --------------------------------------------------------------------------- */

/**
 * Initialize a menu with items and styling.
 * @param items Array of MenuItem structures
 * @param num_items Number of items in the array
 * @param focused_color Color for selected item
 * @param unfocused_color Color for unselected items
 * @param select_style_left Left prefix for selected item
 * @param select_style_right Right suffix for selected item
 * @return Pointer to the created Menu, or NULL on allocation failure
 */
Menu* CreateMenu(MenuItem items[], int num_items, int focused_color,
                 int unfocused_color, const char* select_style_left,
                 const char* select_style_right);

/**
 * Free memory allocated for a Menu.
 * @param menu Pointer to the menu to free
 */
void FreeMenu(Menu* menu);

/**
 * Print a menu centered on screen with offset and line spacing.
 * @param app Pointer to the application data (for click region tracking)
 * @param panel Pointer to the panel containing the menu
 * @param menu Pointer to the menu to print
 * @param offset Offset from screen center
 * @param line_spacing Extra lines between items
 */
void PrintMenuAtCenter(AppData* app, Panel* panel, Menu* menu, Vector2D offset,
                       int line_spacing);

/**
 * Change the selected item in the menu.
 * @param menu Pointer to the menu
 * @param direction 1 for next, -1 for previous
 */
void ChangeSelectedItem(Menu* menu, int direction);

/**
 * ---------------------------------------------------------------------------
 * Click Regions
 * ---------------------------------------------------------------------------
 */

/**
 * Clear all registered click regions for the current frame.
 * @param app Pointer to the application data
 */
void ClearClickRegions(AppData* app);

/**
 * Register a clickable screen region for mouse interaction.
 * @param app Pointer to the application data
 * @param x Top-left x coordinate
 * @param y Top-left y coordinate
 * @param width Region width in characters
 * @param height Region height in characters
 * @param type Region type (DIRECT, MENU_ITEM, POPUP_ITEM)
 * @param action Action function (for REGION_DIRECT)
 * @param menu_index Menu index (for REGION_MENU_ITEM)
 * @param item_index Item index within menu
 */
void RegisterClickRegion(AppData* app, int x, int y, int width, int height,
                         RegionType type, MenuAction action, int menu_index,
                         int item_index, int note_id);

/* ---------------------------------------------------------------------------
 * Floating Dialog
 * --------------------------------------------------------------------------- */

/**
 * Create a FloatingDialog on given position.
 * @param position Vector2D of where to initialize the dialog
 * @param size Dimensions size of the dialog
 * @param border Border character configuration
 * @param menu Menu to display in the dialog
 * @param message Message text to display
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreateFloatingDialog(Vector2D position, Dimensions size,
                                     Border border, Menu menu,
                                     const char* message);

/**
 * Create a FloatingDialog centered on the screen.
 * @param screen Pointer to the screen for dimensions
 * @param menu Menu to display in the dialog
 * @param message Message text to display
 * @param border Border character configuration
 * @return Pointer to the created dialog, or NULL on allocation failure
 */
FloatingDialog* CreateCenterFloatingDialog(Screen* screen, Menu menu,
                                           const char* message, Border border);

/**
 * Free all memory of a FloatingDialog.
 * @param dialog Pointer to the dialog to free
 */
void FreeFloatingDialog(FloatingDialog* dialog);

/**
 * Render a FloatingDialog using ncurses.
 * @param app Pointer to the application data (for click region tracking)
 * @param dialog Pointer to the dialog to render
 */
void RenderFloatingDialog(AppData* app, FloatingDialog* dialog);

/**
 * Update a FloatingDialog to be centered on the current screen.
 * Call this before rendering to handle screen resize.
 * @param dialog Pointer to the dialog to update
 * @param screen Pointer to the screen for current dimensions
 */
void UpdateFloatingDialog(FloatingDialog* dialog, Screen* screen);

/* ---------------------------------------------------------------------------
 * Popups
 * --------------------------------------------------------------------------- */

/**
 * Render a quit confirmation message at the center of the screen.
 * @param app Pointer to the application data
 */
void RenderQuitConfirmation(AppData* app);

/**
 * Render a critical error quit confirmation - no cancel option.
 * Used when app is frozen due to critical error.
 * @param app Pointer to the application data
 */
void RenderCriticalQuitConfirmation(AppData* app);

/**
 * Render a reset pomodoro menu at the center of the screen.
 * @param app Pointer to the application data
 */
void RenderResetMenu(AppData* app);

/**
 * Render a skip confirmation message at the center of the screen.
 * @param app Pointer to the application data
 */
void RenderSkipConfirmation(AppData* app);

/**
 * Create a welcome popup dialog for first-time users.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateWelcomeDialog(AppData* app);

/**
 * Create a continue/cancel popup dialog for unfinished sessions.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateContinueDialog(AppData* app);

/* ---------------------------------------------------------------------------
 * Pomodoro
 * --------------------------------------------------------------------------- */

/**
 * Render pomodoro status (timer display) with animation.
 * @param app Pointer to the application data
 * @param anim_size Dimensions of the animation area
 * @param anim_pos Position of the animation
 */
void RenderPomodoroStatus(AppData* app, Dimensions anim_size,
                          Vector2D anim_pos);

/**
 * Render pomodoro control icons (pause, skip, etc.).
 * @param app Pointer to the application data
 * @param pos Position to render controls
 */
void RenderPomodoroControls(AppData* app, Vector2D pos);

#endif /* UI_H_ */
