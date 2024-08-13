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

/* Structure for storing a single history node */
struct HistoryNode {
  HistoryNode *next; /* Pointer to the next node in the history stack */
  int scene;         /* Identifier for the stored scene */
};

/* Structure for managing history stacks */
struct History {
  HistoryNode *future_stack; /* Stack of future scenes for redo operations */
  HistoryNode *past_stack;   /* Stack of past scenes for undo operations */
  int present;               /* Identifier of the current scene */
};

/* Structure for storing properties of a panel */
struct Panel {
  History *scene_history; /* Pointer to the history management struct */
  Dimensions size;        /* Dimensions of the panel */
  Vector2D position;      /* Position of the panel on the screen */
  int mode;               /* Mode or state of the panel */
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
  const char *top_left;     /* Character for the top-left corner */
  const char *top_right;    /* Character for the top-right corner */
  const char *bottom_left;  /* Character for the bottom-left corner */
  const char *bottom_right; /* Character for the bottom-right corner */
  const char *horizontal;   /* Character for the horizontal border line */
  const char *vertical;     /* Character for the vertical border line */
};

/* Create a screen struct with MAX_PANELS in horizontal rows */
Screen *CreateScreen();

/* Free all memory associated with a Screen struct */
void FreeScreen(Screen *screen);

/* Create a screen struct with the current screen size and MAX_PANELS */
Panel CreatePanel(Dimensions size, Vector2D position);

/* Function to free the memory of a panel */
void FreePanel(Panel *panel);

/* Render a border in a given panel */
void RenderPanelBorder(Panel panel, Border border);

/* Update panels from a given screen */
void UpdateScreen(Screen *screen);

/* Update panel dimensions and position */
void UpdatePanel(Panel *panel, Dimensions size, Vector2D position);

/* Render content at panel center */
void RenderAtPanelCenter(Panel panel, const char *content, Vector2D offset);

/* Render animation at panel center */
void RenderAnimationAtPanelCenter(Panel panel, Rollfilm animation,
                                  Vector2D offset);

/* Render screen size error */
void RenderScreenSizeError(Screen *screen, Panel panel);

/* Function to push a scene onto a stack */
void PushHistory(HistoryNode **stack, int scene);

/* Function to pop a scene from a stack */
int PopHistory(HistoryNode **stack);

/* Function to clear a stack */
void ClearStack(HistoryNode **stack);

/* Function to create and initialize a new history */
History *CreateHistory();

/* Function to free the memory of a history */
void FreeHistory(History *history);

/* Function to perform undo operation */
void UndoHistory(History *history);

/* Function to perform redo operation */
void RedoHistory(History *history);

/* Function to perform do operation */
void ExecuteHistory(History *history, int new_scene);

#endif /* UI_H_ */
