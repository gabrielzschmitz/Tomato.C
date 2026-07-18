#include "draw.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anim.h"
#include "config.h"
#include "error.h"
#include "init.h"
#include "input.h"
#include "notes.h"
#include "tomato.h"
#include "ui.h"
#include "util.h"

/* PRIVATE DRAW FUNCTIONS */
static void renderPopups(AppData* app);
static void renderPanel(AppData* app, Border border);
static void renderPanelScene(AppData* app, Panel* panel, Rollfilm* animation,
                             Dimensions size, Vector2D position);
static void renderPomodoroScene(AppData* app, Rollfilm* animation,
                                SceneType scene, Dimensions size,
                                Vector2D position);
static void renderNotesScene(AppData* app, Panel* panel, Rollfilm* animation);
static void renderNotesPageIndicator(AppData* app, Panel* panel,
                                     Rollfilm* animation);
static void renderDialogPopups(AppData* app);
static void renderAnimationDebug(Panel* panel, Rollfilm* animation,
                                 Vector2D offset);
static void renderDebugInfo(AppData* app);

/**
 * Draw the entire screen based on the current app state.
 * Renders all visible panels, animations, and UI elements.
 * @param app Pointer to the application data
 * @return ErrorType NO_ERROR on success, or an error code on failure
 */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  ClearClickRegions(app);
  renderPopups(app);

  if (!ValidateAndRenderScreenSize(app)) return status;

  Border border = InitBorder();
  renderPanel(app, border);

  RenderStatusBar(app->status_bar, app->screen, HasErrors());
  renderDialogPopups(app);

  if (DEBUG) renderDebugInfo(app);

  RenderErrorLine();
  refresh();
  return status;
}

/**
 * Check if the screen size is sufficient and render the error screen.
 * Returns false if screen is too small, true otherwise.
 * @param app Pointer to the application data
 * @return true if screen is large enough, false otherwise
 */
bool ValidateAndRenderScreenSize(AppData* app) {
  if (app->screen->size.width < app->screen->min_panel_size.width ||
      app->screen->size.height < app->screen->min_panel_size.height) {
    app->block_input = true;
    RenderScreenSizeError(app->screen);
    refresh();
    return false;
  }
  app->block_input = false;
  return true;
}

/**
 * Render popup dialogs (quit, skip, reset) when triggered.
 * Only renders in NORMAL or DEFAULT mode.
 * @param app Pointer to the application data
 */
static void renderPopups(AppData* app) {
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  InputState* input = app->screen->panels[app->screen->current_panel].input;
  bool is_default_mode =
    (current_mode == DEFAULT) || (input && input->len == 0);
  if (is_default_mode && app->notes != NULL && !app->notes->is_move_mode &&
      IsKeyAssignedToAction(app->user_input, QuitApp))
    RenderQuitConfirmation(app);

  int steps[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
  size_t steps_count = sizeof(steps) / sizeof(steps[0]);
  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (IsKeyAssignedToAction(app->last_input, SkipPomodoroStep) &&
      IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step) &&
      (POMODORO_SCENES & (1 << current_scene)))
    RenderSkipConfirmation(app);
  if ((IsKeyAssignedToAction(app->last_input, ResetPomodoroStep) ||
       IsKeyAssignedToAction(app->last_input, ResetPomodoroCycle)) &&
      IsCurrentStepInList(steps, steps_count,
                          app->pomodoro_data.current_step) &&
      (POMODORO_SCENES & (1 << current_scene)))
    RenderResetMenu(app);
}

/**
 * Render all visible panels.
 * Also handles scene-specific rendering and panel debug info.
 * @param app Pointer to the application data
 * @param border Border style to use
 */
static void renderPanel(AppData* app, Border border) {
  for (int i = 0; i < MAX_PANELS; i++) {
    Panel* current_panel = &app->screen->panels[i];
    if (!current_panel->visible) continue;

    Rollfilm* animation = NULL;

    int indices[] = {WORK_TIME, SHORT_PAUSE, LONG_PAUSE};
    int largest_index = RollfilmLargest(app->animations, indices, 3);
    if (largest_index == -1) {
      SetError(app, "renderPanel", UPDATE_ERROR);
      return;
    }
    if (app->animations[largest_index] == NULL) {
      SetError(app, "renderPanel", NULL_POINTER_ERROR);
      return;
    }
    Dimensions size = {.width = app->animations[largest_index]->frame_width,
                       .height = app->animations[largest_index]->frame_height};
    Vector2D position = {.x = current_panel->position.x +
                              (current_panel->size.width -
                               app->animations[largest_index]->frame_width) /
                                2,
                         .y = current_panel->position.y +
                              (current_panel->size.height -
                               app->animations[largest_index]->frame_height) /
                                2};

    renderPanelScene(app, current_panel, animation, size, position);

    if (DEBUG && current_panel->visible) {
      mvprintw(1, 2, "M:%d,%d[%#x] CS:%d L:%d CM:%d RG:%d", app->mouse_x,
               app->mouse_y, app->mouse_bstate, app->pomodoro_data.current_step,
               app->last_input, app->current_menu, app->click_region_count);
    }

    SetColor((app->screen->current_panel == i) ? FOCUSED_PANEL_COLOR
                                               : UNFOCUSED_PANEL_COLOR,
             NO_COLOR, A_NORMAL);
    RenderPanelBorder(*current_panel, border);
  }
}

