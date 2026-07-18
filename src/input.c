#include "input.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

/* macOS ncurses does not define BUTTON4/5_PRESSED (mouse wheel buttons). */
#ifndef BUTTON4_PRESSED
#define BUTTON4_PRESSED 0000010L
#endif
#ifndef BUTTON5_PRESSED
#define BUTTON5_PRESSED 0000020L
#endif

#include "anim.h"
#include "audio.h"
#include "config.h"
#include "draw.h"
#include "error.h"
#include "log.h"
#include "notes.h"
#include "notify.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* ---------------------------------------------------------------------------
 * Public History Navigation Functions
 * --------------------------------------------------------------------------- */

/* PRIVATE INPUT FUNCTIONS */
/* Mouse Handlers */
static void handleMousePanel(AppData* app, MEVENT* event);
static void handleMouseNotes(AppData* app, MEVENT* event);
static void handleMousePanelSwitch(AppData* app, MEVENT* event);
static void handleMousePopup(AppData* app, MEVENT* event);
static void handleMouseCancelEdit(AppData* app, MEVENT* event);
/* History */
static void historyRebuildOverview(AppData* app);
static void historyMoveCursorByDays(AppData* app, int dayDelta);
/* Preferences */
static int prefFieldIndexBySelectOffset(AppData* app, int sel_off);

/**
 * ---------------------------------------------------------------------------
 * Input Dispatching
 * ---------------------------------------------------------------------------
 */

/**
 * Process a single key input and dispatch to appropriate handler.
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 */
void ProcessKeyInput(AppData* app, int key) {
  size_t numKeyFunctions = g_config.num_keys;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  int effective_mode = current_mode;

  /* Only switch to DEFAULT if mode is NORMAL and input is empty */
  if (current_mode == NORMAL && (!input || input->len == 0))
    effective_mode = DEFAULT;

  /* Handle printable characters in INSERT mode via special entry */
  if (effective_mode == INSERT && key >= ' ' && key <= '~') {
    for (size_t i = 0; i < numKeyFunctions; i++) {
      if (keys[i].key == -1 && (keys[i].modes & effective_mode) &&
          (keys[i].scene_types & (1 << current_scene))) {
        keys[i].action(app);
        return;
      }
    }
  }

  /* Normal lookup */
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key && (keys[i].modes & effective_mode) &&
        (keys[i].scene_types & (1 << current_scene))) {
      keys[i].action(app);
      return;
    }
  }
}

/**
 * Check if the given key is assigned to the specified action function.
 * @param key The key code to check
 * @param action Function pointer to compare against
 * @return 1 if key is assigned to action, 0 otherwise
 */
int IsKeyAssignedToAction(int key, void (*action)(AppData*)) {
  size_t numKeyFunctions = g_config.num_keys;
  for (size_t i = 0; i < numKeyFunctions; i++)
    if (keys[i].key == key && keys[i].action == action) return 1;
  return 0;
}

/**
 * Handle all user input based on current app state.
 * Reads input from terminal and routes to appropriate handler.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInputs(AppData* app) {
  ErrorType status = NO_ERROR;
  ESCDELAY = 25;

  if (app->user_input != -1) app->last_input = app->user_input;
  app->user_input = getch();

  if (app->frozen) {
    if (app->popup_dialog) HandlePopupInput(app, app->user_input);
    return status;
  }

  int key = app->user_input;
  /* Handle mouse events — drain queue, process buttons from the first
   * event so click/scroll are never overwritten by a subsequent
   * REPORT_MOUSE_POSITION, then store the last event for hover tracking. */
  if (key == KEY_MOUSE) {
    MEVENT event, last_event, button_event;
    bool has_button = false;
    bool valid = false;
    while (getmouse(&event) == OK) {
      if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON4_PRESSED |
                          BUTTON5_PRESSED)) {
        button_event = event;
        has_button = true;
      }
      last_event = event;
      valid = true;

      key = getch();
      if (key != KEY_MOUSE) {
        if (key != ERR) ungetch(key);
        break;
      }
    }
    if (valid) {
      if (has_button) {
        app->mouse_x = button_event.x;
        app->mouse_y = button_event.y;
        app->mouse_bstate = button_event.bstate;
        ErrorType merr = HandleMouseEvent(app, &button_event);
        if (merr != NO_ERROR) {
          status = merr;
          SetError(app, "HandleInputs", merr);
        }
      }
      app->mouse_x = last_event.x;
      app->mouse_y = last_event.y;
      app->mouse_bstate = last_event.bstate;
      if (!has_button) {
        ErrorType merr = HandleMouseEvent(app, &last_event);
        if (merr != NO_ERROR) {
          status = merr;
          SetError(app, "HandleInputs", merr);
        }
      }
    }
    flushinp();
    app->user_input = -1;
    return status;
  }

  int current_mode = app->screen->panels[app->screen->current_panel].mode;

  /* Dispatch to mode-specific handler */
  switch (current_mode) {
    case DEFAULT:
      status = HandleDefaultMode(app, key);
      break;
    case NORMAL:
      status = HandleNormalMode(app, key);
      break;
    case INSERT:
      status = HandleInsertMode(app, key);
      break;
    case VISUAL:
      status = HandleVisualMode(app, key);
      break;
  }
  flushinp();

  return status;
}

/**
 * ---------------------------------------------------------------------------
 * Mouse Handlers
 * ---------------------------------------------------------------------------
 */

/**
 * Handle mouse events. In DEFAULT mode: hover updates menu selection +
 * switches panel focus on REQUEST_MOUSE_POSITION, clicks execute actions or
 * switch panels. In non-DEFAULT mode: movement is ignored, first click goes
 * directly to DEFAULT mode.
 * @param app Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleMouseEvent(AppData* app, MEVENT* event) {
  ErrorType status = NO_ERROR;
  if (app->screen->panels[app->screen->current_panel].mode != DEFAULT) {
    handleMouseCancelEdit(app, event);
    return status;
  }
  if (app->popup_dialog) {
    handleMousePopup(app, event);
    return status;
  }
  handleMousePanelSwitch(app, event);
  handleMouseNotes(app, event);
  handleMousePanel(app, event);
  return status;
}

/**
 * Handle mouse cancel of edit mode.
 * If the user clicks while in a non-DEFAULT mode (INSERT, NORMAL, VISUAL),
 * the input is cleared and the panel returns to DEFAULT mode.
 * @param app   Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
static void handleMouseCancelEdit(AppData* app, MEVENT* event) {
  if (!(event->bstate & BUTTON1_PRESSED)) return;
  Panel* p = &app->screen->panels[app->screen->current_panel];
  InputState* input = p->input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->pending_parent_id = -1;
  }
  if (app->notes && app->notes->count > 0)
    app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
  p->mode = DEFAULT;
  noecho();
  curs_set(0);
  app->user_input = -1;
  app->last_input = -1;
  move(LINES - 1, 0);
  clrtoeol();
  refresh();
}

/**
 * Handle mouse events on the popup dialog.
 * Routes clicks to the correct action based on click region,
 * closes the dialog on outside clicks (except for welcome/continue slides).
 * For slide-based dialogs it forwards mouse movement to slide.update().
 * @param app   Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
static void handleMousePopup(AppData* app, MEVENT* event) {
  bool is_click = (event->bstate == BUTTON1_PRESSED);

  bool inside_popup = false;
  {
    Vector2D pos = app->popup_dialog->position;
    Dimensions size = app->popup_dialog->size;
    inside_popup = (event->x >= pos.x && event->x < pos.x + size.width &&
                    event->y >= pos.y && event->y < pos.y + size.height);
  }
  if (is_click && !inside_popup &&
      app->popup_dialog->slide_type != SLIDE_TYPE_WELCOME &&
      app->popup_dialog->slide_type != SLIDE_TYPE_CONTINUE &&
      app->popup_dialog->slide_type != SLIDE_TYPE_NOISE) {
    ClosePopup(app);
    return;
  }
  if (inside_popup) {
    if (app->popup_dialog->slide_type == SLIDE_TYPE_NOISE) {
      FloatingDialog* d = app->popup_dialog;
      if (!d || !d->slides || !d->slides[0]) return;
      SlideDef* def = d->slides[0];
      if (event->bstate & REPORT_MOUSE_POSITION) {
        def->update(app, def);
        flushinp();
      }
      if (is_click || (event->bstate & (BUTTON4_PRESSED | BUTTON5_PRESSED))) {
        NoiseSlideMouseAction(app, event, is_click);
        if (!app->popup_dialog) return;
        /* Check registered click regions (e.g. "q Close") */
        if (is_click) {
          for (int i = 0; i < app->click_region_count; i++) {
            ClickRegion* r = &app->click_regions[i];
            if (r->type != REGION_SLIDE_NAV) continue;
            if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
                event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
              if (r->action) r->action(app);
              break;
            }
          }
        }
      }
      if (is_click || event->bstate == BUTTON1_RELEASED) def->hovered = -1;
      return;
    }
    /* History Overview — grid cell clicks and nav hint clicks */
    if (app->popup_dialog->slide_type == SLIDE_TYPE_HISTORY_OVERVIEW) {
      FloatingDialog* d = app->popup_dialog;
      if (!d->slides || !d->slides[0]) return;
      SlideDef* def = d->slides[0];
      if (event->bstate & REPORT_MOUSE_POSITION) {
        def->update(app, def);
      }
      if (is_click) {
        /* Check nav hint regions first */
        for (int i = 0; i < app->click_region_count; i++) {
          ClickRegion* r = &app->click_regions[i];
          if (r->type != REGION_SLIDE_NAV) continue;
          if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
              event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
            if (r->action) r->action(app);
            return;
          }
        }
        /* Check grid area: compute week column and DOW from mouse position */
        int gx = d->position.x + 2 + 3 + 1; /* x + border + dayLabelW + space */
        int gy = d->position.y + 4;         /* grid top row */
        if (event->y >= gy && event->y < gy + 7 && event->x >= gx) {
          int col = (event->x - gx) / 3;
          int dow = event->y - gy;
          if (col >= 0 && dow >= 0 && dow < 7) {
            HistoryData* h = &app->history_data;
            /* If clicking same cell already selected, open day detail */
            if (col == h->cursorWeek && dow == h->cursorDow && h->selDay > 0) {
              HistoryOpenDayDetail(app);
            } else {
              h->cursorDow = dow;
              h->cursorWeek = col;
              HistoryResolveCursor(app);
              CreateHistoryOverviewDialog(app);
            }
          }
        }
      }
      return;
    }
    if (app->popup_dialog->slide_type == SLIDE_TYPE_PREFERENCES) {
      FloatingDialog* d = app->popup_dialog;
      if (!d->slides || !d->slides[0]) return;
      SlideDef* def = d->slides[0];
      if (event->bstate & REPORT_MOUSE_POSITION) def->update(app, def);
      if (event->bstate & BUTTON4_PRESSED)
        PrefsScrollUp(app);
      else if (event->bstate & BUTTON5_PRESSED)
        PrefsScrollDown(app);
      if (is_click) {
        for (int i = 0; i < app->click_region_count; i++) {
          ClickRegion* r = &app->click_regions[i];
          if (r->type != REGION_SLIDE_NAV) continue;
          if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
              event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
            if (r->action) r->action(app);
            break;
          }
        }
      }
      return;
    }
    if (app->popup_dialog->slide_type == SLIDE_TYPE_HELP) {
      FloatingDialog* d = app->popup_dialog;
      if (!d->slides || !d->slides[0]) return;
      SlideDef* def = d->slides[0];
      if (event->bstate & REPORT_MOUSE_POSITION) def->update(app, def);
      if (event->bstate & BUTTON4_PRESSED)
        HelpScrollUp(app);
      else if (event->bstate & BUTTON5_PRESSED)
        HelpScrollDown(app);
      if (is_click) {
        for (int i = 0; i < app->click_region_count; i++) {
          ClickRegion* r = &app->click_regions[i];
          if (r->type != REGION_SLIDE_NAV) continue;
          if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
              event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
            if (r->action) r->action(app);
            break;
          }
        }
      }
      return;
    }
    if (app->popup_dialog->slide_type == SLIDE_TYPE_WELCOME ||
        app->popup_dialog->slide_type == SLIDE_TYPE_CONTINUE) {
      FloatingDialog* d = app->popup_dialog;
      int stride = d->slideCount / 3;
      int icon_type = GetConfigIconType();
      if (!d->slides) return;
      SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
      if (!def) return;
      if (event->bstate & REPORT_MOUSE_POSITION) {
        def->update(app, def);
      }
      if (is_click) {
        def->hovered = -1;
        for (int i = 0; i < app->click_region_count; i++) {
          ClickRegion* r = &app->click_regions[i];
          if (r->type != REGION_SLIDE_NAV) continue;
          if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
              event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
            if (r->action) r->action(app);
            break;
          }
        }
      }
      if (app->popup_dialog &&
          (app->popup_dialog->slide_type == SLIDE_TYPE_WELCOME ||
           app->popup_dialog->slide_type == SLIDE_TYPE_CONTINUE) &&
          (is_click || event->bstate == BUTTON1_RELEASED)) {
        def = d->slides[icon_type * stride + d->currentSlide];
        def->hovered = -1;
      }
      return;
    }
    if (app->popup_dialog->slide_type == SLIDE_TYPE_PREFS_SELECT) {
      FloatingDialog* d = app->popup_dialog;
      if (!d->slides || !d->slides[0]) return;
      SlideDef* def = d->slides[0];
      if (event->bstate & REPORT_MOUSE_POSITION) def->update(app, def);
      if (is_click) {
        for (int i = 0; i < app->click_region_count; i++) {
          ClickRegion* r = &app->click_regions[i];
          if (r->type != REGION_SLIDE_NAV) continue;
          if (event->x >= r->pos.x && event->x < r->pos.x + r->size.width &&
              event->y >= r->pos.y && event->y < r->pos.y + r->size.height) {
            if (r->action) r->action(app);
            break;
          }
        }
      }
      return;
    }
    for (int i = 0; i < app->click_region_count; i++) {
      ClickRegion* r = &app->click_regions[i];
      if (event->x < r->pos.x || event->x >= r->pos.x + r->size.width) continue;
      if (event->y < r->pos.y || event->y >= r->pos.y + r->size.height)
        continue;
      if (r->type == REGION_POPUP_ITEM && app->popup_dialog &&
          r->item_index < app->popup_dialog->menu.item_count) {
        if (event->bstate & REPORT_MOUSE_POSITION)
          app->popup_dialog->menu.selected_item = r->item_index;
        else if (is_click) {
          app->popup_dialog->menu.selected_item = r->item_index;
          ExecuteMenuAction(app);
        }
      } else if (r->type == REGION_SLIDE_NAV && is_click && r->action) {
        r->action(app);
      }
      break;
    }
  }
}

