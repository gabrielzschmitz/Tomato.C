#ifndef UI_H_
#define UI_H_

#include "anim.h"
#include "history.h"
#include "util.h"

#define MAX_PANELS 2
#define MAX_CLICK_REGIONS 32
#define WELCOME_SLIDE_COUNT 5

/* UI specific structs */
typedef struct Border Border;
typedef struct Screen Screen;
typedef struct Panel Panel;
typedef struct Menu Menu;
typedef struct MenuItem MenuItem;
typedef struct FloatingDialog FloatingDialog;
typedef struct SlideDef SlideDef;

/* Forward declarations */
typedef struct AppData AppData;
typedef struct InputState InputState;

/**
 * Alignment for slide content lines.
 */
typedef enum {
  ALIGN_SLIDE_LEFT,   /**< Left-aligned */
  ALIGN_SLIDE_CENTER, /**< Centered */
  ALIGN_SLIDE_RIGHT   /**< Right-aligned */
} SlideAlign;

/** Action function for slide nav controls (prev/next/close). */
typedef void (*SlideNavAction)(AppData* app);

/**
 * A single control button in a slide's navigation bar.
 * Rendered at the position determined by its alignment, executing
 * action on click.
 */
typedef struct {
  const char* text;      /**< Display text (e.g. "[Close]", "Next  >") */
  SlideAlign align;      /**< LEFT, CENTER, or RIGHT positioning */
  SlideNavAction action; /**< Function to call on click */
} ControlButton;

/**
 * Definition of navigation controls for a slide.
 * Contains an array of at most 3 buttons arranged by alignment.
 */
typedef struct {
  ControlButton* buttons; /**< Array of control buttons */
  int count;              /**< Number of buttons (0-3) */
} SlideControls;

/**
 * Definition of a progress indicator for slides.
 * Renders a title with either dots ("Welcome ●●○○○") or
 * counter format ("Welcome 2/5" in ASCII mode).
 */
typedef struct {
  const char* title;    /**< Title prefix (e.g. "Welcome") */
  const char* icon_on;  /**< Filled dot character ("●"); "" for N/M mode */
  const char* icon_off; /**< Empty dot character ("○"); "" for N/M mode */
  int total;            /**< Total number of slides */
  int current; /**< Metadata only; render reads from dialog at runtime */
} SlideProgress;

/**
 * A single formatted text segment in a slide's token stream.
 * Each token carries its own color, alignment, and optional x-override.
 * Tokens are linked into a singly-linked list owned by SlideDef.
 */
typedef struct SlideToken {
  int y;                   /**< Row offset (0 = row 3 in dialog) */
  char* text;              /**< Allocated text for this segment */
  int color;               /**< Color 0-15 (<0 = NO_COLOR → white+bold) */
  SlideAlign align;        /**< Alignment (from \a, persists across \c) */
  int x;                   /**< Absolute column override (0 = use align) */
  struct SlideToken* next; /**< Next token (NULL = end) */
} SlideToken;

/**
 * Slide type identifier that determines input handling behaviour.
 * Each slide-based dialog sets this to route keyboard/mouse
 * dispatch to the appropriate action set.
 */
typedef enum {
  SLIDE_TYPE_NONE,     /**< Not a slide-based dialog (regular menu popup) */
  SLIDE_TYPE_WELCOME,  /**< Multi-slide carousel with prev/next navigation */
  SLIDE_TYPE_CONTINUE, /**< Single-slide session dialog with action buttons */
  SLIDE_TYPE_NOISE,    /**< Single-slide white noise dialog */
  SLIDE_TYPE_HISTORY_OVERVIEW, /**< History contribution graph */
  SLIDE_TYPE_HISTORY_DAY,      /**< History single-day session details */
  SLIDE_TYPE_HISTORY_STATS,    /**< History statistics */
} SlideType;

/**
 * Definition of a single slide.
 * Contains token stream for one icon type, render/update callbacks,
 * per-frame hover state, slide type for input dispatch, and optional
 * generic progress/controls renderers.
 */
struct SlideDef {
  SlideToken* tokens; /**< Linked-list head of formatted tokens */
  Dimensions size;    /**< Slide width and height in columns/rows */
  void (*render)(AppData* app, SlideDef* def); /**< Render this slide */
  void (*update)(AppData* app, SlideDef* def); /**< Update hover state */
  int hovered; /**< Currently hovered nav control (-1 = none) */
  SlideType
    slide_type; /**< Determines input dispatch (welcome vs continue etc.) */
  void (*render_progress)(AppData* app, int x, int y, int w, SlideDef* def,
                          SlideProgress* params); /**< Progress renderer */
  void (*render_controls)(AppData* app, int x, int y, int w, SlideDef* def,
                          SlideControls* params); /**< Controls renderer */
  SlideProgress* progress; /**< Progress params (heap-allocated, owned) */
  SlideControls* controls; /**< Controls params (heap-allocated, owned) */
};

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
  SlideDef** slides; /**< Pre-built slides [iconType * stride + slideIdx] */
  int slideCount;    /**< Total slides in array (3 * stride) */
  int currentSlide;  /**< Currently displayed slide index (0 to stride-1) */
  SlideType
    slide_type; /**< Determines input dispatch (welcome vs continue etc.) */
  int hovered_button; /**< Currently hovered control button index (-1 = none) */
};

/**
 * Mouse click region types.
 */