/**
 * Route scene-specific rendering to appropriate handlers.
 * Dispatches to menu, pomodoro, or notes renderers based on scene type.
 * @param app Pointer to the application data
 * @param panel Pointer to the panel to render
 * @param animation Pointer to the animation to render (may be NULL)
 * @param size Dimensions of the animation frame
 * @param position Position for status text rendering
 */
static void renderPanelScene(AppData* app, Panel* panel, Rollfilm* animation,
                             Dimensions size, Vector2D position) {
  SceneType scene = panel->scene_history->present;

  switch (scene) {
    case MAIN_MENU:
      if (ANIMATIONS) {
        animation = app->animations[MAIN_MENU];
        if (DEBUG) renderAnimationDebug(panel, animation, (Vector2D){0, 0});
        RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
      } else {
        animation = app->animations[MAIN_MENU];
        if (animation) {
          RollfilmSeekFrame(animation, animation->default_frame);
          RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
        }
      }
      if (DEBUG)
        PrintMenuAtCenter(app, panel, app->menus[0],
                          (Vector2D){0, animation->frame_height / 2 + 3}, 0);
      else
        PrintMenuAtCenter(app, panel, app->menus[0],
                          (Vector2D){0, animation->frame_height / 2 + 2}, 0);
      break;
    case WORK_TIME:
    case SHORT_PAUSE:
    case LONG_PAUSE:
      if (ANIMATIONS) {
        animation = app->animations[scene];
        if (DEBUG) renderAnimationDebug(panel, animation, (Vector2D){0, 0});
        RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
      } else {
        animation = app->animations[scene];
        if (animation) {
          RollfilmSeekFrame(animation, animation->default_frame);
          RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
        }
      }
      renderPomodoroScene(app, animation, scene, size, position);
      break;
    case NOTES:
      if (ANIMATIONS) {
        if (app->notes && app->notes->transitioning) {
          animation = app->animations[NOTES_TRANSITION];
          if (animation) {
            if (DEBUG) renderAnimationDebug(panel, animation, (Vector2D){0, 0});
            RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
            renderNotesPageIndicator(app, panel, animation);
          }
        } else {
          animation = app->animations[NOTES];
          renderNotesScene(app, panel, animation);
          if (animation) {
            if (DEBUG) renderAnimationDebug(panel, animation, (Vector2D){0, 0});
            RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
            renderNotesPageIndicator(app, panel, animation);
          }
        }
      } else {
        animation = app->animations[NOTES];
        renderNotesScene(app, panel, animation);
        if (animation) {
          RollfilmSeekFrame(animation, animation->default_frame);
          RenderAnimationAtPanelCenter(panel, animation, (Vector2D){0, 0});
        }
        renderNotesPageIndicator(app, panel, animation);
      }
      break;
    case NOTES_TRANSITION:
      break;
    case HELP:
    case CONTINUE:
      if (ANIMATIONS) animation = app->animations[scene];
      break;
    default:
      break;
  }
}

/**
 * Render pomodoro status text (timer display) for pomodoro scenes.
 * Adjusts position offset for DEBUG mode.
 * @param app Pointer to the application data
 * @param animation Pointer to the animation (unused)
 * @param scene Scene type (WORK_TIME, SHORT_PAUSE, LONG_PAUSE)
 * @param size Dimensions for the status text
 * @param position Base position for rendering
 */
static void renderPomodoroScene(AppData* app, Rollfilm* animation,
                                SceneType scene, Dimensions size,
                                Vector2D position) {
  (void)animation;
  Vector2D status_pos = position;
  if (DEBUG) {
    if (scene == SHORT_PAUSE)
      status_pos.y += 2;
    else if (scene == LONG_PAUSE)
      status_pos.y += 1;
  }
  RenderPomodoroStatus(app, size, status_pos);
}