/**
 * Handle mouse hover to switch the active panel.
 * When the cursor moves over a visible panel that is not the current
 * panel, the focus switches to that panel.
 * @param app   Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
static void handleMousePanelSwitch(AppData* app, MEVENT* event) {
  for (int i = 0; i < MAX_PANELS; i++) {
    Panel* p = &app->screen->panels[i];
    if (p->visible && i != app->screen->current_panel &&
        event->x >= p->position.x && event->x < p->position.x + p->size.width &&
        event->y >= p->position.y &&
        event->y < p->position.y + p->size.height) {
      app->screen->current_panel = i;
      app->current_menu = p->menu_index;
      break;
    }
  }
}

/**
 * Handle mouse events on the notes panel.
 * Supports click selection, drag-to-reorder, and toggle-task on release.
 * @param app   Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
static void handleMouseNotes(AppData* app, MEVENT* event) {
  bool b1_press = event->bstate & BUTTON1_PRESSED;
  bool b1_release = event->bstate & BUTTON1_RELEASED;

  int hit_note_id = -1;
  if (app->notes) {
    for (int i = 0; i < app->click_region_count; i++) {
      ClickRegion* r = &app->click_regions[i];
      if (event->x < r->pos.x || event->x >= r->pos.x + r->size.width) continue;
      if (event->y < r->pos.y || event->y >= r->pos.y + r->size.height)
        continue;
      if (r->type == REGION_NOTE_ITEM && r->note_id >= 0) {
        hit_note_id = r->note_id;
        break;
      }
    }
  }

  if (app->notes && app->notes->drag_note_id >= 0 &&
      (event->bstate & REPORT_MOUSE_POSITION)) {
    if (event->y <= app->notes->drag_start_y - 1) {
      app->notes->drag_moved = true;
      app->notes->is_move_mode = true;
      MoveNoteUp(app->notes);
      app->notes->drag_start_y = event->y;
    } else if (event->y >= app->notes->drag_start_y + 1) {
      app->notes->drag_moved = true;
      app->notes->is_move_mode = true;
      MoveNoteDown(app->notes);
      app->notes->drag_start_y = event->y;
    }
    return;
  }

  if ((event->bstate & REPORT_MOUSE_POSITION) && !b1_press &&
      hit_note_id >= 0) {
    app->notes->current_id = hit_note_id;
    return;
  }

  if (b1_press && hit_note_id >= 0) {
    app->notes->current_id = hit_note_id;
    app->notes->drag_note_id = hit_note_id;
    app->notes->drag_start_y = event->y;
    app->notes->drag_moved = false;
    return;
  }

  if (b1_release && app->notes) {
    app->notes->is_move_mode = false;
    if (!app->notes->drag_moved) {
      int target_id = (app->notes->drag_note_id >= 0) ? app->notes->drag_note_id
                                                      : hit_note_id;
      if (target_id >= 0) {
        app->notes->current_id = target_id;
        ToggleTask(app->notes);
      }
    }
    app->notes->drag_note_id = -1;
    app->notes->drag_moved = false;
  }
}

/**
 * Handle mouse events on panel content (menus, direct-action buttons).
 * Dispatches hover updates for menu items and executes actions on click.
 * @param app   Pointer to the application data
 * @param event Pointer to the ncurses mouse event
 */
static void handleMousePanel(AppData* app, MEVENT* event) {
  bool b1_press = event->bstate & BUTTON1_PRESSED;

  if ((event->bstate & REPORT_MOUSE_POSITION) && !b1_press) {
    for (int i = 0; i < app->click_region_count; i++) {
      ClickRegion* r = &app->click_regions[i];
      if (event->x < r->pos.x || event->x >= r->pos.x + r->size.width) continue;
      if (event->y < r->pos.y || event->y >= r->pos.y + r->size.height)
        continue;

      if (r->type == REGION_MENU_ITEM && r->menu_index >= 0 &&
          r->menu_index < MAX_MENUS &&
          r->item_index < app->menus[r->menu_index]->item_count) {
        app->current_menu = r->menu_index;
        app->menus[r->menu_index]->selected_item = r->item_index;
      } else if (r->type == REGION_POPUP_ITEM && app->popup_dialog &&
                 r->item_index < app->popup_dialog->menu.item_count)
        app->popup_dialog->menu.selected_item = r->item_index;
      break;
    }
    return;
  }

  if (b1_press) {
    for (int i = 0; i < app->click_region_count; i++) {
      ClickRegion* r = &app->click_regions[i];
      if (event->x < r->pos.x || event->x >= r->pos.x + r->size.width) continue;
      if (event->y < r->pos.y || event->y >= r->pos.y + r->size.height)
        continue;

      if (r->type == REGION_DIRECT) {
        if (app->notes) {
          app->notes->drag_note_id = -1;
          app->notes->drag_moved = false;
        }
        if (r->action) r->action(app);
      } else if (r->type == REGION_MENU_ITEM && r->menu_index >= 0 &&
                 r->menu_index < MAX_MENUS &&
                 r->item_index < app->menus[r->menu_index]->item_count) {
        if (app->notes) {
          app->notes->drag_note_id = -1;
          app->notes->drag_moved = false;
        }
        app->current_menu = r->menu_index;
        app->menus[r->menu_index]->selected_item = r->item_index;
        ExecuteMenuAction(app);
      } else if (r->type == REGION_POPUP_ITEM && app->popup_dialog &&
                 r->item_index < app->popup_dialog->menu.item_count) {
        if (app->notes) {
          app->notes->drag_note_id = -1;
          app->notes->drag_moved = false;
        }
        app->popup_dialog->menu.selected_item = r->item_index;
        ExecuteMenuAction(app);
      }
      break;
    }
  }
}

/**
 * ---------------------------------------------------------------------------
 * Keyboard Handlers
 * ---------------------------------------------------------------------------
 */

/**
 * Handle input in DEFAULT mode (menu navigation, pomodoro control).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleDefaultMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;

  /* First handle popup input if popup is active */
  if (HandlePopupInput(app, key)) {
    return status; /* popup key was handled */
  }

  if (!ValidateAndRenderScreenSize(app)) {
    if (IsKeyAssignedToAction(key, QuitApp))
      ProcessKeyInput(app, key);
    else
      app->user_input = -1;
  } else if (!app->block_input)
    ProcessKeyInput(app, key);

  return status;
}

/**
 * Handle input in NORMAL mode (text navigation, note editing).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleNormalMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  Panel* panel = &app->screen->panels[app->screen->current_panel];
  InputState* input = panel->input;

  /* If Panel.input exists with content, process editing keys */
  if (input && input->len > 0) {
    ProcessKeyInput(app, key);
    return status;
  }

  /* If input is empty in NORMAL mode, fall through to DEFAULT handling */
  if (!app->block_input) ProcessKeyInput(app, key);

  return status;
}

/**
 * Handle input in INSERT mode (text input).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleInsertMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/**
 * Handle input in VISUAL mode (text selection).
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType HandleVisualMode(AppData* app, int key) {
  ErrorType status = NO_ERROR;
  ProcessKeyInput(app, key);
  return status;
}

/**
 * Handle input in while POPUP is active.
 * @param app Pointer to the application data
 * @param key The key code that was pressed
 * @return true if input is consumed, or false if not popup active
 */
