#include "draw.h"

#include <ncurses.h>
#include <stdio.h>

#include "init.h"
#include "input.h"
#include "ui.h"

/* Print at screen */
ErrorType DrawScreen(AppData* app) {
  ErrorType status = NO_ERROR;
  erase();

  if (IsKeyAssignedToAction(app->last_input, QuitApp))
    RenderQuitConfirmation(app);

  if (!CheckScreenSize(app)) return status;

  Border border = InitBorder();
  for (int i = 0; i < MAX_PANELS; i++) {
    Panel* current_panel = &app->screen->panels[i];
    if (!current_panel->visible) continue;

    Rollfilm* animation = NULL;
    switch (current_panel->scene_history->present) {
      case MAIN_MENU:
        if (ANIMATIONS) {
          animation = app->animations[MAIN_MENU];
          RenderAnimationAtPanelCenter(current_panel, animation,
                                       (Vector2D){0, 0});
        }
        PrintMenuAtCenter(current_panel, app->menus[0],
                          (Vector2D){0, animation->frame_height / 2 + 2}, 0);
        break;
      case WORK_TIME:
        if (ANIMATIONS) animation = app->animations[WORK_TIME];
        break;
      case SHORT_PAUSE:
        if (ANIMATIONS) animation = app->animations[SHORT_PAUSE];
        break;
      case LONG_PAUSE:
        if (ANIMATIONS) animation = app->animations[LONG_PAUSE];
        break;
      case NOTES:
        if (ANIMATIONS) {
          animation = app->animations[NOTES];
          RenderAnimationAtPanelCenter(current_panel, animation,
                                       (Vector2D){0, 0});
        }
        break;
      case HELP:
        if (ANIMATIONS) animation = app->animations[HELP];
        break;
      case CONTINUE:
        if (ANIMATIONS) animation = app->animations[CONTINUE];
        break;
      default: break;
    }
    if (DEBUG && current_panel->visible){
      mvprintw(1, 2, "%c/%c - %dcm", app->user_input, app->last_input, app->current_menu);
    }

    SetColor((app->screen->current_panel == i) ? FOCUSED_PANEL_COLOR
                                               : UNFOCUSED_PANEL_COLOR,
             NO_COLOR, A_NORMAL);
    RenderPanelBorder(*current_panel, border);
  }

  RenderStatusBar(app->status_bar, app->screen);

  if (app->popup_dialog != NULL)
    if(app->popup_dialog->menu.items[0].action == ForcefullyQuitApp)
      RenderQuitConfirmation(app);

  if (DEBUG) {
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

  refresh();
  return status;
}

/* Check and Render Screen Size */
bool CheckScreenSize(AppData* app) {
  if (app->screen->size.width < app->screen->min_panel_size.width ||
      app->screen->size.height < app->screen->min_panel_size.height) {
    app->block_input = true;
    RenderScreenSizeError(app->screen,
                          &app->screen->panels[app->screen->current_panel]);
    refresh();
    return false;
  }
  app->block_input = false;
  return true;
}

/* Show debug info and render a animation */
void DebugAnimation(Panel* panel, Rollfilm* animation, Vector2D offset) {
  const int DEBUG_INFO_WIDTH = 23;
  if (animation) {
    RenderAnimationAtPanelCenter(panel, animation, offset);

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
}