/**
 * Render notes content for the NOTES scene.
 * Calculates bounds based on animation blanks or panel size.
 * @param app Pointer to the application data
 * @param panel Pointer to the panel to render
 * @param animation Pointer to the animation (may be NULL)
 */
static void renderNotesScene(AppData* app, Panel* panel, Rollfilm* animation) {
  if (app->notes == NULL) return;

  int start_x, start_y, end_x, end_y;
  if (animation != NULL) {
    int panel_center_x = panel->position.x + panel->size.width / 2;
    int panel_center_y = panel->position.y + panel->size.height / 2;
    int anim_x = panel_center_x - animation->frame_width / 2;
    int anim_y = panel_center_y - animation->frame_height / 2;
    int blank_x, blank_y;
    if (RollfilmFirstBlank(animation, &blank_x, &blank_y)) {
      start_x = anim_x + blank_x;
      start_y = anim_y + blank_y;
    } else {
      start_x = anim_x;
      start_y = anim_y + animation->frame_height;
    }
    if (RollfilmLastBlank(animation, &blank_x, &blank_y)) {
      end_x = anim_x + blank_x;
      end_y = anim_y + blank_y + 1;
    } else {
      end_x = anim_x + animation->frame_width;
      end_y = anim_y + animation->frame_height;
    }
  } else {
    start_x = panel->position.x + 1;
    start_y = panel->position.y + 1;
    end_x = panel->position.x + panel->size.width - 1;
    end_y = panel->position.y + panel->size.height - 1;
  }
  if (end_x <= start_x) end_x = start_x + 1;
  if (end_y <= start_y) end_y = start_y + 1;

  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  InputState* input = panel->input;

  RenderNotes(app, app->notes, start_x, start_y, end_x, end_y, input,
              current_mode);

  if (DEBUG)
    RenderNotesHistoryDebug(app->notes, panel->position.x + panel->size.width,
                            panel->position.y);
}

/**
 * Render page indicator below the notes animation.
 * Shows current page and total pages, e.g. "< 1/3 >".
 * Left/right arrows are clickable with hover highlight.
 * @param app Pointer to the application data
 * @param panel Pointer to the panel
 * @param animation Pointer to the animation (may be NULL)
 */
static void renderNotesPageIndicator(AppData* app, Panel* panel,
                                     Rollfilm* animation) {
  if (!app->notes) return;

  int indicator_y;
  int panel_center_x = panel->position.x + panel->size.width / 2;

  if (animation) {
    int panel_center_y = panel->position.y + panel->size.height / 2;
    int frame_y = panel_center_y - animation->frame_height / 2;
    indicator_y = frame_y + animation->frame_height;
  } else {
    indicator_y = panel->position.y + panel->size.height - 2;
  }

  int num = app->notes->current_page + 1;
  int total = app->notes->page_count;

  char page_str[24];
  snprintf(page_str, sizeof(page_str), " %02d/%02d ", num, total);
  int page_str_len = (int)strlen(page_str);

  int left_x = panel_center_x - page_str_len / 2 - 1;
  int right_x = panel_center_x + page_str_len / 2 + 1;

  bool hover_left = (app->mouse_y == indicator_y && app->mouse_x >= left_x &&
                     app->mouse_x < left_x + 1);
  bool hover_right = (app->mouse_y == indicator_y && app->mouse_x >= right_x &&
                      app->mouse_x < right_x + 1);

  /* Render left arrow with hover effect */
  if (hover_left)
    SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
  else
    SetColor(COLOR_WHITE, NO_COLOR, A_DIM);
  mvaddch(indicator_y, left_x, '<');

  /* Render page number */
  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);
  mvprintw(indicator_y, panel_center_x - page_str_len / 2, "%s", page_str);

  /* Render right arrow with hover effect */
  if (hover_right)
    SetColor(COLOR_BLACK, COLOR_WHITE, A_NORMAL);
  else
    SetColor(COLOR_WHITE, NO_COLOR, A_DIM);
  mvaddch(indicator_y, right_x, '>');

  SetColor(COLOR_WHITE, NO_COLOR, A_NORMAL);

  /* Register click regions */
  RegisterClickRegion(app, left_x, indicator_y, 1, 1, REGION_DIRECT,
                      NotesPrevPage, -1, -1, -1);
  RegisterClickRegion(app, right_x, indicator_y, 1, 1, REGION_DIRECT,
                      NotesNextPage, -1, -1, -1);
}