bool HandlePopupInput(AppData* app, int key) {
  if (app->popup_dialog == NULL) return 0;

  /* Universal help key — always opens help regardless of keybindings */
  if (key == '?' || key == KEY_F(1)) {
    OpenHelp(app);
    return true;
  }

  /* White noise dialog — dispatch via configurable key bindings */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_NOISE) {
    size_t numKeyFunctions = g_config.num_keys;
    for (size_t i = 0; i < numKeyFunctions; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_NOISE)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while noise dialog is active */
  }

  /* Preferences dialog — dispatch via configurable key bindings */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_PREFERENCES) {
    size_t numKeyFunctions = g_config.num_keys;
    for (size_t i = 0; i < numKeyFunctions; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_PREFERENCES)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while preferences dialog is active */
  }

  /* Preferences stepper sub-dialog — dispatch via configurable key bindings */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_PREFS_STEPPER) {
    size_t nk = g_config.num_keys;
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_PREFS_STEPPER)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while stepper is active */
  }

  /* Preferences select sub-dialog — dispatch via configurable key bindings */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_PREFS_SELECT) {
    size_t nk = g_config.num_keys;
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_PREFS_SELECT)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while select is active */
  }

  /* Welcome dialog — LEFT/RIGHT selects options, ENTER executes */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_WELCOME) {
    if (IsKeyAssignedToAction(key, SelectPrevButton)) {
      SelectPrevButton(app);
      return true;
    }
    if (IsKeyAssignedToAction(key, SelectNextButton)) {
      SelectNextButton(app);
      return true;
    }
    if (IsKeyAssignedToAction(key, ExecuteButtonAction)) {
      ExecuteButtonAction(app);
      return true;
    }
    if (IsKeyAssignedToAction(key, ClosePopup)) {
      ClosePopup(app);
      return true;
    }
    return true; /* consume all keys while a slide dialog is active */
  }

  /* Continue dialog — dispatch via configurable key bindings (noise pattern) */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_CONTINUE) {
    size_t nk = g_config.num_keys;
    /* Pass 1 — scene-specific only (not bare ALL_SCENES) */
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_CONTINUE) &&
          keys[i].scene_types != ALL_SCENES) {
        keys[i].action(app);
        return true;
      }
    }
    /* Pass 2 — ALL_SCENES fallback */
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & ALL_SCENES)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while continue dialog is active */
  }

  /* History popups — dispatch via configurable key bindings (noise pattern).
   * Two-pass approach: first pass matches scene-specific keys only (ignoring
   * ALL_SCENES bindings like QuitApp), second pass falls back to ALL_SCENES. */
  {
    int sceneMask = 0;
    SlideType st = app->popup_dialog->slide_type;
    if (st == SLIDE_TYPE_HISTORY_OVERVIEW)
      sceneMask = SCENE_HISTORY_OVERVIEW;
    else if (st == SLIDE_TYPE_HISTORY_DAY)
      sceneMask = SCENE_HISTORY_DAY;
    else if (st == SLIDE_TYPE_HISTORY_STATS)
      sceneMask = SCENE_HISTORY_STATS;
    if (sceneMask) {
      size_t nk = g_config.num_keys;

      /* Pass 1 — scene-specific only (not bare ALL_SCENES) */
      for (size_t i = 0; i < nk; i++) {
        if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
            (keys[i].scene_types & sceneMask) &&
            keys[i].scene_types != ALL_SCENES) {
          keys[i].action(app);
          return true;
        }
      }
      /* Pass 2 — ALL_SCENES fallback (e.g. resize, close) */
      for (size_t i = 0; i < nk; i++) {
        if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
            (keys[i].scene_types & ALL_SCENES)) {
          keys[i].action(app);
          return true;
        }
      }
      return true; /* consume all keys while a history popup is active */
    }
  }

  /* Help dialog — dispatch via configurable key bindings */
  if (app->popup_dialog->slide_type == SLIDE_TYPE_HELP) {
    size_t nk = g_config.num_keys;
    /* Pass 1 — SCENE_HELP-specific only (not bare ALL_SCENES) */
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & SCENE_HELP) &&
          keys[i].scene_types != ALL_SCENES) {
        keys[i].action(app);
        return true;
      }
    }
    /* Pass 2 — ALL_SCENES fallback */
    for (size_t i = 0; i < nk; i++) {
      if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
          (keys[i].scene_types & ALL_SCENES)) {
        keys[i].action(app);
        return true;
      }
    }
    return true; /* consume all keys while help is active */
  }

  /* When popup is active, only use keys bound to ALL_SCENES to avoid
   * scene-specific keys (like ToggleTaskAtNotes) intercepting popup input */
  size_t numKeyFunctions = g_config.num_keys;
  for (size_t i = 0; i < numKeyFunctions; i++) {
    if (keys[i].key == key && (keys[i].modes & DEFAULT) &&
        keys[i].scene_types == ALL_SCENES) {
      keys[i].action(app);
      return true; /* handled */
    }
  }
  /* For other keys (like q, ESC), let them pass through */
  return false;
}

/**
 * ---------------------------------------------------------------------------
 * InputState Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new InputState with default values.
 * @return Pointer to the created InputState, or NULL on allocation failure
 */
InputState* InputStateCreate(void) {
  InputState* s = (InputState*)malloc(sizeof(InputState));
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->max_len = sizeof(s->buffer) - 1;
    s->is_task = true;
    s->pending_parent_id = -1;
    s->insert_after_id = -1;
    s->selection.start = 0;
    s->selection.end = 0;
  }
  return s;
}

/**
 * Destroy an InputState and free its memory.
 * @param input Pointer to the InputState pointer to free
 */
void InputStateDestroy(InputState** input) {
  if (input && *input) {
    free(*input);
    *input = NULL;
  }
}

/**
 * Clear the InputState contents, resetting cursor and buffer.
 * @param s Pointer to the InputState to clear
 */
void InputStateClear(InputState* s) {
  if (s) {
    s->buffer[0] = '\0';
    s->len = 0;
    s->cursor = 0;
    s->selection.start = 0;
    s->selection.end = 0;
  }
}

/**
 * ---------------------------------------------------------------------------
 * Mode Management
 * ---------------------------------------------------------------------------
 */

/**
 * Set the input mode of a panel, updating the panel's mode field.
 * Centralized mode transition that handles mode switching logic.
 * @param panel Pointer to the panel to update
 * @param mode The new input mode (DEFAULT, NORMAL, INSERT, VISUAL)
 */
void InputSetMode(Panel* panel, InputMode mode) {
  panel->mode = mode;

  if (panel->input) {
    int max = (mode == INSERT)
                ? panel->input->len
                : (panel->input->len > 0 ? panel->input->len - 1 : 0);
    if (panel->input->cursor > max) {
      panel->input->cursor = max;
    }

    if (mode == INSERT) {
      echo();
      curs_set(1);
    } else {
      noecho();
      curs_set(0);
    }

    if (mode != VISUAL) {
      panel->input->selection.start = 0;
      panel->input->selection.end = 0;
    }
  }
}

/**
 * Switch to INSERT mode from current position.
 * @param app Pointer to the application data
 */
void SwitchToInsertMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Switch to INSERT mode after current cursor position.
 * @param app Pointer to the application data
 */
void SwitchToInsertModeAppend(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor < input->len) input->cursor++;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Enter VISUAL mode for text selection.
 * @param app Pointer to the application data
 */
void SwitchToVisualMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  app->screen->panels[app->screen->current_panel].mode = VISUAL;
  if (input) input->selection.start = input->cursor;
}

/**
 * Exit to NORMAL mode (ESC key handler).
 * @param app Pointer to the application data
 */
void SwitchToNormalMode(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
  /* Clear input state to prevent quit popup from appearing */
  app->user_input = -1;
  app->last_input = -1;
}

/**
 * ---------------------------------------------------------------------------
 * Editor Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move cursor one position to the left.
 * @param app Pointer to the application data
 */
void InputCursorLeft(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && input->cursor > 0) input->cursor--;
}

/**
 * Move cursor one position to the right.
 * @param app Pointer to the application data
 */
void InputCursorRight(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  int mode = app->screen->panels[app->screen->current_panel].mode;
  if (mode == INSERT) {
    if (input->cursor < input->len) input->cursor++;
  } else {
    if (input->len > 0 && input->cursor < input->len - 1) input->cursor++;
  }
}

/**
 * Delete character before cursor (backspace).
 * @param app Pointer to the application data
 */
void InputBackspace(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  if (input->cursor > 0) {
    for (int i = input->cursor - 1; i < input->len - 1; i++)
      input->buffer[i] = input->buffer[i + 1];
    input->cursor--;
    input->len--;
    input->buffer[input->len] = '\0';
  }
}

/**
 * Delete character at cursor (in NORMAL mode).
 * @param app Pointer to the application data
 */
void InputDeleteChar(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  SaveNotesToHistory(app->notes, input->cursor);

  if (input->cursor < input->len) {
    for (int i = input->cursor; i < input->len - 1; i++)
      input->buffer[i] = input->buffer[i + 1];
    input->len--;
    input->buffer[input->len] = '\0';
    int mode = app->screen->panels[app->screen->current_panel].mode;
    int max_cursor =
      (mode == INSERT) ? input->len : (input->len > 0 ? input->len - 1 : 0);
    if (input->cursor > max_cursor) input->cursor = max_cursor;
  }
  if (input->len == 0) {
    input->cursor = 0;
    input->buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  }
}

/**
 * Delete character(s) in visual selection.
 * @param app Pointer to the application data
 */
void InputVisualDelete(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  SaveNotesToHistory(app->notes, input->cursor);

  int start_sel = (input->selection.start < input->cursor)
                    ? input->selection.start
                    : input->cursor;
  int end_sel = (input->selection.start < input->cursor)
                  ? input->cursor
                  : input->selection.start;
  end_sel++;
  int sel_len = end_sel - start_sel;
  if (sel_len > 0 && start_sel < input->len) {
    for (int i = start_sel; i < input->len - sel_len; i++)
      input->buffer[i] = input->buffer[i + sel_len];
    input->len -= sel_len;
    input->buffer[input->len] = '\0';
    input->cursor = start_sel;
    input->selection.start = start_sel;
    int mode = app->screen->panels[app->screen->current_panel].mode;
    int max_cursor =
      (mode == INSERT) ? input->len : (input->len > 0 ? input->len - 1 : 0);
    if (input->cursor > max_cursor) input->cursor = max_cursor;
  }
  if (input->len == 0) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    app->screen->panels[app->screen->current_panel].mode = INSERT;
    echo();
    curs_set(1);
    refresh();
  } else {
    if (input->cursor > 0 && input->cursor > input->len - 1)
      input->cursor = (input->len > 0) ? input->len - 1 : 0;
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    refresh();
  }
}

/**
 * Commit current input (return/enter key).
 * Finalizes text entry or confirms selection.
 * @param app Pointer to the application data
 */