typedef enum {
  REGION_DIRECT,      /**< Direct action (skip/pause buttons) */
  REGION_MENU_ITEM,   /**< Regular menu item */
  REGION_POPUP_ITEM,  /**< Popup dialog menu item */
  REGION_NOTE_ITEM,   /**< Note/task item in the notes panel */
  REGION_WELCOME_NAV, /**< Welcome dialog navigation control */
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

/**
 * Create a white noise control dialog for ambient sounds.
 * @param app Pointer to the application data
 * @return Pointer to the created dialog, or NULL on failure
 */
FloatingDialog* CreateNoiseDialog(AppData* app);

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

/**
 * ---------------------------------------------------------------------------
 * Slides
 * ---------------------------------------------------------------------------
 */

/**
 * Free all memory associated with a welcome slides array.
 * Walks and frees each slide's token linked list, then the slide array.
 * @param slides Array of SlideDef pointers to free
 * @param count Number of elements in the array
 */
void FreeWelcomeSlides(SlideDef** slides, int count);

/**
 * Build all welcome slide definitions for all icon types.
 * Allocates 3 * stride SlideDef instances, one per icon
 * type per slide. Index with [iconType * WELCOME_SLIDE_COUNT + slideIdx].
 * @param size Slide dimensions: width=41, height=-1 to use per-slide height
 * @return Pointer to array of SlideDef pointers, or NULL on allocation failure
 */
SlideDef** BuildWelcomeSlides(Dimensions size);

/**
 * Build a set of continue session slides (one per icon type).
 * Reads current session data from app->pomodoro_data and
 * formats it into token-format text with escape sequences.
 * @param app Application state with loaded pomodoro session data
 * @param size Slide dimensions (width=39, height=19)
 * @return Array of 3 SlideDef pointers (one per icon type), or NULL on failure
 */
SlideDef** BuildContinueSlides(AppData* app, Dimensions size);

/**
 * Build an array of white noise slides (one slide).
 * Height is computed dynamically from the registered track count
 * as h = 15 + 2 * track_count (minimum 15).
 * @param app  Application state
 * @param size Slide dimensions (width used; height recomputed)
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildNoiseSlides(AppData* app, Dimensions size);

/**
 * Handle mouse click and scroll events on the white noise control slide.
 *
 * Determines which track row the event falls on (y + 8 + i*2 for tracks,
 * y + 18 for master), then for clicks divides the row horizontally:
 * left of the \x17 (minus) column toggles play/pause, the minus icon
 * zone decreases volume, and the plus icon zone increases it.  Scroll
 * events (BUTTON4/BUTTON5) adjust the volume of the targeted row by ±10.
 *
 * @param app     Pointer to the application data
 * @param event   The ncurses mouse event
 * @param is_click True if this is a click (BUTTON1_PRESSED), false for scroll
 */
void NoiseSlideMouseAction(AppData* app, MEVENT* event, bool is_click);

/**
 * Default progress renderer for slides.
 * Draws the title with filled/empty dots ("Welcome ●●○○○") in
 * UTF-8/nerd mode, or as a counter ("Welcome 3/5") in ASCII mode.
 * Current slide index is obtained from app->popup_dialog->currentSlide.
 * @param app    Application state
 * @param x      Absolute column position (slide left edge)
 * @param y      Absolute row position (progress line)
 * @param w      Slide width
 * @param def    Slide definition (unused)
 * @param params Progress parameters (title, icons, total)
 */
void SlideProgressRender(AppData* app, int x, int y, int w, SlideDef* def,
                         SlideProgress* params);

/**
 * Default controls renderer for slides.
 * Positions each button by its align field:
 *   LEFT   → x + 2
 *   CENTER → centered within (w - 2)
 *   RIGHT  → x + w - 2 - text_width
 * Draws with A_REVERSE when def->hovered matches the button index,
 * and registers a REGION_WELCOME_NAV click region with the action.
 * @param app    Application state
 * @param x      Absolute column position (slide left edge)
 * @param y      Absolute row position (controls line)
 * @param w      Slide width
 * @param def    Slide definition (provides hovered index)
 * @param params Controls parameters (button array + count)
 */
void SlideControlsRender(AppData* app, int x, int y, int w, SlideDef* def,
                         SlideControls* params);

/**
 * Draw the dialog frame with ACS line-drawing characters.
 * The frame has a top rule, middle rule (at y+2), bottom rule, and vertical sides.
 * @param x X position of the dialog
 * @param y Y position of the dialog
 * @param w Width of the dialog
 * @param h Height of the dialog
 */
void RenderSlideBox(int x, int y, int w, int h);

/* ---------------------------------------------------------------------------
 * History Popups
 * --------------------------------------------------------------------------- */

/**
 * @brief Create the History Overview popup (contribution graph).
 * @param app Application state
 */
void CreateHistoryOverviewDialog(AppData* app);

/**
 * @brief Create the Day Detail popup.
 * @param app Application state
 */
void CreateHistoryDayDialog(AppData* app);

/**
 * @brief Create the Statistics popup.
 * @param app Application state
 */
void CreateHistoryStatsDialog(AppData* app);

/**
 * @brief Resolve cursor position (cursorWeek, cursorDow) into a concrete date
 *        stored in history_data (selYear, selMonth, selDay).
 * @param app Application state
 */
void historyResolveCursor(AppData* app);

/**
 * @brief Build a single history slide with custom render/update callbacks.
 * The slide is not token-based — the render function draws directly.
 * @param size Slide dimensions
 * @param render Custom render callback
 * @param update Custom update callback (may be NULL)
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildHistorySlide(Dimensions size,
                             void (*render)(AppData*, SlideDef*),
                             void (*update)(AppData*, SlideDef*));

/**
 * @brief Build a token-based history slide from formatted text.
 * @param text Token-format text with escape sequences
 * @param size Slide dimensions
 * @return Array of 1 SlideDef pointer, or NULL on failure
 */
SlideDef** BuildHistoryTextSlide(const char* text, Dimensions size,
                                 int slide_type);

#endif /* UI_H_ */