/**
 * Render dialog popups on top of the screen.
 * Only renders if a popup dialog exists and mode is NORMAL or DEFAULT.
 * @param app Pointer to the application data
 */
static void renderDialogPopups(AppData* app) {
  int current_mode = app->screen->panels[app->screen->current_panel].mode;
  if (app->popup_dialog == NULL ||
      (current_mode != NORMAL && current_mode != DEFAULT))
    return;

  int current_scene =
    app->screen->panels[app->screen->current_panel].scene_history->present;
  if (app->popup_dialog->menu.items[0].action == ForcefullyQuitApp)
    RenderQuitConfirmation(app);
  else if (app->popup_dialog->menu.items[0].action ==
             ForcefullySkipPomodoroStep &&
           (POMODORO_SCENES & (1 << current_scene)))
    RenderSkipConfirmation(app);
  else if ((app->popup_dialog->menu.items[0].action == ResetPomodoroStep ||
            app->popup_dialog->menu.items[0].action == ResetPomodoroCycle) &&
           (POMODORO_SCENES & (1 << current_scene)))
    RenderResetMenu(app);
  else if (app->popup_dialog->slide_type == SLIDE_TYPE_WELCOME ||
           app->popup_dialog->slide_type == SLIDE_TYPE_CONTINUE ||
           app->popup_dialog->slide_type == SLIDE_TYPE_NOISE ||
           app->popup_dialog->slide_type == SLIDE_TYPE_HISTORY_OVERVIEW ||
           app->popup_dialog->slide_type == SLIDE_TYPE_HISTORY_DAY ||
           app->popup_dialog->slide_type == SLIDE_TYPE_HISTORY_STATS ||
           app->popup_dialog->slide_type == SLIDE_TYPE_PREFERENCES ||
           app->popup_dialog->slide_type == SLIDE_TYPE_PREFS_STEPPER ||
           app->popup_dialog->slide_type == SLIDE_TYPE_PREFS_SELECT ||
           app->popup_dialog->slide_type == SLIDE_TYPE_HELP) {
    int stride = app->popup_dialog->slideCount / 3;
    int icon_type = GetConfigIconType();
    int idx = icon_type * stride + app->popup_dialog->currentSlide;
    if (!app->popup_dialog->slides) return;
    SlideDef* def = app->popup_dialog->slides[idx];
    if (!def) return;
    if (def->update) def->update(app, def);
    def->render(app, def);
  } else {
    UpdateFloatingDialog(app->popup_dialog, app->screen);
    RenderFloatingDialog(app, app->popup_dialog);
  }
}

/**
 * Debug overlay for animation rendering.
 * Shows frame count, width, seconds multiplier, and frame ID.
 * @param panel Pointer to the panel to render in
 * @param animation Pointer to the animation to render
 * @param offset Offset from the panel position
 */
static void renderAnimationDebug(Panel* panel, Rollfilm* animation,
                                 Vector2D offset) {
  const int DEBUG_INFO_WIDTH = 23;
  if (animation == NULL) return;

  int panel_center_x = panel->position.x + panel->size.width / 2;
  int panel_center_y = panel->position.y + panel->size.height / 2;

  int frame_x = panel_center_x - animation->frame_width / 2 + offset.x;
  int frame_y = panel_center_y - animation->frame_height / 2 + offset.y;

  int debug_y = frame_y + animation->frame_height;
  int debug_x = frame_x + (animation->frame_width - DEBUG_INFO_WIDTH) / 2;

  SetColor(COLOR_WHITE, COLOR_BLACK, A_BOLD);
  mvprintw(debug_y, debug_x, "%dC - %dW - %1.3lfS - %dID",
           animation->frame_count, animation->frame_width,
           animation->frames->seconds_multiplier, animation->frames->id);
  mvprintw(debug_y + 1, debug_x + 5, "%lf", animation->delta_frame_ms);
}

/**
 * Render debug information (scene, paused state, panel size).
 * Only active when DEBUG flag is enabled.
 * @param app Pointer to the application data
 */
static void renderDebugInfo(AppData* app) {
  SetColor(COLOR_BLACK, COLOR_WHITE, A_BOLD);
  Panel* current_panel = &app->screen->panels[app->screen->current_panel];
  mvprintw(current_panel->position.y, current_panel->position.x,
           "%02dM - %dP - %02dWx%02dH", current_panel->scene_history->present,
           app->is_paused, current_panel->size.width,
           current_panel->size.height);
  mvprintw(current_panel->position.y, current_panel->position.x + 20, "%dx%d",
           app->screen->min_panel_size.width,
           app->screen->min_panel_size.height);
}