void InputCommit(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input || input->len == 0) {
    if (app->notes->count > 0 && app->notes->current_id < 0)
      app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
    app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
    return;
  }

  input->buffer[input->len] = '\0';
  NoteState state;
  if (app->notes->current_id >= 0) {
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    state = note ? note->state : (input->is_task ? NOTE_UNDONE : NOTE_PLAIN);
  } else
    state = input->is_task ? NOTE_UNDONE : NOTE_PLAIN;

  if (app->notes->current_id >= 0) {
    app->notes->last_affected_id = app->notes->current_id;
    UpdateNote(app->notes, app->notes->current_id, input->buffer, state);
  } else if (input->insert_after_id >= 0) {
    bool valid = false;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == input->insert_after_id &&
          app->notes->items[i]->page_id == app->notes->current_page) {
        valid = true;
        break;
      }
    }
    if (!valid) {
      input->insert_after_id = -1;
      app->notes->last_affected_id = -1;
      AddNote(app, app->notes, input->buffer, state);
      app->notes->last_affected_id = app->notes->current_id;
    } else {
      app->notes->last_affected_id = -1;
      AddNoteAfter(app, app->notes, input->insert_after_id, input->buffer,
                   state);
      app->notes->last_affected_id = app->notes->current_id;
    }
  } else if (input->pending_parent_id >= 0) {
    app->notes->last_affected_id = input->pending_parent_id;
    AddChildNote(app, app->notes, input->pending_parent_id, input->buffer,
                 state);
    app->notes->last_affected_id = app->notes->current_id;
  } else {
    app->notes->last_affected_id = -1;
    AddNote(app, app->notes, input->buffer, state);
    app->notes->last_affected_id = app->notes->current_id;
  }
  input->len = 0;
  input->cursor = 0;
  input->buffer[0] = '\0';
  input->is_task = true;
  input->pending_parent_id = -1;
  input->insert_after_id = -1;
  app->screen->panels[app->screen->current_panel].mode = DEFAULT;
  app->user_input = -1;
  app->last_input = -1;
  move(LINES - 1, 0);
  clrtoeol();
  refresh();
}

/**
 * Handle escape key - exit to DEFAULT mode or close dialog.
 * @param app Pointer to the application data
 */
void InputESC(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == INSERT) {
    if (input && input->cursor > 0 && input->cursor > input->len - 1)
      input->cursor = (input->len > 0) ? input->len - 1 : 0;
    if (!input || input->len == 0) {
      if (input) {
        input->len = 0;
        input->cursor = 0;
        input->buffer[0] = '\0';
        input->pending_parent_id = -1;
      }
      if (app->notes->count > 0)
        app->notes->current_id = app->notes->items[app->notes->count - 1]->id;
      app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    } else {
      app->screen->panels[app->screen->current_panel].mode = NORMAL;
    }
    noecho();
    curs_set(0);
    app->user_input = -1;
    app->last_input = -1;
  } else if (current_mode == VISUAL) {
    app->screen->panels[app->screen->current_panel].mode = NORMAL;
    noecho();
    curs_set(0);
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  } else if (current_mode == NORMAL) {
    if (input) {
      input->len = 0;
      input->cursor = 0;
      input->buffer[0] = '\0';
    }
    app->screen->panels[app->screen->current_panel].mode = DEFAULT;
    app->user_input = -1;
    app->last_input = -1;
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
  }
}

/**
 * Insert a printable character at cursor position.
 * @param app Pointer to the application data
 */
void InputInsertChar(AppData* app) {
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;
  int key = app->user_input;
  if (key >= ' ' && key <= '~' && input->len < input->max_len) {
    for (int i = input->len; i > input->cursor; i--)
      input->buffer[i] = input->buffer[i - 1];
    input->buffer[input->cursor] = key;
    input->cursor++;
    input->len++;
    input->buffer[input->len] = '\0';
  }
}

/**
 * Switch from VISUAL mode to INSERT mode, keeping selection.
 * @param app Pointer to the application data
 */
void InputSwitchToInsertFromVisual(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->panels[app->screen->current_panel].mode = INSERT;
  noecho();
  curs_set(1);
  refresh();
}

/**
 * ---------------------------------------------------------------------------
 * App Control Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Switch focus to the next panel.
 * @param app Pointer to the application data
 */
void NextPanel(AppData* app) {
  if (app->popup_dialog != NULL) return;
  app->screen->current_panel = (app->screen->current_panel + 1) % MAX_PANELS;
  app->current_menu =
    app->screen->panels[app->screen->current_panel].menu_index;
}

/**
 * Toggle pomodoro timer pause state.
 * @param app Pointer to the application data
 */
void TogglePause(AppData* app) {
  if (app->popup_dialog != NULL) return;
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
  app->is_paused = !app->is_paused;
}

/**
 * Update animation mode for debugging (step through frames).
 * @param app Pointer to the application data
 * @param step Number of frames to advance (can be negative)
 */
void ChangeDebugAnimation(AppData* app, int step) {
  if (app->popup_dialog != NULL) return;
  int* present =
    &app->screen->panels[app->screen->current_panel].scene_history->present;

  *present = (*present + step + MAX_ANIMATIONS) % MAX_ANIMATIONS;
}

/**
 * Quit the program with confirmation dialog.
 * @param app Pointer to the application data
 */
void QuitApp(AppData* app) {
  if (app->notes && app->notes->is_move_mode) return;
  if (app->user_input == app->last_input) app->running = false;
}

/**
 * Quit the program immediately without confirmation.
 * @param app Pointer to the application data
 */
void ForcefullyQuitApp(AppData* app) { app->running = false; }

/**
 * Start a new pomodoro cycle from the current scene.
 * @param app Pointer to the application data
 */
void StartPomodoro(AppData* app) {
  if (app->screen->panels[app->screen->current_panel].scene_history->present !=
      MAIN_MENU)
    return;

  bool has_active_session =
    app->pomodoro_data.session_index > 0 &&
    (app->pomodoro_data.current_step == WORK_TIME ||
     app->pomodoro_data.current_step == SHORT_PAUSE ||
     app->pomodoro_data.current_step == LONG_PAUSE);

  if (has_active_session) {
    ExecuteHistory(app->screen->panels[0].scene_history,
                   app->pomodoro_data.current_step);
    app->screen->panels[0].menu_index = -1;
    app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
    if (app->pomodoro_data.step_start_time == 0)
      app->pomodoro_data.step_start_time = time(NULL);
    app->pomodoro_data.status = 1;
    return;
  }

  ExecuteHistory(app->screen->panels[0].scene_history, WORK_TIME);
  app->screen->panels[0].menu_index = -1;
  app->pomodoro_data.current_step = WORK_TIME;
  app->pomodoro_data.current_cycle = 0;
  app->pomodoro_data.step_start_time = time(NULL);
  app->pomodoro_data.session_start_time = time(NULL);
  app->pomodoro_data.delta_time_ms = GetCurrentTimeMS();
  if (app->pomodoro_data.session_index == 0)
    app->pomodoro_data.session_index = GetLastLogIndexOnly(POMODORO_LOG) + 1;
  app->pomodoro_data.status = 1;

  if (Notify(&app->notification_work) != NO_ERROR)
    SetError(app, "Sending start notification", NOTIFICATION_SEND_ERROR);
}

/**
 * Open the reset pomodoro menu dialog.
 * @param app Pointer to the application data
 */
void OpenResetMenu(AppData* app) {
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;
  RenderResetMenu(app);
}

/**
 * Open the white noise control dialog.
 * Creates the noise popup and initializes playback state if not already open.
 * @param app Pointer to the application data
 */
void OpenNoiseMenu(AppData* app) {
  if (app->popup_dialog) return;
  if (!NOISE_ENABLED) return;
  app->popup_dialog = CreateNoiseDialog(app);
}

/**
 * Close the white noise dialog.
 * Delegates to ClosePopup to free the dialog and clear the reference.
 * @param app Pointer to the application data
 */
void NoiseClose(AppData* app) { ClosePopup(app); }

/**
 * Select the previous noise track (wraps to master at bottom).
 * @param app Pointer to the application data
 */
void NoiseSelectPrev(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->track_count == 0) return;
  nd->selected--;
  if (nd->selected < 0) nd->selected = nd->track_count;
  app->popup_dialog->hovered_button = nd->selected;
}

/**
 * Select the next noise track (wraps to first track from master).
 * @param app Pointer to the application data
 */
void NoiseSelectNext(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->track_count == 0) return;
  nd->selected++;
  if (nd->selected > nd->track_count) nd->selected = 0;
  app->popup_dialog->hovered_button = nd->selected;
}

/**
 * Toggle playback of the currently selected noise track.
 * Starts the track via NoiseStartTrack if turning on and volume > 0,
 * stops via NoiseStopTrack if turning off.
 * @param app Pointer to the application data
 */
void NoiseTogglePlay(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->selected >= 0 && nd->selected < nd->track_count) {
    nd->playing[nd->selected] = !nd->playing[nd->selected];
    if (nd->playing[nd->selected] && nd->volume[nd->selected] > 0)
      NoiseStartTrack(
        nd->selected, nd->tracks[nd->selected].sound_path,
        (float)nd->volume[nd->selected] * (float)nd->master_volume / 10000.0f);
    else
      NoiseStopTrack(nd->selected);
  }
}

/**
 * Increase volume of the selected track or master by 10.
 * Clamps at 100.  Updates the miniaudio playback volume for
 * actively playing tracks.
 * @param app Pointer to the application data
 */
void NoiseVolumeUp(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->track_count == 0) return;
  if (nd->selected == nd->track_count) {
    if (nd->master_volume < 100) nd->master_volume += 10;
    if (nd->master_volume > 100) nd->master_volume = 100;
    for (int i = 0; i < nd->track_count; i++)
      if (nd->playing[i])
        NoiseSetVolume(
          i, (float)nd->volume[i] * (float)nd->master_volume / 10000.0f);
  } else {
    if (nd->volume[nd->selected] < 100) nd->volume[nd->selected] += 10;
    if (nd->volume[nd->selected] > 100) nd->volume[nd->selected] = 100;
    if (nd->playing[nd->selected])
      NoiseSetVolume(nd->selected, (float)nd->volume[nd->selected] *
                                     (float)nd->master_volume / 10000.0f);
  }
}

/**
 * Decrease volume of the selected track or master by 10.
 * Clamps at 0.  Updates the miniaudio playback volume for
 * actively playing tracks.
 * @param app Pointer to the application data
 */
void NoiseVolumeDown(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->track_count == 0) return;
  if (nd->selected == nd->track_count) {
    if (nd->master_volume > 0) nd->master_volume -= 10;
    if (nd->master_volume < 0) nd->master_volume = 0;
    for (int i = 0; i < nd->track_count; i++)
      if (nd->playing[i])
        NoiseSetVolume(
          i, (float)nd->volume[i] * (float)nd->master_volume / 10000.0f);
  } else {
    if (nd->volume[nd->selected] > 0) nd->volume[nd->selected] -= 10;
    if (nd->volume[nd->selected] < 0) nd->volume[nd->selected] = 0;
    if (nd->playing[nd->selected])
      NoiseSetVolume(nd->selected, (float)nd->volume[nd->selected] *
                                     (float)nd->master_volume / 10000.0f);
  }
}

/**
 * Reset all noise tracks to default state.
 * Sets all volumes to 50, master volume to 70, stops all
 * tracks, and selects the first track.
 * @param app Pointer to the application data
 */
void NoiseResetAll(AppData* app) {
  WhiteNoiseData* nd = &app->noise_data;
  if (nd->track_count == 0) return;
  for (int i = 0; i < nd->track_count; i++) {
    nd->volume[i] = 50;
    nd->playing[i] = false;
  }
  nd->master_volume = 70;
  nd->selected = 0;
  app->popup_dialog->hovered_button = 0;
}

/**
 * Reset the current pomodoro step (time only, not cycle).
 * @param app Pointer to the application data
 */
void ResetPomodoroStep(AppData* app) {
  app->pomodoro_data.current_step_time = 0;
}

/**
 * Reset the entire pomodoro cycle (all steps and progress).
 * @param app Pointer to the application data
 */
void ResetPomodoroCycle(AppData* app) { StartPomodoro(app); }

/**
 * Skip the current pomodoro step (with confirmation).
 * @param app Pointer to the application data
 */
void SkipPomodoroStep(AppData* app) {
  if (app->user_input != app->last_input) return;
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;

  ResetInput(app);
  ClosePopup(app);

  int step = app->pomodoro_data.current_step;
  int duration;

  switch (step) {
    case WORK_TIME:
      duration = app->pomodoro_data.work_time;
      break;
    case SHORT_PAUSE:
      duration = app->pomodoro_data.short_pause_time;
      break;
    case LONG_PAUSE:
      duration = app->pomodoro_data.long_pause_time;
      break;
    default:
      return;
  }

  app->pomodoro_data.current_step_time = duration * 60;
}

/**
 * Skip the current pomodoro step without confirmation.
 * @param app Pointer to the application data
 */
void ForcefullySkipPomodoroStep(AppData* app) {
  /* Only work in pomodoro scenes */
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;

  int step = app->pomodoro_data.current_step;
  int duration;

  switch (step) {
    case WORK_TIME:
      duration = app->pomodoro_data.work_time;
      break;
    case SHORT_PAUSE:
      duration = app->pomodoro_data.short_pause_time;
      break;
    case LONG_PAUSE:
      duration = app->pomodoro_data.long_pause_time;
      break;
    default:
      return;
  }

  app->pomodoro_data.current_step_time = duration * 60;
}

/**
 * Return to main menu from a pomodoro session.
 * Saves the session to log and pauses the timer.
 * @param app Pointer to the application data
 */
void ReturnToMainMenu(AppData* app) {
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (!(POMODORO_SCENES & (1 << current_scene))) return;

  if (WORK_LOG) {
    if (SavePomodoro(POMODORO_LOG, &app->pomodoro_data, true) != NO_ERROR)
      SetError(app, "Saving pomodoro on return to menu", TIMER_LOG_ERROR);
  }

  app->is_paused = true;
  ExecuteHistory(app->screen->panels[0].scene_history, MAIN_MENU);
  app->screen->panels[0].menu_index = MAIN_MENU_MENU;
}

/**
 * ---------------------------------------------------------------------------
 * Navigation Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Select the next item in the current menu.
 * @param app Pointer to the application data
 */
void SelectNextItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], 1);
}

/**
 * Select the previous item in the current menu.
 * @param app Pointer to the application data
 */
void SelectPreviousItem(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  if (app->screen->panels[app->screen->current_panel].scene_history->present ==
      MAIN_MENU)
    ChangeSelectedItem(app->menus[0], -1);
}

/**
 * Execute the action of the currently selected menu item.
 * @param app Pointer to the application data
 */
void ExecuteMenuAction(AppData* app) {
  if (app->popup_dialog != NULL) {
    Menu* current_menu = &app->popup_dialog->menu;
    int selected_index = current_menu->selected_item;
    MenuAction action = current_menu->items[selected_index].action;

    if (action) action(app);
    ResetInput(app);
    ClosePopup(app);
    return;
  }
  if (app->current_menu == -1) return;

  Menu* current_menu = app->menus[app->current_menu];
  int selected_index = current_menu->selected_item;
  MenuAction action = current_menu->items[selected_index].action;

  if (action) action(app);
  ResetInput(app);
}

/**
 * Close the currently open popup dialog.
 * @param app Pointer to the application data
 */
void ClosePopup(AppData* app) {
  if (app->popup_dialog != NULL) {
    if (app->popup_dialog->slide_type == SLIDE_TYPE_HELP) {
      FreeFloatingDialog(app->popup_dialog);
      app->popup_dialog = app->saved_popup;
      app->saved_popup = NULL;
    } else {
      FreeFloatingDialog(app->popup_dialog);
      app->popup_dialog = NULL;
    }
  }
  app->user_input = -1;
  app->last_input = -1;
}

/**
 * Navigate popup left/up (previous item).
 * @param app Pointer to the application data
 */
void GoPrevSlide(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (d && d->currentSlide > 0) {
    int stride = d->slideCount / 3;
    int icon_type = GetConfigIconType();
    SlideDef* cur = d->slides[icon_type * stride + d->currentSlide];
    const char* btn_text = NULL;
    if (cur && cur->controls && d->hovered_button >= 0 &&
        d->hovered_button < cur->controls->count)
      btn_text = cur->controls->buttons[d->hovered_button].text;
    d->currentSlide--;
    d->hovered_button = 0;
    if (btn_text) {
      SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
      if (def && def->controls) {
        for (int i = 0; i < def->controls->count; i++) {
          if (strcmp(def->controls->buttons[i].text, btn_text) == 0) {
            d->hovered_button = i;
            break;
          }
        }
      }
    }
  }
}

/**
 * Navigate to the next slide in a welcome dialog.
 * @param app Pointer to the application data
 */
void GoNextSlide(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (d) {
    int stride = d->slideCount / 3;
    if (d->currentSlide < stride - 1) {
      int icon_type = GetConfigIconType();
      SlideDef* cur = d->slides[icon_type * stride + d->currentSlide];
      const char* btn_text = NULL;
      if (cur && cur->controls && d->hovered_button >= 0 &&
          d->hovered_button < cur->controls->count)
        btn_text = cur->controls->buttons[d->hovered_button].text;
      d->currentSlide++;
      d->hovered_button = 0;
      if (btn_text) {
        SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
        if (def && def->controls) {
          for (int i = 0; i < def->controls->count; i++) {
            if (strcmp(def->controls->buttons[i].text, btn_text) == 0) {
              d->hovered_button = i;
              break;
            }
          }
        }
      }
    }
  }
}

/**
 * Select the previous button in a slide-dialog control bar.
 * Cycles the hovered index toward 0 (most commonly used for
 * SLIDE_TYPE_CONTINUE two-button layouts).
 * @param app Application state
 */
void SelectPrevButton(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int stride = d->slideCount / 3;
  int icon_type = GetConfigIconType();
  if (!d->slides) return;
  SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
  if (!def) return;
  if (def->controls && d->hovered_button > 0) {
    d->hovered_button--;
    def->hovered = d->hovered_button;
  }
}

/**
 * Select the next button in a slide-dialog control bar.
 * Cycles the hovered index toward the last button.
 * @param app Application state
 */
void SelectNextButton(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int stride = d->slideCount / 3;
  int icon_type = GetConfigIconType();
  if (!d->slides) return;
  SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
  if (!def) return;
  if (def->controls && d->hovered_button < def->controls->count - 1) {
    d->hovered_button++;
    def->hovered = d->hovered_button;
  }
}

/**
 * Execute the action bound to the currently hovered button.
 * For SLIDE_TYPE_CONTINUE: calls the button's action function
 * which typically continues or abandons the session then closes.
 * @param app Application state
 */
void ExecuteButtonAction(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int stride = d->slideCount / 3;
  int icon_type = GetConfigIconType();
  if (!d->slides) return;
  SlideDef* def = d->slides[icon_type * stride + d->currentSlide];
  if (!def || !def->controls) return;
  if (d->hovered_button >= 0 && d->hovered_button < def->controls->count &&
      def->controls->buttons[d->hovered_button].action)
    def->controls->buttons[d->hovered_button].action(app);
}

/**
 * Navigate popup left/up (previous item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemLeft(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    flushinp();
  } else
    SelectPreviousItem(app);
}

/**
 * Navigate popup right/down (next item).
 * @param app Pointer to the application data
 */
void ChangeSelectedItemRight(AppData* app) {
  if (app->popup_dialog != NULL) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    flushinp();
  } else
    SelectNextItem(app);
}

/**
 * Continue a previous unfinished pomodoro session.
 * Resumes the saved scene and hides the menu.
 * @param app Pointer to the application data
 */
void ContinuePreviousSession(AppData* app) {
  ExecuteHistory(app->screen->panels[0].scene_history,
                 app->pomodoro_data.current_step);
  app->screen->panels[0].menu_index = -1;
}

/**
 * Abandon a previous unfinished pomodoro session.
 * Resets pomodoro data to defaults and removes the
 * uncompleted log entry so the popup won't reappear.
 * @param app Pointer to the application data
 */
void AbandonPreviousSession(AppData* app) {
  app->pomodoro_data.current_step = MAIN_MENU;
  app->pomodoro_data.current_cycle = 0;
  app->pomodoro_data.current_step_time = 0;
  app->pomodoro_data.total_elapsed = 0;
  app->pomodoro_data.last_step_time = -1;
  app->pomodoro_data.status = 0;
  if (app->pomodoro_data.session_index > 0)
    RemoveUncompletedEntries(POMODORO_LOG, app->pomodoro_data.session_index);
  app->pomodoro_data.session_index = 0;
}

/**
 * ---------------------------------------------------------------------------
 * Notes Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Move selected note up in the list.
 * @param app Pointer to the application data
 */
void NoteUpApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, -1);
    return;
  }
  NoteUp(app->notes);
}

/**
 * Move selected note down in the list.
 * @param app Pointer to the application data
 */
void NoteDownApp(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) {
    ChangeSelectedItem(&app->popup_dialog->menu, 1);
    return;
  }
  NoteDown(app->notes);
}

/**
 * Toggle the selected note between done and undone.
 * @param app Pointer to the application data
 */
void ToggleTaskAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  ToggleTask(app->notes);
}

/**
 * Delete the currently selected note.
 * @param app Pointer to the application data
 */
void DeleteNoteAtNotes(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->transitioning) return;
  DeleteNote(app->notes);
}

/**
 * Add a new task with [ ] prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewTask(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  int insert_after = app->notes->current_id;
  if (insert_after >= 0) {
    bool valid = false;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == insert_after &&
          app->notes->items[i]->page_id == app->notes->current_page) {
        valid = true;
        break;
      }
    }
    if (!valid) insert_after = -1;
  }
  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = true;
    input->pending_parent_id = -1;
    input->insert_after_id = insert_after;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a new note with - prefix at the end of notes.
 * @param app Pointer to the application data
 */
void AddNewNote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  int insert_after = app->notes->current_id;
  if (insert_after >= 0) {
    bool valid = false;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == insert_after &&
          app->notes->items[i]->page_id == app->notes->current_page) {
        valid = true;
        break;
      }
    }
    if (!valid) insert_after = -1;
  }
  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = false;
    input->pending_parent_id = -1;
    input->insert_after_id = insert_after;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a subtask under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubtask(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->current_id < 0) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  NoteItem* parent = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      parent = app->notes->items[i];
      break;
    }
  }
  if (!parent) return;
  if (parent->depth >= MAX_NOTE_DEPTH) return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = true;
    input->pending_parent_id = parent->id;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Add a subnote under the selected note node.
 * @param app Pointer to the application data
 */
void AddSubnote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes->current_id < 0) return;
  if (app->notes->max_lines > 0 &&
      app->notes->total_lines >= app->notes->max_lines)
    return;

  NoteItem* parent = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      parent = app->notes->items[i];
      break;
    }
  }
  if (!parent) return;
  if (parent->depth >= MAX_NOTE_DEPTH) return;

  app->notes->current_id = -1;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input) {
    input->len = 0;
    input->cursor = 0;
    input->buffer[0] = '\0';
    input->is_task = false;
    input->pending_parent_id = parent->id;
  }
  app->screen->panels[app->screen->current_panel].mode = INSERT;
}

/**
 * Edit the selected note content (NORMAL mode).
 * Switches to INSERT mode with existing content loaded.
 * @param app Pointer to the application data
 */
void EditCurrentNote(AppData* app) {
  if (app->popup_dialog != NULL) return;
  if (app->notes == NULL || app->notes->current_id < 0) return;

  NoteItem* note = NULL;
  for (int i = 0; i < app->notes->count; i++) {
    if (app->notes->items[i]->id == app->notes->current_id) {
      note = app->notes->items[i];
      break;
    }
  }
  if (!note) return;

  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (!input) return;

  char* text = GapBufferToString(note->text);
  if (!text) return;

  input->len = 0;
  input->cursor = 0;
  input->buffer[0] = '\0';
  strncpy(input->buffer, text, 255);
  input->buffer[255] = '\0';
  input->len = strlen(input->buffer);
  input->cursor = input->len > 0 ? input->len - 1 : 0;
  input->is_task = (note->state != NOTE_PLAIN);

  free(text);
  app->screen->panels[app->screen->current_panel].mode = NORMAL;
}

/**
 * Undo last note operation.
 * @param app Pointer to the application data
 */
void UndoNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (!HistoryCanUndo(app->notes->history)) return;

  /* Don't allow undo in Move Mode */
  if (app->notes->is_move_mode) return;

  /* Check if in NORMAL mode - only allow undo if action was on current note */
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == NORMAL) {
    void* snapshot = HistoryPop(app->notes->history, true);
    NotesData* snap = (NotesData*)snapshot;
    if (snap && snap->last_affected_id != app->notes->current_id) {
      /* Not allowed - push back and return */
      HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, true);
      return;
    }
    /* Allowed - push it back for the normal undo flow */
    HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, true);
  }

  int prev_page = app->notes->current_page;
  NotesData* current = CloneNotesData(app->notes);
  void* prev = HistoryPop(app->notes->history, true);
  HistoryPush(app->notes->history, current, FreeClonedNotesData, true);
  RestoreNotesData(app->notes, prev);

  /* Force stay in DEFAULT mode, not Move Mode */
  app->notes->is_move_mode = false;

  /* If undo changed the page, start a transition */
  if (prev_page != app->notes->current_page && !app->notes->transitioning) {
    app->notes->transitioning = true;
    app->notes->transition_target = app->notes->current_page;
    int start_frame = (prev_page < app->notes->current_page)
                        ? 0
                        : app->animations[NOTES_TRANSITION]->frame_count - 1;
    RollfilmSeekFrame(app->animations[NOTES_TRANSITION], start_frame);
  }

  /* Restore input buffer and cursor to match restored note */
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && app->notes->current_id >= 0) {
    NotesData* snap = (NotesData*)prev;
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    if (note) {
      char* text = GapBufferToString(note->text);
      if (text) {
        int len = strlen(text);
        if (len >= input->max_len) len = input->max_len - 1;
        memcpy(input->buffer, text, len);
        input->buffer[len] = '\0';
        input->len = len;
        /* Restore cursor - clamp to valid range */
        if (snap && snap->saved_cursor >= 0) {
          input->cursor = snap->saved_cursor;
        } else {
          input->cursor = len;
        }
        /* Clamp to valid range for current mode */
        int max_cursor =
          (app->screen->panels[app->screen->current_panel].mode == INSERT)
            ? len
            : (len > 0 ? len - 1 : 0);
        if (input->cursor > max_cursor) input->cursor = max_cursor;
        if (input->cursor < 0) input->cursor = 0;
        free(text);
      }
    }
  }

  FreeClonedNotesData(prev);
}

/**
 * Redo last undone note operation.
 * @param app Pointer to the application data
 */
void RedoNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (!HistoryCanRedo(app->notes->history)) return;

  /* Don't allow redo in Move Mode */
  if (app->notes->is_move_mode) return;

  /* Check if in NORMAL mode - only allow redo if action was on current note */
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (current_mode == NORMAL) {
    void* snapshot = HistoryPop(app->notes->history, false);
    NotesData* snap = (NotesData*)snapshot;
    if (snap && snap->last_affected_id != app->notes->current_id) {
      HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, false);
      return;
    }
    HistoryPush(app->notes->history, snapshot, FreeClonedNotesData, false);
  }

  int prev_page = app->notes->current_page;
  NotesData* current = CloneNotesData(app->notes);
  void* next = HistoryPop(app->notes->history, false);
  HistoryPush(app->notes->history, current, FreeClonedNotesData, false);
  RestoreNotesData(app->notes, next);

  /* Force stay in DEFAULT mode, not Move Mode */
  app->notes->is_move_mode = false;

  /* If redo changed the page, start a transition */
  if (prev_page != app->notes->current_page && !app->notes->transitioning) {
    app->notes->transitioning = true;
    app->notes->transition_target = app->notes->current_page;
    int start_frame = (prev_page < app->notes->current_page)
                        ? 0
                        : app->animations[NOTES_TRANSITION]->frame_count - 1;
    RollfilmSeekFrame(app->animations[NOTES_TRANSITION], start_frame);
  }

  /* Restore input buffer and cursor to match restored note */
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  if (input && app->notes->current_id >= 0) {
    NotesData* snap = (NotesData*)next;
    NoteItem* note = NULL;
    for (int i = 0; i < app->notes->count; i++) {
      if (app->notes->items[i]->id == app->notes->current_id) {
        note = app->notes->items[i];
        break;
      }
    }
    if (note) {
      char* text = GapBufferToString(note->text);
      if (text) {
        int len = strlen(text);
        if (len >= input->max_len) len = input->max_len - 1;
        memcpy(input->buffer, text, len);
        input->buffer[len] = '\0';
        input->len = len;
        /* Restore cursor - clamp to valid range */
        if (snap && snap->saved_cursor >= 0) {
          input->cursor = snap->saved_cursor;
        } else {
          input->cursor = len;
        }
        /* Clamp to valid range for current mode */
        int max_cursor =
          (app->screen->panels[app->screen->current_panel].mode == INSERT)
            ? len
            : (len > 0 ? len - 1 : 0);
        if (input->cursor > max_cursor) input->cursor = max_cursor;
        if (input->cursor < 0) input->cursor = 0;
        free(text);
      }
    }
  }

  FreeClonedNotesData(next);
}

/**
 * ---------------------------------------------------------------------------
 * Move Mode Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Toggle move mode for reorganizing notes.
 * @param app Pointer to the application data
 */
void ToggleMoveMode(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  app->notes->is_move_mode = !app->notes->is_move_mode;
}

/**
 * Exit move mode and return to normal interaction.
 * @param app Pointer to the application data
 */
void ExitMoveMode(AppData* app) {
  if (!app || !app->notes) return;
  if (app->notes->is_move_mode) {
    app->notes->is_move_mode = false;
    app->user_input = -1;
    app->last_input = -1;
  } else {
    ToggleTaskAtNotes(app);
  }
}

/**
 * Move the selected note up one position.
 * @param app Pointer to the application data
 */
void MoveNoteUpWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode)
    MoveNoteUp(app->notes);
  else
    NoteUp(app->notes);
}

/**
 * Move the selected note down one position.
 * @param app Pointer to the application data
 */
void MoveNoteDownWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode)
    MoveNoteDown(app->notes);
  else
    NoteDown(app->notes);
}

/**
 * Promote note (move to parent level, decrease depth).
 * @param app Pointer to the application data
 */
void PromoteNoteWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) PromoteNote(app->notes);
}

/**
 * Demote note (move to child of previous sibling, increase depth).
 * @param app Pointer to the application data
 */
void DemoteNoteWrapper(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) DemoteNote(app->notes);
}

/**
 * Check if the given page has any notes.
 * @param notes Pointer to NotesData
 * @param page Page index to check
 * @return true if the page contains at least one note
 */
static bool pageHasNotes(NotesData* notes, int page) {
  for (int i = 0; i < notes->count; i++) {
    if (notes->items[i]->page_id == page) return true;
  }
  return false;
}

/**
 * Navigate to the previous page of notes.
 * Delegates to PromoteNoteWrapper in move mode.
 * @param app Pointer to the application data
 */
void NotesPrevPage(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) {
    PromoteNoteWrapper(app);
    return;
  }
  if (app->notes->transitioning) return;
  if (app->notes->current_page <= 0) return;

  app->notes->transitioning = true;
  app->notes->transition_target = app->notes->current_page - 1;
  RollfilmSeekFrame(app->animations[NOTES_TRANSITION],
                    app->animations[NOTES_TRANSITION]->frame_count - 1);
}

/**
 * Navigate to the next page of notes.
 * Delegates to DemoteNoteWrapper in move mode.
 * Caps to one blank page — only allows going beyond existing pages
 * if the current last page has content.
 * @param app Pointer to the application data
 */
void NotesNextPage(AppData* app) {
  if (!app || !app->notes) return;
  if (app->popup_dialog) return;
  if (app->notes->is_move_mode) {
    DemoteNoteWrapper(app);
    return;
  }
  if (app->notes->transitioning) return;

  int target = app->notes->current_page + 1;

  /* If navigating beyond existing pages, allow one blank page ahead */
  if (target >= app->notes->page_count) {
    /* Only create blank page if the last existing page has notes */
    if (pageHasNotes(app->notes, app->notes->page_count - 1)) {
      app->notes->page_count++;
      if (app->notes->page_count > app->notes->page_capacity) {
        app->notes->page_capacity *= 2;
      }
    } else {
      return;
    }
  }

  app->notes->transitioning = true;
  app->notes->transition_target = target;
  RollfilmSeekFrame(app->animations[NOTES_TRANSITION], 0);
}

/**
 * Quit notes scene and return to previous scene.
 * @param app Pointer to the application data
 */
void QuitAppNotes(AppData* app) {
  if (!app || !app->notes) return;
  if (app->notes->is_move_mode) {
    app->notes->is_move_mode = false;
    app->user_input = -1;
    app->last_input = -1;
    return;
  }
  if (app->user_input == app->last_input) app->running = false;
}

/**
 * ---------------------------------------------------------------------------
 * History Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Open the History Overview popup.
 * Initialises the 5-month window ending at the current month and resolves
 * the cursor to today.
 * @param app Application state
 */
void OpenHistoryPopup(AppData* app) {
  if (!app) return;
  if (app->popup_dialog) return;

  HistoryData* h = &app->history_data;

  /* 5-month window ending at current month */
  time_t now = time(NULL);
  struct tm* tm = localtime(&now);
  int curYear = tm->tm_year + 1900;
  int curMonth = tm->tm_mon + 1;

  h->firstYear = curYear;
  h->firstMonth = curMonth;
  for (int i = 0; i < HISTORY_VISIBLE_MONTHS - 1; i++) {
    if (--h->firstMonth < 1) {
      h->firstMonth = 12;
      h->firstYear--;
    }
  }

  /* Resolve cursor to today */
  h->selYear = curYear;
  h->selMonth = curMonth;
  h->selDay = tm->tm_mday;
  h->cursorDow = tm->tm_wday;
  h->cursorWeek = 0; /* will be corrected by resolveCursor if needed */
  h->dayScroll = 0;

  /* Determine which week column today falls in */
  int mw[HISTORY_VISIBLE_MONTHS], msw[HISTORY_VISIBLE_MONTHS];
  int md[HISTORY_VISIBLE_MONTHS], msd[HISTORY_VISIBLE_MONTHS];
  int y = h->firstYear, m = h->firstMonth;
  for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
    md[i] = HistDaysInMonth(y, m);
    msd[i] = HistDayOfWeek(y, m, 1);
    mw[i] = (msd[i] + md[i] + 6) / 7;
    msw[i] = (i == 0) ? 0 : msw[i - 1] + mw[i - 1];
    if (y == curYear && m == curMonth) {
      int dayOffset = curMonth - h->firstMonth;
      if (dayOffset < 0) dayOffset += 12;
      if (dayOffset < HISTORY_VISIBLE_MONTHS) {
        int localWeek =
          (tm->tm_wday < msd[i]) ? 0 : (tm->tm_mday - 1 + msd[i]) / 7;
        h->cursorWeek = msw[i] + localWeek;
      }
    }
    if (++m > 12) {
      m = 1;
      y++;
    }
  }

  HistoryResolveCursor(app);
  CreateHistoryOverviewDialog(app);
}

/**
 * Close the current history popup and return to the overview.
 * @param app Application state
 */
void HistoryCloseToOverview(AppData* app) {
  if (!app || !app->popup_dialog) return;
  SlideType cur = app->popup_dialog->slide_type;
  if (cur == SLIDE_TYPE_HISTORY_OVERVIEW) {
    ClosePopup(app);
    return;
  }
  ClosePopup(app);
  CreateHistoryOverviewDialog(app);
}

/**
 * Open the Day Detail popup for the currently selected date.
 * @param app Application state
 */
void HistoryOpenDayDetail(AppData* app) {
  if (!app || !app->popup_dialog) return;
  HistoryData* h = &app->history_data;

  /* Ensure cursor is resolved */
  if (h->selDay == 0) HistoryResolveCursor(app);

  h->dayScroll = 0;
  ClosePopup(app);
  CreateHistoryDayDialog(app);
}

/**
 * Open the Statistics popup.
 * @param app Application state
 */
void HistoryOpenStatistics(AppData* app) {
  if (!app || !app->popup_dialog) return;
  ClosePopup(app);
  CreateHistoryStatsDialog(app);
}

/**
 * Move the history cursor left.
 * When near the first week of the month (day ≤ 7) moves by 1 day to
 * cross the boundary cleanly; otherwise moves by a full week (7 days).
 * @param app Application state
 */
void HistoryCursorLeft(AppData* app) {
  if (!app) return;
  HistoryData* h = &app->history_data;
  int delta = (h->selDay > 0 && h->selDay <= 7) ? -1 : -7;
  historyMoveCursorByDays(app, delta);
}

/**
 * Move the history cursor right.
 * When near the last week of the month (day > dim-7) moves by 1 day to
 * cross the boundary cleanly; otherwise moves by a full week (7 days).
 * @param app Application state
 */
void HistoryCursorRight(AppData* app) {
  if (!app) return;
  HistoryData* h = &app->history_data;
  int dim = (h->selDay > 0) ? HistDaysInMonth(h->selYear, h->selMonth) : 31;
  int delta = (h->selDay > 0 && h->selDay > dim - 7) ? 1 : 7;
  historyMoveCursorByDays(app, delta);
}

/**
 * Move the history cursor up by one calendar day.
 * @param app Application state
 */
void HistoryCursorUp(AppData* app) { historyMoveCursorByDays(app, -1); }

/**
 * Move the history cursor down by one calendar day.
 * @param app Application state
 */
void HistoryCursorDown(AppData* app) { historyMoveCursorByDays(app, 1); }

/**
 * Scroll the day-detail session list up.
 * @param app Application state
 */
void HistoryScrollUp(AppData* app) {
  if (!app || !app->popup_dialog) return;
  HistoryData* h = &app->history_data;
  if (h->dayScroll > 0) h->dayScroll--;
}

/**
 * Scroll the day-detail session list down.
 * @param app Application state
 */
void HistoryScrollDown(AppData* app) {
  if (!app || !app->popup_dialog) return;
  HistoryData* h = &app->history_data;
  int indices[100];
  time_t times[100];
  int dur[100], st[100];
  int total = HistSessionsForDay(POMODORO_LOG, h->selYear, h->selMonth,
                                 h->selDay, indices, times, dur, st, 100);
  if (h->dayScroll < total - 1) h->dayScroll++;
}

/**
 * Rebuild the history overview popup in-place.
 * Closes the current dialog and re-creates it.
 * @param app Application state
 */
static void historyRebuildOverview(AppData* app) {
  CreateHistoryOverviewDialog(app);
}

/**
 * Move the history cursor by a given number of calendar days.
 * Replaces grid-based cursor movement with proper date arithmetic so
 * crossing month boundaries works correctly (up = day-1, down = day+1).
 * If the target date falls outside the visible window it auto-scrolls.
 * @param app Application state
 * @param dayDelta Number of days to move (positive = forward, negative = backward)
 */
static void historyMoveCursorByDays(AppData* app, int dayDelta) {
  if (!app || !app->popup_dialog) return;
  HistoryData* h = &app->history_data;

  /* Start from current resolved date, or today if none */
  int y = h->selYear, m = h->selMonth, d = h->selDay;
  if (d <= 0) {
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    y = tm->tm_year + 1900;
    m = tm->tm_mon + 1;
    d = tm->tm_mday;
  }

  /* Add dayDelta via mktime normalization */
  struct tm date = {0};
  date.tm_year = y - 1900;
  date.tm_mon = m - 1;
  date.tm_mday = d + dayDelta;
  mktime(&date);
  int newYear = date.tm_year + 1900;
  int newMon = date.tm_mon + 1;
  int newDay = date.tm_mday;
  int newDow = date.tm_wday;

  /* Check if new date is within current 5-month window */
  {
    int wy = h->firstYear, wm = h->firstMonth;
    int inWindow = 0;
    for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
      if (wy == newYear && wm == newMon) {
        inWindow = 1;
        break;
      }
      if (++wm > 12) {
        wm = 1;
        wy++;
      }
    }
    if (!inWindow) {
      /* Scroll window so the new date is in the last visible month */
      h->firstYear = newYear;
      h->firstMonth = newMon;
      for (int i = 0; i < HISTORY_VISIBLE_MONTHS - 1; i++) {
        if (--h->firstMonth < 1) {
          h->firstMonth = 12;
          h->firstYear--;
        }
      }
    }
  }

  /* Compute grid layout for the (possibly scrolled) window */
  int mw[HISTORY_VISIBLE_MONTHS], msw[HISTORY_VISIBLE_MONTHS];
  int md[HISTORY_VISIBLE_MONTHS], msd[HISTORY_VISIBLE_MONTHS];
  {
    int y2 = h->firstYear, m2 = h->firstMonth;
    int acc = 0;
    for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
      md[i] = HistDaysInMonth(y2, m2);
      msd[i] = HistDayOfWeek(y2, m2, 1);
      mw[i] = (msd[i] + md[i] + 6) / 7;
      msw[i] = acc;
      acc += mw[i];
      if (++m2 > 12) {
        m2 = 1;
        y2++;
      }
    }
  }

  /* Find the month index and compute grid position */
  {
    int my = h->firstYear, mm = h->firstMonth;
    for (int i = 0; i < HISTORY_VISIBLE_MONTHS; i++) {
      if (my == newYear && mm == newMon) {
        int localWeek = (msd[i] + newDay - 1) / 7;
        if (localWeek >= mw[i]) localWeek = mw[i] - 1;
        h->cursorWeek = msw[i] + localWeek;
        h->cursorDow = newDow;
        break;
      }
      if (++mm > 12) {
        mm = 1;
        my++;
      }
    }
  }

  h->selYear = newYear;
  h->selMonth = newMon;
  h->selDay = newDay;
  historyRebuildOverview(app);
}

/**
 * ---------------------------------------------------------------------------
 * Preferences Actions
 * ---------------------------------------------------------------------------
 */

/**
 * Open the preferences dialog from the main menu.
 * @param app Application state
 */
void OpenPreferencesMenu(AppData* app) {
  if (app->popup_dialog) return;
  app->popup_dialog = CreatePreferencesDialog(app);
}

void OpenHelp(AppData* app) {
  if (app->popup_dialog && app->popup_dialog->slide_type == SLIDE_TYPE_HELP)
    return;
  int scene;
  if (app->popup_dialog) {
    switch (app->popup_dialog->slide_type) {
      case SLIDE_TYPE_PREFERENCES:
      case SLIDE_TYPE_PREFS_STEPPER:
      case SLIDE_TYPE_PREFS_SELECT:
        scene = SCENE_PREFERENCES;
        break;
      case SLIDE_TYPE_NOISE:
        scene = SCENE_NOISE;
        break;
      case SLIDE_TYPE_CONTINUE:
        scene = SCENE_CONTINUE;
        break;
      case SLIDE_TYPE_HISTORY_OVERVIEW:
        scene = SCENE_HISTORY_OVERVIEW;
        break;
      case SLIDE_TYPE_HISTORY_DAY:
        scene = SCENE_HISTORY_DAY;
        break;
      case SLIDE_TYPE_HISTORY_STATS:
        scene = SCENE_HISTORY_STATS;
        break;
      default:
        scene = 1 << app->screen->panels[app->screen->current_panel]
                       .scene_history->present;
    }
  } else {
    scene = 1 << app->screen->panels[app->screen->current_panel]
                   .scene_history->present;
  }
  app->saved_popup = app->popup_dialog;
  app->help_context_scene = scene;
  app->help_scroll_row = 0;
  app->popup_dialog = CreateHelpDialog(app);
}

void OpenHelpMenu(AppData* app) {
  if (app->popup_dialog) return;
  app->saved_popup = NULL;
  app->help_context_scene = ALL_SCENES;
  app->help_scroll_row = 0;
  app->popup_dialog = CreateHelpDialog(app);
}

void HelpScrollUp(AppData* app) {
  if (app->help_scroll_row > 0) app->help_scroll_row--;
}

void HelpScrollDown(AppData* app) { app->help_scroll_row++; }

/**
 * After a selection change, ensure the selected field is visible by adjusting
 * scroll_row.
 */
static int contentRowForField(AppData* app, int field_idx);
static int totalPrefContentRows(AppData* app);

static void ensurePrefVisible(AppData* app) {
  if (!app->popup_dialog) return;
  int sel = app->popup_dialog->hovered_button;
  if (sel == 0) {
    app->prefs.scroll_row = 0;
    return;
  }
  int idx = prefFieldIndexBySelectOffset(app, sel);
  if (idx < 0) return;
  int row = contentRowForField(app, idx);
  int h = app->popup_dialog->size.height;
  int max_visible = h - 6;
  int sr = app->prefs.scroll_row;
  int total_cr = totalPrefContentRows(app);
  int end_cr = sr + max_visible;
  if (end_cr > total_cr) end_cr = total_cr;
  bool show_up = (sr > 0);
  bool show_down = (end_cr < total_cr);
  int field_cap = max_visible - (show_up ? 1 : 0) - (show_down ? 1 : 0);
  if (field_cap < 1) field_cap = 1;
  if (row < sr)
    app->prefs.scroll_row = row;
  else if (row >= sr + field_cap)
    app->prefs.scroll_row = row - field_cap + 1;
}

/**
 * Select the previous setting in the preferences dialog.
 * @param app Application state
 */
void PrefsSelectPrev(AppData* app) {
  if (!app->popup_dialog) return;
  int sel = app->popup_dialog->hovered_button;
  if (sel > 0) sel--;
  app->popup_dialog->hovered_button = sel;
  ensurePrefVisible(app);
}

/**
 * Select the next setting in the preferences dialog.
 * @param app Application state
 */
void PrefsSelectNext(AppData* app) {
  if (!app->popup_dialog) return;
  int sel = app->popup_dialog->hovered_button;
  int max = 0;
  for (int i = 0; i < app->prefs.count; i++)
    if (app->prefs.fields[i].type != PREF_SECTION) max++;
  if (sel < max - 1) sel++;
  app->popup_dialog->hovered_button = sel;
  ensurePrefVisible(app);
}

/**
 * Decrease the value of the selected setting.
 * @param app Application state
 */
void PrefsValueDown(AppData* app) {
  if (!app->popup_dialog) return;
  int idx =
    prefFieldIndexBySelectOffset(app, app->popup_dialog->hovered_button);
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (f->type == PREF_SELECT) {
    int val = *f->int_value - 1;
    if (val < 0) val = f->option_count - 1;
    *f->int_value = val;
    if (f->int_value == &g_config.visual.icons_index) {
      SyncIconsFromIndex();
      app->animations = app->icon_animations[GetConfigIconType()];
      app->screen->min_panel_size = GetWidestAndTallestAnimation(app);
    }
    return;
  }
  int val = (f->type == PREF_STEPPER_FLOAT)
              ? (int)(*f->float_value * 100.0f + 0.5f)
              : *f->int_value;
  val -= f->step;
  if (val < f->min) val = f->min;
  if (f->type == PREF_STEPPER_FLOAT)
    *f->float_value = val / 100.0f;
  else
    *f->int_value = val;
}

/**
 * Increase the value of the selected setting.
 * @param app Application state
 */
void PrefsValueUp(AppData* app) {
  if (!app->popup_dialog) return;
  int idx =
    prefFieldIndexBySelectOffset(app, app->popup_dialog->hovered_button);
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (f->type == PREF_SELECT) {
    int val = *f->int_value + 1;
    if (val >= f->option_count) val = 0;
    *f->int_value = val;
    if (f->int_value == &g_config.visual.icons_index) {
      SyncIconsFromIndex();
      app->animations = app->icon_animations[GetConfigIconType()];
      app->screen->min_panel_size = GetWidestAndTallestAnimation(app);
    }
    return;
  }
  int val = (f->type == PREF_STEPPER_FLOAT)
              ? (int)(*f->float_value * 100.0f + 0.5f)
              : *f->int_value;
  val += f->step;
  if (val > f->max) val = f->max;
  if (f->type == PREF_STEPPER_FLOAT)
    *f->float_value = val / 100.0f;
  else
    *f->int_value = val;
}

/**
 * Toggle a boolean setting.
 * @param app Application state
 */
void PrefsToggle(AppData* app) {
  if (!app->popup_dialog) return;
  int idx =
    prefFieldIndexBySelectOffset(app, app->popup_dialog->hovered_button);
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (f->type == PREF_SELECT) {
    int val = *f->int_value + 1;
    if (val >= f->option_count) val = 0;
    *f->int_value = val;
    if (f->int_value == &g_config.visual.icons_index) {
      SyncIconsFromIndex();
      app->animations = app->icon_animations[GetConfigIconType()];
      app->screen->min_panel_size = GetWidestAndTallestAnimation(app);
    }
    return;
  }
  if (f->type != PREF_TOGGLE) return;
  *f->int_value = !*f->int_value;
}

/**
 * Open the edit sub-dialog for the selected setting (stepper/selector).
 * @param app Application state
 */
void PrefsEdit(AppData* app) {
  if (!app->popup_dialog) return;
  int idx =
    prefFieldIndexBySelectOffset(app, app->popup_dialog->hovered_button);
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (f->type == PREF_TOGGLE || f->type == PREF_STEPPER_INT ||
      f->type == PREF_STEPPER_FLOAT) {
    FloatingDialog* d = CreatePrefsStepperDialog(app, idx);
    if (d) {
      FreeFloatingDialog(app->popup_dialog);
      app->popup_dialog = d;
    }
    return;
  }
  if (f->type == PREF_SELECT) {
    FloatingDialog* d = CreatePrefsSelectDialog(app, idx);
    if (d) {
      FreeFloatingDialog(app->popup_dialog);
      app->popup_dialog = d;
    }
    return;
  }
  FloatingDialog* d = CreatePrefsStepperDialog(app, idx);
  if (d) {
    FreeFloatingDialog(app->popup_dialog);
    app->popup_dialog = d;
  }
}

/**
 * Go back from the preferences dialog.
 * @param app Application state
 */
void PrefsBack(AppData* app) { ClosePopup(app); }

/**
 * After a scroll, clamp the selection so it stays inside the visible field
 * area (between the More indicators).
 */
static int contentRowForField(AppData* app, int field_idx) {
  int r = 0;
  for (int i = 0; i < field_idx; i++) {
    if (app->prefs.fields[i].type == PREF_SECTION) {
      if (i > 0)
        r += 2;
      else
        r++;
    } else {
      r++;
    }
  }
  return r;
}

static int totalPrefContentRows(AppData* app) {
  int r = 0;
  for (int i = 0; i < app->prefs.count; i++) {
    if (app->prefs.fields[i].type == PREF_SECTION && i > 0) {
      r += 2;
    } else {
      r++;
    }
  }
  return r;
}

static void clampSelectionToView(AppData* app) {
  if (!app->popup_dialog) return;
  int sel = app->popup_dialog->hovered_button;
  int idx = prefFieldIndexBySelectOffset(app, sel);
  if (idx < 0) return;
  int h = app->popup_dialog->size.height;
  int max_visible = h - 6;
  int sr = app->prefs.scroll_row;
  int total_cr = totalPrefContentRows(app);
  int end_cr = sr + max_visible;
  if (end_cr > total_cr) end_cr = total_cr;
  bool show_down = (end_cr < total_cr);
  bool show_up = (sr > 0);
  int field_cap = max_visible - (show_up ? 1 : 0) - (show_down ? 1 : 0);
  if (field_cap < 1) field_cap = 1;
  int sel_cr = contentRowForField(app, idx);
  if (sel_cr < sr) {
    while (sel_cr < sr && sel < app->prefs.count) {
      sel++;
      int nidx = prefFieldIndexBySelectOffset(app, sel);
      if (nidx < 0) break;
      sel_cr = contentRowForField(app, nidx);
    }
    app->popup_dialog->hovered_button = sel;
  }
  if (sel_cr >= sr + field_cap) {
    while (sel_cr >= sr + field_cap && sel > 0) {
      sel--;
      int nidx = prefFieldIndexBySelectOffset(app, sel);
      if (nidx < 0) break;
      sel_cr = contentRowForField(app, nidx);
    }
    app->popup_dialog->hovered_button = sel;
  }
}

/**
 * Scroll the preferences list up by one content row.
 * @param app Application state
 */
void PrefsScrollUp(AppData* app) {
  if (app->prefs.scroll_row > 0) {
    app->prefs.scroll_row--;
    clampSelectionToView(app);
  }
}

/**
 * Scroll the preferences list down by one content row.
 * @param app Application state
 */
void PrefsScrollDown(AppData* app) {
  int total_cr = totalPrefContentRows(app);
  int h = app->popup_dialog ? app->popup_dialog->size.height : 25;
  int max_visible = h - 6;
  if (app->prefs.scroll_row + max_visible < total_cr) {
    app->prefs.scroll_row++;
    clampSelectionToView(app);
  }
}

/**
 * Preview the currently selected setting. For sound/notification fields: plays
 * the example audio/test notification. Not bound to keyboard, only accessible
 * via mouse hover in stepper/select popups.
 * @param app Application state
 */
void PrefsPreview(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx;
  if (d->slide_type == SLIDE_TYPE_PREFS_SELECT)
    idx = app->prefs.select_index;
  else if (d->slide_type == SLIDE_TYPE_PREFS_STEPPER)
    idx = d->hovered_button;
  else
    idx = prefFieldIndexBySelectOffset(app, d->hovered_button);
  if (idx < 0 || idx >= app->prefs.count) return;
  PrefField* f = &app->prefs.fields[idx];
  if (f->preview) f->preview(app);
}

/**
 * Decrement the value in a preferences stepper sub-dialog.
 * Used for numeric settings (stepper INT/FLOAT).
 * @param app Application state
 */
void StepperDecrement(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = d->hovered_button;
  if (idx < 0 || idx >= app->prefs.count) return;
  PrefField* f = &app->prefs.fields[idx];
  int val = GetPrefInt(app, idx);
  val -= f->step;
  if (val < f->min) val = f->min;
  if (f->type == PREF_STEPPER_FLOAT)
    *f->float_value = val / 100.0f;
  else
    *f->int_value = val;
}

/**
 * Increment the value in a preferences stepper sub-dialog.
 * Used for numeric settings (stepper INT/FLOAT).
 * @param app Application state
 */
void StepperIncrement(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = d->hovered_button;
  if (idx < 0 || idx >= app->prefs.count) return;
  PrefField* f = &app->prefs.fields[idx];
  int val = GetPrefInt(app, idx);
  val += f->step;
  if (val > f->max) val = f->max;
  if (f->type == PREF_STEPPER_FLOAT)
    *f->float_value = val / 100.0f;
  else
    *f->int_value = val;
}

/**
 * Close the stepper sub-dialog and return to the main preferences dialog.
 * @param app Application state
 */
void StepperClose(AppData* app) {
  SyncIconsFromIndex();
  FreeFloatingDialog(app->popup_dialog);
  app->popup_dialog = CreatePreferencesDialog(app);
}

/**
 * Apply the selected value and close the option selector sub-dialog.
 * @param app Application state
 */
void SelectApply(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = app->prefs.select_index;
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  *f->int_value = d->hovered_button;
  ClosePrefsSelect(app);
}

/**
 * Cancel selection and return to the main preferences dialog.
 * @param app Application state
 */
void SelectCancel(AppData* app) { ClosePrefsSelect(app); }

/**
 * Navigate to the previous option in a preferences select sub-dialog.
 * @param app Application state
 */
void SelectPrevOption(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = app->prefs.select_index;
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (d->hovered_button > 0) d->hovered_button--;
}

/**
 * Navigate to the next option in a preferences select sub-dialog.
 * @param app Application state
 */
void SelectNextOption(AppData* app) {
  FloatingDialog* d = app->popup_dialog;
  if (!d) return;
  int idx = app->prefs.select_index;
  if (idx < 0) return;
  PrefField* f = &app->prefs.fields[idx];
  if (d->hovered_button < f->option_count - 1) d->hovered_button++;
}

/**
 * Convert a selectable offset to a preference field index.
 * Walks through app->prefs.fields and skips PREF_SECTION entries.
 * Selectable offset: 0 for first non-section preference, 1 for second, etc.
 * @param app Application state
 * @param sel_off Selectable offset (0-based after skipping sections)
 * @return Preference field index, or -1 if out of range
 */
static int prefFieldIndexBySelectOffset(AppData* app, int sel_off) {
  int n = 0;
  for (int i = 0; i < app->prefs.count; i++) {
    if (app->prefs.fields[i].type == PREF_SECTION) continue;
    if (n == sel_off) return i;
    n++;
  }
  return -1;
}
